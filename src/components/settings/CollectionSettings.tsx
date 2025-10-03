import {
  type ChangeEvent,
  useCallback,
  useMemo,
  useRef,
  useState,
} from "react";
import {
  Alert,
  Box,
  Button,
  Chip,
  Dialog,
  DialogActions,
  DialogContent,
  DialogContentText,
  DialogTitle,
  Divider,
  FormControlLabel,
  LinearProgress,
  List,
  ListItem,
  ListItemText,
  Paper,
  Snackbar,
  Stack,
  Switch,
  Typography,
} from "@mui/material";
import CloudUploadIcon from "@mui/icons-material/CloudUpload";
import RestartAltIcon from "@mui/icons-material/RestartAlt";
import DeleteForeverIcon from "@mui/icons-material/DeleteForever";
import LocalOfferIcon from "@mui/icons-material/LocalOffer";
import Papa, { type ParseResult } from "papaparse";
import apiUrl from "../../api";
import { wikiGenres } from "../../wiki";

const DEFAULT_COLLECTION = "My Collection";
const MIN_RELEASE_YEAR = 1877;
const MAX_RELEASE_YEAR = 2100;

interface DiscogsCsvRow {
  [key: string]: string | undefined;
}

interface ParsedDiscogsRecord {
  artist: string;
  record: string;
  rating: number;
  release: number;
  rawDateAdded: string | null;
}

interface ImportResult {
  created: number;
  skipped: number;
  withoutCover: number;
}

function normalizeArtist(raw: string): string {
  const trimmed = raw.trim();
  if (trimmed.endsWith(")")) {
    const start = trimmed.lastIndexOf("(");
    const end = trimmed.lastIndexOf(")");
    if (start !== -1 && end === trimmed.length - 1) {
      return trimmed.slice(0, start).trim();
    }
  }
  return trimmed;
}

function pickField(row: DiscogsCsvRow, candidates: string[]): string | "" {
  for (const key of candidates) {
    const value = row[key];
    if (typeof value === "string" && value.trim()) {
      return value.trim();
    }
  }
  return "";
}

function parseDiscogsRows(rows: DiscogsCsvRow[]): ParsedDiscogsRecord[] {
  const parsed: ParsedDiscogsRecord[] = [];
  for (const row of rows) {
    const artistRaw = pickField(row, ["Artist", "artist"]);
    const titleRaw = pickField(row, ["Title", "title", "Release Title"]);
    if (!artistRaw || !titleRaw) continue;

    const ratingRaw = pickField(row, ["Rating", "rating"]);
    const releaseRaw = pickField(row, ["Released", "released", "Year", "year"]);
    const rawDate = pickField(row, [
      "Date Added",
      "Added",
      "Collection Date Added",
    ]);

    let rating = 0;
    if (ratingRaw) {
      const numeric = Number(ratingRaw);
      if (!Number.isNaN(numeric) && numeric >= 0) {
        rating = Math.round(Math.min(5, numeric) * 2);
      }
    }

    let release = Number.parseInt(releaseRaw, 10);
    if (
      !Number.isInteger(release) ||
      release < MIN_RELEASE_YEAR ||
      release > MAX_RELEASE_YEAR
    ) {
      release = Math.min(
        Math.max(new Date().getFullYear(), MIN_RELEASE_YEAR),
        MAX_RELEASE_YEAR
      );
    }

    let normalizedDate: string | null = null;
    if (rawDate) {
      const match = rawDate.match(/\d{4}-\d{2}-\d{2}/);
      if (match) {
        normalizedDate = match[0];
      } else {
        const parsedDate = new Date(rawDate);
        if (!Number.isNaN(parsedDate.getTime())) {
          normalizedDate = parsedDate.toISOString().slice(0, 10);
        }
      }
    }

    parsed.push({
      artist: normalizeArtist(artistRaw),
      record: titleRaw.trim(),
      rating,
      release,
      rawDateAdded: normalizedDate,
    });
  }
  return parsed;
}

export default function CollectionSettings() {
  const fileInputRef = useRef<HTMLInputElement | null>(null);
  const [fileName, setFileName] = useState<string | null>(null);
  const [parsedRecords, setParsedRecords] = useState<ParsedDiscogsRecord[]>([]);
  const [parseError, setParseError] = useState<string | null>(null);
  const [includeWikiTags, setIncludeWikiTags] = useState<boolean>(false);
  const [useDateAdded, setUseDateAdded] = useState<boolean>(true);
  const [importing, setImporting] = useState<boolean>(false);
  const [tagProgress, setTagProgress] = useState<number>(0);
  const [submittingRecords, setSubmittingRecords] = useState<boolean>(false);
  const [snackbar, setSnackbar] = useState<{
    open: boolean;
    message: string;
    severity: "success" | "error" | "info";
  }>({ open: false, message: "", severity: "success" });
  const [importSummary, setImportSummary] = useState<ImportResult | null>(null);
  const [clearingCollection, setClearingCollection] = useState(false);
  const [clearingTags, setClearingTags] = useState(false);
  const [clearDialogOpen, setClearDialogOpen] = useState(false);
  const [clearDialogType, setClearDialogType] = useState<
    "collection" | "tags" | "wishlist" | null
  >(null);
  const [clearingWishlist, setClearingWishlist] = useState(false);

  const sampleRecords = useMemo(
    () => parsedRecords.slice(0, 5),
    [parsedRecords]
  );

  const handleFileParsed = useCallback((rows: DiscogsCsvRow[]) => {
    const parsed = parseDiscogsRows(rows);
    setParsedRecords(parsed);
    setImportSummary(null);
    if (parsed.length === 0) {
      setParseError("No valid records were found in the selected CSV file.");
    } else {
      setParseError(null);
      setSnackbar({
        open: true,
        message: `Loaded ${parsed.length} records from Discogs CSV`,
        severity: "info",
      });
    }
  }, []);

  const handleFileChange = useCallback(
    (event: ChangeEvent<HTMLInputElement>) => {
      const file = event.target.files?.[0];
      if (!file) return;
      setFileName(file.name);
      setParseError(null);
      Papa.parse<DiscogsCsvRow>(file, {
        header: true,
        skipEmptyLines: true,
        complete: (results: ParseResult<DiscogsCsvRow>) => {
          if (results.errors && results.errors.length > 0) {
            setParseError("Failed to parse CSV file. Please check the format.");
            setParsedRecords([]);
            return;
          }
          handleFileParsed(results.data ?? []);
        },
        error: () => {
          setParseError("Failed to read CSV file. Please try again.");
          setParsedRecords([]);
        },
      });
    },
    [handleFileParsed]
  );

  const resetSelection = () => {
    setFileName(null);
    setParsedRecords([]);
    setParseError(null);
    setImportSummary(null);
    if (fileInputRef.current) {
      fileInputRef.current.value = "";
    }
  };

  const handleImport = async () => {
    if (parsedRecords.length === 0 || importing) return;
    setImporting(true);
    setImportSummary(null);
    setTagProgress(0);

    const payloadRecords = [];
    for (let i = 0; i < parsedRecords.length; i += 1) {
      const record = parsedRecords[i];
      let tags: string[] = [];
      if (includeWikiTags) {
        try {
          const fetched = await wikiGenres(record.record, record.artist, false);
          if (Array.isArray(fetched) && fetched.length > 0) {
            const unique = Array.from(
              new Set(fetched.map((tag) => tag.trim()).filter(Boolean))
            );
            tags = unique.slice(0, 12);
          }
        } catch {
          // ignore tag fetch failures
        }
        setTagProgress(Math.round(((i + 1) / parsedRecords.length) * 100));
      }

      payloadRecords.push({
        record: record.record,
        artist: record.artist,
        rating: record.rating,
        release: record.release,
        tags,
        dateAdded: useDateAdded ? record.rawDateAdded : null,
      });
    }

    try {
      // Indicate we are now sending the assembled records to the server
      setSubmittingRecords(true);
      const res = await fetch(apiUrl("/api/import/discogs"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify({
          tableName: DEFAULT_COLLECTION,
          records: payloadRecords,
        }),
      });

      const data = await res.json().catch(() => ({}));
      if (!res.ok) {
        const message = data?.error || "Import failed";
        setSnackbar({ open: true, message, severity: "error" });
        return;
      }

      const summary: ImportResult = {
        created: Number(data.created) || 0,
        skipped: Number(data.skipped) || 0,
        withoutCover: Number(data.withoutCover) || 0,
      };
      setImportSummary(summary);
      const parts = [
        `${summary.created} added`,
        summary.skipped ? `${summary.skipped} skipped` : null,
        summary.withoutCover
          ? `${summary.withoutCover} without cover art`
          : null,
      ].filter(Boolean);
      setSnackbar({
        open: true,
        message: `Import complete: ${parts.join(", ")}`,
        severity: "success",
      });
    } catch {
      setSnackbar({
        open: true,
        message: "Network error during import",
        severity: "error",
      });
    } finally {
      setImporting(false);
      setTagProgress(0);
      setSubmittingRecords(false);
    }
  };

  return (
    <Box display="flex" flexDirection="column" gap={3}>
      <Box>
        <Typography variant="h5" gutterBottom>
          Collection Settings
        </Typography>
        <Typography variant="body2" color="text.secondary">
          Import your Discogs collection directly into{" "}
          <strong>{DEFAULT_COLLECTION}</strong>. Choose your Discogs CSV export,
          optionally enrich records with Wikipedia tags, and let the app add
          everything in one go.
        </Typography>
      </Box>

      <Paper
        variant="outlined"
        sx={{ p: 3, borderRadius: 2, backgroundColor: "background.paper" }}
      >
        <Stack direction="column" spacing={2} alignItems="stretch">
          <Box sx={{ display: "flex", gap: 2, alignItems: "center" }}>
            <Button
              variant="contained"
              startIcon={<CloudUploadIcon />}
              component="label"
              disabled={importing}
              sx={{ mb: 0 }}
            >
              {fileName ? "Replace CSV" : "Select Discogs CSV"}
              <input
                hidden
                ref={fileInputRef}
                type="file"
                accept=".csv,text/csv"
                onChange={handleFileChange}
              />
            </Button>
            {fileName && (
              <Chip
                label={fileName}
                onDelete={importing ? undefined : resetSelection}
                deleteIcon={<RestartAltIcon />}
                color="primary"
                variant="outlined"
                sx={{ maxWidth: "100%" }}
              />
            )}
          </Box>

          <Box>
            <FormControlLabel
              control={
                <Switch
                  color="primary"
                  checked={includeWikiTags}
                  onChange={(_, checked) => setIncludeWikiTags(checked)}
                  disabled={importing}
                />
              }
              label="Add suggested tags from record genres"
              sx={{ mb: -2 }}
            />
          </Box>

          <Box>
            <FormControlLabel
              control={
                <Switch
                  color="primary"
                  checked={useDateAdded}
                  onChange={(_, checked) => setUseDateAdded(checked)}
                  disabled={importing}
                />
              }
              label="Import Discogs 'Date Added' values"
            />
          </Box>

          <Box>
            <Button
              variant="contained"
              color="success"
              onClick={handleImport}
              disabled={importing || parsedRecords.length === 0}
            >
              {importing
                ? "Importing..."
                : `Import ${parsedRecords.length || "0"} records`}
            </Button>
          </Box>
        </Stack>

        {importing && (
          <Box sx={{ mt: 2 }}>
            <LinearProgress
              variant={
                includeWikiTags && !submittingRecords
                  ? "determinate"
                  : "indeterminate"
              }
              value={
                includeWikiTags && !submittingRecords ? tagProgress : undefined
              }
            />
            <Typography variant="caption" display="block" sx={{ mt: 1 }}>
              {includeWikiTags && !submittingRecords
                ? `Fetching wiki tags... ${tagProgress}%`
                : "Submitting records to the server..."}
            </Typography>
          </Box>
        )}

        {parseError && (
          <Alert severity="error" sx={{ mt: 2 }}>
            {parseError}
          </Alert>
        )}

        {parsedRecords.length > 0 && (
          <Box sx={{ mt: 3 }}>
            <Divider sx={{ mb: 2 }} />
            <Typography variant="subtitle1" gutterBottom>
              Preview ({parsedRecords.length} record
              {parsedRecords.length === 1 ? "" : "s"} found)
            </Typography>
            <List dense disablePadding>
              {sampleRecords.map((rec, idx) => (
                <ListItem
                  key={`${rec.artist}-${rec.record}-${idx}`}
                  sx={{ py: 0.5 }}
                >
                  <ListItemText
                    primary={`${rec.artist} — ${rec.record}`}
                    secondary={`Year: ${rec.release} • Rating: ${rec.rating}${
                      useDateAdded && rec.rawDateAdded
                        ? ` • Added: ${rec.rawDateAdded}`
                        : ""
                    }`}
                  />
                </ListItem>
              ))}
            </List>
            {parsedRecords.length > sampleRecords.length && (
              <Typography variant="caption" color="text.secondary">
                Showing {sampleRecords.length} of {parsedRecords.length}{" "}
                entries.
              </Typography>
            )}
          </Box>
        )}

        {importSummary && (
          <Alert severity="success" sx={{ mt: 3 }}>
            Imported {importSummary.created} record
            {importSummary.created === 1 ? "" : "s"}.
            {importSummary.skipped
              ? ` Skipped ${importSummary.skipped} duplicate${
                  importSummary.skipped === 1 ? "" : "s"
                }.`
              : ""}
            {importSummary.withoutCover
              ? ` ${importSummary.withoutCover} record${
                  importSummary.withoutCover === 1 ? "" : "s"
                } missing cover art.`
              : ""}
          </Alert>
        )}
      </Paper>

      <Divider />

      <Box sx={{ display: "flex", gap: 2, flexWrap: "wrap" }}>
        <Button
          variant="contained"
          color="error"
          startIcon={<DeleteForeverIcon />}
          onClick={() => {
            setClearDialogType("collection");
            setClearDialogOpen(true);
          }}
          disabled={importing || clearingCollection}
        >
          {clearingCollection ? "Clearing..." : `Clear ${DEFAULT_COLLECTION}`}
        </Button>

        <Button
          variant="contained"
          color="error"
          startIcon={<DeleteForeverIcon />}
          onClick={() => {
            setClearDialogType("wishlist");
            setClearDialogOpen(true);
          }}
          disabled={importing || clearingWishlist}
        >
          {clearingWishlist ? "Clearing..." : "Clear Wishlist"}
        </Button>

        <Button
          variant="outlined"
          color="error"
          startIcon={<LocalOfferIcon />}
          onClick={() => {
            setClearDialogType("tags");
            setClearDialogOpen(true);
          }}
          disabled={importing || clearingTags}
        >
          {clearingTags ? "Clearing..." : "Clear Tags"}
        </Button>
      </Box>

      <Snackbar
        open={snackbar.open}
        autoHideDuration={5000}
        onClose={(_, reason) => {
          if (reason === "clickaway") return;
          setSnackbar((prev) => ({ ...prev, open: false }));
        }}
        anchorOrigin={{ vertical: "bottom", horizontal: "center" }}
      >
        <Alert
          severity={snackbar.severity}
          sx={{ width: "100%" }}
          onClose={() => setSnackbar((prev) => ({ ...prev, open: false }))}
        >
          {snackbar.message}
        </Alert>
      </Snackbar>
      <Dialog
        open={clearDialogOpen}
        onClose={() => {
          if (clearingCollection || clearingTags) return;
          setClearDialogOpen(false);
          setClearDialogType(null);
        }}
      >
        <DialogTitle sx={{ bgcolor: "background.paper" }}>
          {clearDialogType === "collection"
            ? "Delete Collection"
            : "Delete All Tags"}
        </DialogTitle>
        <DialogContent sx={{ bgcolor: "background.paper" }}>
          <DialogContentText>
            {clearDialogType === "collection"
              ? `Are you sure you want to permanently delete all records in '${DEFAULT_COLLECTION}'? This action cannot be undone.`
              : "Are you sure you want to permanently delete all your tags? This will also remove tag associations from records."}
          </DialogContentText>
        </DialogContent>
        <DialogActions sx={{ bgcolor: "background.paper" }}>
          <Button
            onClick={() => {
              setClearDialogOpen(false);
              setClearDialogType(null);
            }}
            disabled={clearingCollection || clearingTags}
            sx={{ fontWeight: 700 }}
          >
            Cancel
          </Button>
          <Button
            color="error"
            variant="contained"
            onClick={async () => {
              if (clearDialogType === "collection") {
                setClearingCollection(true);
                try {
                  const res = await fetch(apiUrl("/api/records/clear"), {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    credentials: "include",
                    body: JSON.stringify({ tableName: DEFAULT_COLLECTION }),
                  });
                  const data = await res.json().catch(() => ({}));
                  if (!res.ok) {
                    setSnackbar({
                      open: true,
                      message: data.error || "Failed to clear collection",
                      severity: "error",
                    });
                  } else {
                    setSnackbar({
                      open: true,
                      message: `Cleared ${
                        data.deleted || 0
                      } records from ${DEFAULT_COLLECTION}`,
                      severity: "success",
                    });
                    setParsedRecords([]);
                  }
                } catch (err) {
                  setSnackbar({
                    open: true,
                    message: "Network error clearing collection",
                    severity: "error",
                  });
                } finally {
                  setClearingCollection(false);
                  setClearDialogOpen(false);
                  setClearDialogType(null);
                }
              } else if (clearDialogType === "tags") {
                setClearingTags(true);
                try {
                  const res = await fetch(apiUrl("/api/tags/clear"), {
                    method: "POST",
                    credentials: "include",
                    headers: { "Content-Type": "application/json" },
                  });
                  const data = await res.json().catch(() => ({}));
                  if (!res.ok) {
                    setSnackbar({
                      open: true,
                      message: data.error || "Failed to clear tags",
                      severity: "error",
                    });
                  } else {
                    setSnackbar({
                      open: true,
                      message: `Deleted ${
                        data.tagsDeleted || 0
                      } tags (removed ${data.taggedDeleted || 0} tag links)`,
                      severity: "success",
                    });
                  }
                } catch (err) {
                  setSnackbar({
                    open: true,
                    message: "Network error clearing tags",
                    severity: "error",
                  });
                } finally {
                  setClearingTags(false);
                  setClearDialogOpen(false);
                  setClearDialogType(null);
                }
              } else if (clearDialogType === "wishlist") {
                setClearingWishlist(true);
                try {
                  const res = await fetch(apiUrl("/api/collection/clear"), {
                    method: "POST",
                    headers: { "Content-Type": "application/json" },
                    credentials: "include",
                    body: JSON.stringify({ tableName: "Wishlist" }),
                  });
                  const data = await res.json().catch(() => ({}));
                  if (!res.ok) {
                    setSnackbar({
                      open: true,
                      message: data.error || "Failed to clear wishlist",
                      severity: "error",
                    });
                  } else {
                    setSnackbar({
                      open: true,
                      message: `Cleared ${
                        data.deleted || 0
                      } records from Wishlist`,
                      severity: "success",
                    });
                  }
                } catch (err) {
                  setSnackbar({
                    open: true,
                    message: "Network error clearing wishlist",
                    severity: "error",
                  });
                } finally {
                  setClearingWishlist(false);
                  setClearDialogOpen(false);
                  setClearDialogType(null);
                }
              }
            }}
            disabled={clearingCollection || clearingTags || clearingWishlist}
            sx={{ fontWeight: 700 }}
          >
            {clearingCollection || clearingTags || clearingWishlist
              ? "Deleting..."
              : "Delete"}
          </Button>
        </DialogActions>
      </Dialog>
    </Box>
  );
}
