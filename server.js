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
  try {
    const pool = await getPool();
    const [rows] = await pool.query(
      `SELECT r.id, r.name as record, r.artist, r.cover, r.rating, r.release_year as 'release', r.added as dateAdded
       FROM Record r WHERE r.userUuid = ?`,
      [req.userUuid]
    );
    // Fetch tags for each record
    const [tagRows] = await pool.query(
      `SELECT t.name, tg.recordId FROM Tag t JOIN Tagged tg ON t.id = tg.tagId WHERE t.userUuid = ?`,
      [req.userUuid]
    );
    const tagsByRecord = {};
    for (const tr of tagRows) {
      const rid = tr.recordId;
      tagsByRecord[rid] = tagsByRecord[rid] || [];
      tagsByRecord[rid].push(tr.name);
    }
    const out = rows.map((r) => ({
      ...r,
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
  const { username, password } = req.body;
  if (!username || !password) {
    return res.status(400).json({ error: 'Username and password required.' });
  }
  try {
    const hashedPassword = await bcrypt.hash(password, 10);
    const userUuid = uuidv4();
    const pool = await getPool();
    await pool.execute(
      'INSERT INTO User (uuid, username, password) VALUES (?, ?, ?)',
      [userUuid, username, hashedPassword]
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
    const [rows] = await pool.execute('SELECT username FROM User WHERE uuid = ?', [req.userUuid]);
    if (rows.length === 0) return res.status(404).json({ error: 'User not found' });
    res.json({ username: rows[0].username });
  } catch (err) {
    res.status(500).json({ error: 'Failed to fetch user info' });
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
  try {
    const pool = await getPool();
    // Update main record
    await pool.execute(
      `UPDATE Record SET name = ?, artist = ?, cover = ?, rating = ?, release_year = ? WHERE id = ? AND userUuid = ?`,
      [record, artist, cover, rating, release, id, req.userUuid]
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
      `SELECT r.id, r.name as record, r.artist, r.cover, r.rating, r.release_year as 'release', r.added as dateAdded FROM Record r WHERE r.id = ? AND r.userUuid = ?`,
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
  const { record, artist, cover, rating, tags, release } = req.body;
  if (!record) return res.status(400).json({ error: 'Missing record name' });
  try {
    const pool = await getPool();
    const [result] = await pool.execute(
      `INSERT INTO Record (name, artist, cover, rating, release_year, userUuid, added) VALUES (?, ?, ?, ?, ?, ?, NOW())`,
      [record, artist, cover, rating, release, req.userUuid]
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
      `SELECT r.id, r.name as record, r.artist, r.cover, r.rating, r.release_year as 'release', r.added as dateAdded FROM Record r WHERE r.id = ? AND r.userUuid = ?`,
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

// Proxy to Last.fm album.search (requires LASTFM_API_KEY in env)
app.get('/api/lastfm/album.search', requireAuth, async (req, res) => {
  console.log("Proxying Last.fm album.search...");
  const { q } = req.query;
  if (!q || typeof q !== 'string') return res.status(400).json({ error: 'Missing q param' });
  const apiKey = process.env.LASTFM_API_KEY;
  if (!apiKey) return res.status(500).json({ error: 'Missing LASTFM_API_KEY on server' });
  try {
    const url = `https://ws.audioscrobbler.com/2.0/?method=album.search&album=${encodeURIComponent(q)}&api_key=${apiKey}&format=json&limit=50`;
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
