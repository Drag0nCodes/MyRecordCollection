import express from "express";
import cors from "cors";
import mysql from "mysql2/promise";
import dotenv from "dotenv";
import cookieParser from "cookie-parser";
import path from 'path';
import { fileURLToPath } from 'url';
import { v4 as uuidv4 } from "uuid";
import bcrypt from "bcrypt";
import jwt from "jsonwebtoken";
import fetch from 'node-fetch';

dotenv.config();

const app = express();
app.use(express.json());
app.use(cors({
  origin: process.env.FRONTEND_ORIGIN || 'http://localhost:5173',
  credentials: true
}));
app.use(cookieParser());

const PORT = Number(process.env.PORT || 4000);
// bind to 0.0.0.0 so the server is reachable from other machines (GCE VM)
const HOST = process.env.HOST || '0.0.0.0';
const JWT_SECRET = process.env.JWT_SECRET;
const DEFAULT_COLLECTION_NAME = "My Collection";

// In production we require a JWT secret
if (process.env.NODE_ENV === 'production' && !JWT_SECRET) {
  console.error('Missing JWT_SECRET in production environment. Set JWT_SECRET and restart.');
  process.exit(1);
}

// Helper to issue JWT
function issueToken(userUuid) {
  return jwt.sign({ userUuid }, JWT_SECRET);
}

// Create a single shared pool (previously a new pool was created per request causing 'Too many connections')
let _pool; // singleton reference
function getPool() {
  if (!_pool) {
    _pool = mysql.createPool({
      host: process.env.DB_HOST,
      user: process.env.DB_USER,
      password: process.env.DB_PASS,
      database: process.env.DB_NAME,
      waitForConnections: true,
      connectionLimit: Number(process.env.DB_CONN_LIMIT || 10),
      queueLimit: 0,
    });
    console.log("MySQL pool created");
  }
  return _pool;
}

async function getUserTableId(pool, userUuid, tableName) {
  const [rows] = await pool.execute(
    `SELECT id FROM RecTable WHERE userUuid = ? AND name = ? LIMIT 1`,
    [userUuid, tableName]
  );
  return rows.length > 0 ? rows[0].id : null;
}

async function fetchLastFmCover(artist, record) {
  const apiKey = process.env.LASTFM_API_KEY;
  if (!apiKey) return null;
  const query = `${record}`.trim(); // search by record title only per request
  if (!query) return null;
  const url = `https://ws.audioscrobbler.com/2.0/?method=album.search&album=${encodeURIComponent(
    query
  )}&api_key=${apiKey}&format=json&limit=5`;
  try {
    const response = await fetch(url);
    if (!response.ok) {
      return null;
    }
    const data = await response.json();
    const albums = data?.results?.albummatches?.album;
    if (!Array.isArray(albums) || albums.length === 0) return null;

    // normalize target artist for comparison
    const targetArtist = (artist || "").toLowerCase().trim();

    // helper to extract extralarge (or best fallback) from album image array
    const pickExtralarge = (album) => {
      const images = Array.isArray(album?.image) ? album.image : [];
      // find extralarge first
      const extral = images.find((img) => img && img.size === 'extralarge' && img['#text']);
      if (extral && extral['#text']) return extral['#text'];
      // fallback to largest available (prefer mega, then large, then medium, then small)
      const order = ['extralarge', 'large', 'medium', 'small'];
      for (const sz of order) {
        const found = images.find((img) => img && img.size === sz && img['#text']);
        if (found && found['#text']) return found['#text'];
      }
      return null;
    };

    // Try to find an album where the artist attribute matches (case-insensitive)
    if (targetArtist) {
      for (const album of albums) {
        const aArtist = (album?.artist || "").toLowerCase().trim();
        if (aArtist && aArtist === targetArtist) {
          const urlText = pickExtralarge(album);
          if (urlText) return urlText;
        }
      }
    }

    // No exact artist match found — use the first album's cover (prefer extralarge)
    const firstCover = pickExtralarge(albums[0]);
    return firstCover || null;
  } catch (err) {
    console.warn('Last.fm cover lookup failed', err);
    return null;
  }
}

// Graceful shutdown to release pool connections
async function shutdown() {
  if (_pool) {
    try {
      console.log("Closing MySQL pool...");
      await _pool.end();
      console.log("MySQL pool closed");
    } catch (e) {
      console.error("Error closing MySQL pool", e);
    }
  }
  process.exit(0);
}
process.on('SIGINT', shutdown);
process.on('SIGTERM', shutdown);

// JWT auth middleware
function requireAuth(req, res, next) {
  const token = req.cookies.token;
  if (!token) return res.status(401).json({ error: "Not authenticated" });
  try {
    const payload = jwt.verify(token, JWT_SECRET);
    req.userUuid = payload.userUuid;
    next();
  } catch {
    return res.status(401).json({ error: "Invalid token" });
  }
}

app.get("/api/records", requireAuth, async (req, res) => {
  console.log("Fetching records...");
  const tableName = typeof req.query.table === "string" ? req.query.table : null;
  if (!tableName) {
    return res.status(400).json({ error: "table query parameter required" });
  }
  try {
    const pool = await getPool();
    const tableId = await getUserTableId(pool, req.userUuid, tableName);
    if (!tableId) {
      return res.status(404).json({ error: "Collection not found" });
    }

    const [rows] = await pool.query(
      `SELECT r.id, r.name as record, r.artist, r.cover, r.rating, r.release_year as 'release', r.added as dateAdded, r.tableId
       FROM Record r WHERE r.userUuid = ? AND r.tableId = ?`,
      [req.userUuid, tableId]
    );

    const recordIds = rows.map((r) => r.id);
    const tagsByRecord = {};
    if (recordIds.length > 0) {
      const placeholders = recordIds.map(() => "?").join(", ");
      const [tagRows] = await pool.query(
        `SELECT t.name, tg.recordId FROM Tag t JOIN Tagged tg ON t.id = tg.tagId WHERE tg.recordId IN (${placeholders})`,
        recordIds
      );
      for (const tr of tagRows) {
        const rid = tr.recordId;
        tagsByRecord[rid] = tagsByRecord[rid] || [];
        tagsByRecord[rid].push(tr.name);
      }
    }

    const out = rows.map((r) => ({
      ...r,
      tableId: r.tableId,
      tags: tagsByRecord[r.id] || [],
    }));
    res.json(out);
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "DB error" });
  }
});

app.get("/api/tags", requireAuth, async (req, res) => {
  console.log("Fetching tags...");
  try {
    const pool = await getPool();
    const [rows] = await pool.query(`SELECT name FROM Tag WHERE userUuid = ? ORDER BY name`, [req.userUuid]);
    res.json(rows.map((r) => r.name));
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: "DB error" });
  }
});

// Register endpoint
app.post('/api/register', async (req, res) => {
  console.log("Registering user...");
  const { username, password, displayName: rawDisplayName } = req.body;
  if (!username || !password) {
    return res.status(400).json({ error: 'Username and password required.' });
  }
  if (username.length < 3 || username.length > 30) {
    return res.status(400).json({ error: 'Username must be 3-30 characters.' });
  }
  if (!/^[a-zA-Z0-9_]+$/.test(username)) {
    return res.status(400).json({ error: 'Username must contain only letters, numbers, and underscores.' });
  }
  if (password.length < 8) {
    return res.status(400).json({ error: 'Password must be at least 8 characters.' });
  }
  if (!/(?=.*[A-Za-z])(?=.*\d)(?=.*[^A-Za-z0-9])/.test(password)) {
    return res.status(400).json({ error: 'Password must contain at least one letter, one number, and one special character.' });
  }
  try {
    const hashedPassword = await bcrypt.hash(password, 10);
    const userUuid = uuidv4();
    const displayName =
      typeof rawDisplayName === "string" && rawDisplayName.trim()
        ? rawDisplayName.trim().slice(0, 50)
        : username;
    const pool = await getPool();
    await pool.execute(
      'INSERT INTO User (uuid, username, displayName, password) VALUES (?, ?, ?, ?)',
      [userUuid, username, displayName, hashedPassword]
    );
    await pool.execute(
      `INSERT INTO RecTable (name, userUuid) VALUES (?, ?), (?, ?)`,
      ["My Collection", userUuid, "Wishlist", userUuid]
    );
    const token = issueToken(userUuid);
  res.cookie('token', token, { httpOnly: true, sameSite: process.env.CROSS_SITE_COOKIES === 'true' ? 'none' : 'lax', secure: process.env.NODE_ENV === 'production', maxAge: 7*24*60*60*1000 });
    res.json({ success: true });
  } catch (err) {
    if (err.code === 'ER_DUP_ENTRY') {
      return res.status(409).json({ error: 'Username already exists.' });
    }
    res.status(500).json({ error: 'Registration failed.' });
  }
});

// Login endpoint
app.post('/api/login', async (req, res) => {
  console.log("Logging in...");
  const { username, password } = req.body;
  if (!username || !password) {
    return res.status(400).json({ error: 'Username and password required.' });
  }
  try {
    const pool = await getPool();
    const [rows] = await pool.execute(
      'SELECT uuid, password FROM User WHERE username = ?',
      [username]
    );
    if (rows.length === 0) {
      return res.status(401).json({ error: 'Invalid credentials.' });
    }
    const user = rows[0];
    const match = await bcrypt.compare(password, user.password);
    if (!match) {
      return res.status(401).json({ error: 'Invalid credentials.' });
    }
    const token = issueToken(user.uuid);
  res.cookie('token', token, { httpOnly: true, sameSite: process.env.CROSS_SITE_COOKIES === 'true' ? 'none' : 'lax', secure: process.env.NODE_ENV === 'production', maxAge: 7*24*60*60*1000 });
    res.json({ success: true });
  } catch (err) {
    res.status(500).json({ error: 'Login failed.' });
  }
});

// Logout endpoint
app.post('/api/logout', (req, res) => {
  console.log("Logging out...");
  res.clearCookie('token', { httpOnly: true, sameSite: process.env.CROSS_SITE_COOKIES === 'true' ? 'none' : 'lax', secure: process.env.NODE_ENV === 'production' });
  res.json({ success: true });
});

app.get('/api/me', requireAuth, async (req, res) => {
  try {
    const pool = await getPool();
    const [rows] = await pool.execute('SELECT username, displayName FROM User WHERE uuid = ?', [req.userUuid]);
    if (rows.length === 0) return res.status(404).json({ error: 'User not found' });
    // Also return the userUuid so clients can wire analytics user_id without decoding the token
    res.json({ username: rows[0].username, displayName: rows[0].displayName, userUuid: req.userUuid });
  } catch (err) {
    res.status(500).json({ error: 'Failed to fetch user info' });
  }
});

app.patch('/api/profile', requireAuth, async (req, res) => {
  const { username: newUsername, displayName: rawDisplayName } = req.body || {};
  if (!newUsername && !rawDisplayName) {
    return res.status(400).json({ error: 'Nothing to update' });
  }

  if (newUsername) {
    if (typeof newUsername !== 'string' || newUsername.trim().length < 3 || newUsername.trim().length > 30) {
      return res.status(400).json({ error: 'Username must be 3-30 characters.' });
    }
    if (!/^[a-zA-Z0-9_]+$/.test(newUsername)) {
      return res.status(400).json({ error: 'Username must contain only letters, numbers, and underscores.' });
    }
  }

  let displayName;
  if (rawDisplayName !== undefined) {
    if (typeof rawDisplayName !== 'string' || !rawDisplayName.trim()) {
      return res.status(400).json({ error: 'Display name cannot be empty.' });
    }
    displayName = rawDisplayName.trim().slice(0, 50);
  }

  try {
    const pool = await getPool();
    if (newUsername) {
      const [existing] = await pool.execute(
        'SELECT uuid FROM User WHERE username = ? AND uuid <> ? LIMIT 1',
        [newUsername, req.userUuid]
      );
      if (existing.length > 0) {
        return res.status(409).json({ error: 'Username already taken.' });
      }
    }

    const fields = [];
    const params = [];
    if (newUsername) {
      fields.push('username = ?');
      params.push(newUsername);
    }
    if (displayName !== undefined) {
      fields.push('displayName = ?');
      params.push(displayName);
    }
    if (fields.length === 0) {
      return res.status(400).json({ error: 'Nothing to update' });
    }
    params.push(req.userUuid);
    await pool.execute(
      `UPDATE User SET ${fields.join(', ')} WHERE uuid = ?`,
      params
    );

    const [rows] = await pool.execute(
      'SELECT username, displayName FROM User WHERE uuid = ?',
      [req.userUuid]
    );
    res.json({ success: true, user: rows[0] });
  } catch (err) {
    console.error('Profile update failed', err);
    res.status(500).json({ error: 'Failed to update profile' });
  }
});

app.post('/api/profile/password', requireAuth, async (req, res) => {
  const { currentPassword, newPassword, confirmPassword } = req.body || {};
  if (!currentPassword || !newPassword || !confirmPassword) {
    return res.status(400).json({ error: 'All password fields are required.' });
  }
  if (newPassword !== confirmPassword) {
    return res.status(400).json({ error: 'New passwords do not match.' });
  }
  if (newPassword.length < 8) {
    return res.status(400).json({ error: 'Password must be at least 8 characters.' });
  }
  if (!/(?=.*[A-Za-z])(?=.*\d)(?=.*[^A-Za-z0-9])/.test(newPassword)) {
    return res.status(400).json({ error: 'Password must contain at least one letter, one number, and one special character.' });
  }

  try {
    const pool = await getPool();
    const [rows] = await pool.execute('SELECT password FROM User WHERE uuid = ?', [req.userUuid]);
    if (rows.length === 0) {
      return res.status(404).json({ error: 'User not found' });
    }
    const hash = rows[0].password;
    const matches = await bcrypt.compare(currentPassword, hash);
    if (!matches) {
      return res.status(401).json({ error: 'Current password is incorrect.' });
    }
    const newHash = await bcrypt.hash(newPassword, 10);
    await pool.execute('UPDATE User SET password = ? WHERE uuid = ?', [newHash, req.userUuid]);
    res.json({ success: true });
  } catch (err) {
    console.error('Password change failed', err);
    res.status(500).json({ error: 'Failed to change password' });
  }
});

// Serve static built frontend in production (Vite outputs to `dist`)
if (process.env.NODE_ENV === 'production') {
  try {
    const __filename = fileURLToPath(import.meta.url);
    const __dirname = path.dirname(__filename);
    const clientDist = path.join(__dirname, 'dist');
    app.use(express.static(clientDist));
    // Don't let the SPA fallback swallow API requests - skip paths that start with /api
    app.get('*', (req, res, next) => {
      if (req.path && req.path.startsWith('/api')) return next();
      res.sendFile(path.join(clientDist, 'index.html'));
    });
    console.log('Serving static frontend from', clientDist);
  } catch (e) {
    console.warn('Could not enable static serving of frontend:', e && e.message);
  }
}

// Create or update a record
app.post('/api/records/update', requireAuth, async (req, res) => {
  console.log("Updating record...");
  const { id, record, artist, cover, rating, tags, release } = req.body;
  if (!id || !record) return res.status(400).json({ error: 'Missing id or record name' });
  // Validate release year
  const releaseNum = Number(release);
  if (!Number.isInteger(releaseNum) || releaseNum < 1877 || releaseNum > 2100) {
    return res.status(400).json({ error: 'Invalid release year' });
  }
  try {
    const pool = await getPool();
    // Update main record
    await pool.execute(
      `UPDATE Record SET name = ?, artist = ?, cover = ?, rating = ?, release_year = ? WHERE id = ? AND userUuid = ?`,
      [record, artist, cover, rating, releaseNum, id, req.userUuid]
    );
    // Remove old tags
    await pool.execute(`DELETE FROM Tagged WHERE recordId = ?`, [id]);
    // Add new tags (create if missing)
    for (const tagName of tags || []) {
      let [tagRows] = await pool.execute(`SELECT id FROM Tag WHERE name = ? AND userUuid = ?`, [tagName, req.userUuid]);
      let tagId;
      if (tagRows.length === 0) {
        const [result] = await pool.execute(`INSERT INTO Tag (name, userUuid) VALUES (?, ?)`, [tagName, req.userUuid]);
        tagId = result.insertId;
      } else {
        tagId = tagRows[0].id;
      }
      await pool.execute(`INSERT IGNORE INTO Tagged (recordId, tagId) VALUES (?, ?)`, [id, tagId]);
    }
    // Return updated record
    const [rows] = await pool.execute(
      `SELECT r.id, r.name as record, r.artist, r.cover, r.rating, r.release_year as 'release', r.added as dateAdded, r.tableId FROM Record r WHERE r.id = ? AND r.userUuid = ?`,
      [id, req.userUuid]
    );
    const updated = rows[0];
    // Get tags
    const [tagRows] = await pool.execute(
      `SELECT t.name FROM Tag t JOIN Tagged tg ON t.id = tg.tagId WHERE tg.recordId = ?`,
      [id]
    );
    updated.tags = tagRows.map((t) => t.name);
    res.json(updated);
  } catch (err) {
    res.status(500).json({ error: 'Failed to update record' });
  }
});

app.post('/api/records/create', requireAuth, async (req, res) => {
  console.log("Creating record...");
  const { record, artist, cover, rating, tags, release, tableName } = req.body;
  if (!record) return res.status(400).json({ error: 'Missing record name' });
  if (!tableName || typeof tableName !== 'string') {
    return res.status(400).json({ error: 'tableName is required' });
  }
  // Validate release year
  const releaseNum = Number(release);
  if (!Number.isInteger(releaseNum) || releaseNum < 1877 || releaseNum > 2100) {
    return res.status(400).json({ error: 'invalid release year' });
  }
  try {
    const pool = await getPool();
    const tableId = await getUserTableId(pool, req.userUuid, tableName);
    if (!tableId) {
      return res.status(404).json({ error: 'Collection not found' });
    }
    const [result] = await pool.execute(
      `INSERT INTO Record (name, artist, cover, rating, release_year, tableId, userUuid, added) VALUES (?, ?, ?, ?, ?, ?, ?, NOW())`,
      [record, artist, cover, rating, releaseNum, tableId, req.userUuid]
    );
    const newId = result.insertId;
    // Add tags (create if missing)
    for (const tagName of tags || []) {
      let [tagRows] = await pool.execute(`SELECT id FROM Tag WHERE name = ? AND userUuid = ?`, [tagName, req.userUuid]);
      let tagId;
      if (tagRows.length === 0) {
        const [tagResult] = await pool.execute(`INSERT INTO Tag (name, userUuid) VALUES (?, ?)`, [tagName, req.userUuid]);
        tagId = tagResult.insertId;
      } else {
        tagId = tagRows[0].id;
      }
      await pool.execute(`INSERT IGNORE INTO Tagged (recordId, tagId) VALUES (?, ?)`, [newId, tagId]);
    }
    // Return new record
    const [rows] = await pool.execute(
      `SELECT r.id, r.name as record, r.artist, r.cover, r.rating, r.release_year as 'release', r.added as dateAdded, r.tableId FROM Record r WHERE r.id = ? AND r.userUuid = ?`,
      [newId, req.userUuid]
    );
    const created = rows[0];
    // Get tags
    const [tagRows] = await pool.execute(
      `SELECT t.name FROM Tag t JOIN Tagged tg ON t.id = tg.tagId WHERE tg.recordId = ?`,
      [newId]
    );
    created.tags = tagRows.map((t) => t.name);
    res.json(created);
  } catch (err) {
    res.status(500).json({ error: 'Failed to create record' });
  }
});

// Tag management endpoints
app.post('/api/tags/create', requireAuth, async (req, res) => {
  const { name } = req.body;
  if (!name || !name.trim()) return res.status(400).json({ error: 'Tag name required' });
  try {
    const pool = await getPool();
    const trimmed = name.trim();
    // Check duplicate
    const [existing] = await pool.execute(`SELECT id FROM Tag WHERE name = ? AND userUuid = ?`, [trimmed, req.userUuid]);
    if (existing.length > 0) return res.status(409).json({ error: 'Tag already exists' });
    await pool.execute(`INSERT INTO Tag (name, userUuid) VALUES (?, ?)`, [trimmed, req.userUuid]);
    const [rows] = await pool.execute(`SELECT name FROM Tag WHERE userUuid = ? ORDER BY name`, [req.userUuid]);
    res.json({ tags: rows.map(r => r.name) });
  } catch (err) {
    res.status(500).json({ error: 'Failed to create tag' });
  }
});

app.post('/api/tags/rename', requireAuth, async (req, res) => {
  const { oldName, newName } = req.body;
  if (!oldName || !newName) return res.status(400).json({ error: 'oldName and newName required' });
  try {
    const pool = await getPool();
    const trimmedNew = newName.trim();
    if (!trimmedNew) return res.status(400).json({ error: 'New name cannot be empty' });
    const [dup] = await pool.execute(`SELECT id FROM Tag WHERE name = ? AND userUuid = ?`, [trimmedNew, req.userUuid]);
    if (dup.length > 0) return res.status(409).json({ error: 'A tag with that name already exists' });
    const [result] = await pool.execute(`UPDATE Tag SET name = ? WHERE name = ? AND userUuid = ?`, [trimmedNew, oldName, req.userUuid]);
    if (result.affectedRows === 0) return res.status(404).json({ error: 'Tag not found' });
    const [rows] = await pool.execute(`SELECT name FROM Tag WHERE userUuid = ? ORDER BY name`, [req.userUuid]);
    res.json({ tags: rows.map(r => r.name) });
  } catch (err) {
    res.status(500).json({ error: 'Failed to rename tag' });
  }
});

app.post('/api/tags/delete', requireAuth, async (req, res) => {
  const { name } = req.body;
  if (!name) return res.status(400).json({ error: 'Tag name required' });
  try {
    const pool = await getPool();
    const [tagRows] = await pool.execute(`SELECT id FROM Tag WHERE name = ? AND userUuid = ?`, [name, req.userUuid]);
    if (tagRows.length === 0) return res.status(404).json({ error: 'Tag not found' });
    const tagId = tagRows[0].id;
    await pool.execute(`DELETE FROM Tagged WHERE tagId = ?`, [tagId]);
    await pool.execute(`DELETE FROM Tag WHERE id = ? AND userUuid = ?`, [tagId, req.userUuid]);
    const [rows] = await pool.execute(`SELECT name FROM Tag WHERE userUuid = ? ORDER BY name`, [req.userUuid]);
    res.json({ tags: rows.map(r => r.name) });
  } catch (err) {
    res.status(500).json({ error: 'Failed to delete tag' });
  }
});

// Delete a record
app.post('/api/records/delete', requireAuth, async (req, res) => {
  console.log('Deleting record...');
  const { id } = req.body;
  if (!id) return res.status(400).json({ error: 'Missing id' });
  try {
    const pool = await getPool();
    // Ensure record belongs to user
    const [rows] = await pool.execute(`SELECT id FROM Record WHERE id = ? AND userUuid = ?`, [id, req.userUuid]);
    if (rows.length === 0) return res.status(404).json({ error: 'Record not found' });
    await pool.execute(`DELETE FROM Record WHERE id = ? AND userUuid = ?`, [id, req.userUuid]);
    res.json({ success: true });
  } catch (err) {
    console.error(err);
    res.status(500).json({ error: 'Failed to delete record' });
  }
});

app.post('/api/import/discogs', requireAuth, async (req, res) => {
  console.log('Importing Discogs collection...');
  const { records, tableName } = req.body || {};
  if (!Array.isArray(records) || records.length === 0) {
    return res.status(400).json({ error: 'records array is required' });
  }
  const targetTable =
    typeof tableName === 'string' && tableName.trim()
      ? tableName.trim()
      : DEFAULT_COLLECTION_NAME;

  try {
    const pool = await getPool();
    const tableId = await getUserTableId(pool, req.userUuid, targetTable);
    if (!tableId) {
      return res.status(404).json({ error: `Collection '${targetTable}' not found` });
    }

    let created = 0;
    let skipped = 0;
    let withoutCover = 0;
    const tagCache = new Map();

    for (const raw of records) {
      if (!raw || typeof raw !== 'object') {
        skipped += 1;
        continue;
      }
      const recordName = typeof raw.record === 'string' ? raw.record.trim() : '';
      const artist = typeof raw.artist === 'string' ? raw.artist.trim() : '';
      if (!recordName || !artist) {
        skipped += 1;
        continue;
      }

      const releaseNum = Number.parseInt(raw.release, 10);
      let release = Number.isInteger(releaseNum) ? releaseNum : 1900;
      if (release < 1877 || release > 2100) {
        release = 1900;
      }

      const ratingNum = Number(raw.rating);
      let rating = Number.isFinite(ratingNum) ? Math.round(ratingNum) : 0;
      if (rating < 0) rating = 0;
      if (rating > 10) rating = 10;

      const dateVal = typeof raw.dateAdded === 'string' ? raw.dateAdded.trim() : '';
      const dateAdded = /^\d{4}-\d{2}-\d{2}$/.test(dateVal) ? dateVal : null;

      const tagsArray = Array.isArray(raw.tags) ? raw.tags : [];
      const cleanTags = Array.from(
        new Set(
          tagsArray
            .filter((tag) => typeof tag === 'string')
            .map((tag) => tag.trim())
            .filter(Boolean)
        )
      );

      const [existingRows] = await pool.execute(
        `SELECT id FROM Record WHERE userUuid = ? AND tableId = ? AND LOWER(name) = ? AND LOWER(artist) = ? LIMIT 1`,
        [req.userUuid, tableId, recordName.toLowerCase(), artist.toLowerCase()]
      );
      if (existingRows.length > 0) {
        skipped += 1;
        continue;
      }

      const cover = await fetchLastFmCover(artist, recordName);
      if (!cover) {
        withoutCover += 1;
      }

      const addedDate = dateAdded || new Date().toISOString().slice(0, 10);
      const [insertResult] = await pool.execute(
        `INSERT INTO Record (name, artist, cover, rating, release_year, tableId, userUuid, added) VALUES (?, ?, ?, ?, ?, ?, ?, ?)`,
        [recordName, artist, cover || null, rating, release, tableId, req.userUuid, addedDate]
      );
      const newRecordId = insertResult.insertId;
      created += 1;

      for (const tagName of cleanTags) {
        const cacheKey = tagName.toLowerCase();
        let tagId = tagCache.get(cacheKey);
        if (!tagId) {
          const [tagRows] = await pool.execute(
            `SELECT id FROM Tag WHERE name = ? AND userUuid = ? LIMIT 1`,
            [tagName, req.userUuid]
          );
          if (tagRows.length > 0) {
            tagId = tagRows[0].id;
          } else {
            const [tagInsert] = await pool.execute(
              `INSERT INTO Tag (name, userUuid) VALUES (?, ?)`,
              [tagName, req.userUuid]
            );
            tagId = tagInsert.insertId;
          }
          tagCache.set(cacheKey, tagId);
        }
        await pool.execute(
          `INSERT IGNORE INTO Tagged (recordId, tagId) VALUES (?, ?)`,
          [newRecordId, tagId]
        );
      }
    }

    res.json({ success: true, created, skipped, withoutCover });
  } catch (err) {
    console.error('Discogs import failed', err);
    res.status(500).json({ error: 'Failed to import Discogs collection' });
  }
});

// Delete all records in a user's collection (table)
app.post('/api/records/clear', requireAuth, async (req, res) => {
  const { tableName } = req.body || {};
  const targetTable = typeof tableName === 'string' && tableName.trim() ? tableName.trim() : DEFAULT_COLLECTION_NAME;
  try {
    const pool = await getPool();
    const tableId = await getUserTableId(pool, req.userUuid, targetTable);
    if (!tableId) {
      return res.status(404).json({ error: `Collection '${targetTable}' not found` });
    }
    const [result] = await pool.execute(`DELETE FROM Record WHERE userUuid = ? AND tableId = ?`, [req.userUuid, tableId]);
    const deleted = result.affectedRows || 0;
    res.json({ success: true, deleted });
  } catch (err) {
    console.error('Failed to clear collection', err);
    res.status(500).json({ error: 'Failed to clear collection' });
  }
});

// Delete all tags for the user (and associated Tagged rows)
app.post('/api/tags/clear', requireAuth, async (req, res) => {
  try {
    const pool = await getPool();
    const [tagRows] = await pool.execute(`SELECT id FROM Tag WHERE userUuid = ?`, [req.userUuid]);
    const ids = (tagRows || []).map((r) => r.id).filter(Boolean);
    if (ids.length === 0) {
      return res.json({ success: true, tagsDeleted: 0, taggedDeleted: 0 });
    }
    const placeholders = ids.map(() => '?').join(',');
    const [taggedDel] = await pool.execute(`DELETE FROM Tagged WHERE tagId IN (${placeholders})`, ids);
    const taggedDeleted = taggedDel.affectedRows || 0;
    const [tagDel] = await pool.execute(`DELETE FROM Tag WHERE id IN (${placeholders})`, ids);
    const tagsDeleted = tagDel.affectedRows || 0;
    res.json({ success: true, tagsDeleted, taggedDeleted });
  } catch (err) {
    console.error('Failed to clear tags', err);
    res.status(500).json({ error: 'Failed to clear tags' });
  }
});

// Proxy to Last.fm album.search (requires LASTFM_API_KEY in env)
app.get('/api/lastfm/album.search', requireAuth, async (req, res) => {
  console.log("Proxying Last.fm album.search...");
  const { q } = req.query;
  if (!q || typeof q !== 'string') return res.status(400).json({ error: 'Missing q param' });
  const apiKey = process.env.LASTFM_API_KEY;
  if (!apiKey) return res.status(500).json({ error: 'Missing LASTFM_API_KEY on server' });
  try {
    const url = `https://ws.audioscrobbler.com/2.0/?method=album.search&album=${encodeURIComponent(q)}&api_key=${apiKey}&format=json&limit=10`;
    const r = await fetch(url);
    if (!r.ok) {
      const body = await r.text();
      console.error('Last.fm upstream error', r.status, body);
      return res.status(502).json({ error: 'Last.fm upstream failure', status: r.status });
    }
    const data = await r.json();
    res.json(data);
  } catch (err) {
    console.error('Last.fm proxy error', err);
    res.status(500).json({ error: 'Failed to query Last.fm' });
  }
});

app.listen(PORT, HOST, () => {
  console.log(`Server listening on http://${HOST}:${PORT}`);
});
