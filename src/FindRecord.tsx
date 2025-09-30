import { useState, useEffect, useMemo } from "react";
import apiUrl from "./api";
import {
  ThemeProvider,
  CssBaseline,
  Box,
  Button,
  CircularProgress,
  Alert,
} from "@mui/material";
import Grid from "@mui/material/Grid";
import { darkTheme } from "./theme";
import TopBar from "./components/TopBar";
import { useNavigate } from "react-router-dom";
import { setUserId } from "./analytics";
import { DataGrid, type GridColDef } from "@mui/x-data-grid";
import placeholderCover from "./assets/missingImg.jpg";
import FindRecordSidebar, {
  type AlbumListItem,
} from "./components/FindRecordSidebar";
import LibraryMusicIcon from "@mui/icons-material/LibraryMusic";

interface AlbumResult {
  name: string;
  artist: string;
  url: string;
  listeners?: string;
  image?: { ["#text"]: string; size: string }[];
}

export default function FindRecord() {
  const [results, setResults] = useState<AlbumResult[]>([]);
  const [username, setUsername] = useState<string>("");
  const [selectedAlbumId, setSelectedAlbumId] = useState<string | undefined>(
    undefined
  );
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  // Sidebar / add-to-collection state
  const [availableTags, setAvailableTags] = useState<string[]>([]);
  const [selectedTags, setSelectedTags] = useState<string[]>([]);
  const [rating, setRating] = useState<number>(0);
  const [releaseYear, setReleaseYear] = useState<number>(
    new Date().getFullYear()
  );
  const [adding, setAdding] = useState(false);
  const [addError, setAddError] = useState<string | null>(null);
  const navigate = useNavigate();

  useEffect(() => {
    const fetchBasic = async () => {
      try {
        const [meRes, tagsRes] = await Promise.all([
          fetch(apiUrl("/api/me"), { credentials: "include" }),
          fetch(apiUrl("/api/tags"), { credentials: "include" }),
        ]);
        if (meRes.ok) {
          const data = await meRes.json();
          setUsername(data.username);
        }
        if (tagsRes.ok) {
          const tagJson = await tagsRes.json();
          setAvailableTags(tagJson);
        }
      } catch {
        /* ignore */
      }
    };
    fetchBasic();
  }, []);

  const handleLogout = async () => {
    await fetch(apiUrl("/api/logout"), {
      method: "POST",
      credentials: "include",
    });
    try {
      setUserId(undefined);
    } catch {}
    navigate("/login");
  };

  const handleSearchSubmit = async (value: string) => {
    if (!value.trim()) {
      setResults([]);
      setError(null);
      return;
    }
    setLoading(true);
    setError(null);
    try {
      const res = await fetch(
        apiUrl(`/api/lastfm/album.search?q=${encodeURIComponent(value)}`),
        { credentials: "include" }
      );
      if (res.ok) {
        const data = await res.json();
        const albums = data?.results?.albummatches?.album || [];
        setResults(albums);
      } else {
        const problem = await res.json().catch(() => ({}));
        setError(problem.error || `Search failed (${res.status})`);
      }
    } catch (e) {
      console.error(e);
      setError("Network error searching Last.fm");
    } finally {
      setLoading(false);
    }
  };

  const rows = useMemo(() => {
    return results.map((a, idx) => {
      // Prefer largest available image (extralarge/mega) else fall back to last non-empty, else first
      const nonEmpty = (a.image || []).filter((im) => im["#text"]);
      const largePref =
        nonEmpty.find((im) => im.size === "extralarge") ||
        nonEmpty.find((im) => im.size === "mega") ||
        nonEmpty[nonEmpty.length - 1] ||
        nonEmpty[0];
      const cover = largePref?.["#text"] || "";
      return {
        id: `${a.name}-${a.artist}-${idx}`,
        cover,
        record: a.name,
        artist: a.artist,
      } as AlbumListItem & { record: string };
    });
  }, [results]);

  // When selecting a different album reset metadata inputs (except tags maybe keep?) We'll keep chosen tags/rating for convenience only per selection? Probably reset.
  useEffect(() => {
    if (selectedAlbumId) {
      setRating(0);
      setReleaseYear(new Date().getFullYear());
      setSelectedTags([]);
      setAddError(null);
    }
  }, [selectedAlbumId]);

  const handleToggleTag = (tag: string) => {
    setSelectedTags((prev) =>
      prev.includes(tag) ? prev.filter((t) => t !== tag) : [...prev, tag]
    );
  };

  const handleAddNewTag = (tag: string) => {
    setAvailableTags((prev) => (prev.includes(tag) ? prev : [...prev, tag]));
    setSelectedTags((prev) => (prev.includes(tag) ? prev : [...prev, tag]));
  };

  const handleAddRecord = async () => {
    if (!selectedAlbumId) return;
    const selectedRow = rows.find((r) => r.id === selectedAlbumId);
    if (!selectedRow) return;
    setAdding(true);
    setAddError(null);
    try {
      const payload = {
        id: -1,
        cover: selectedRow.cover,
        record: selectedRow.record,
        artist: selectedRow.artist,
        rating,
        tags: selectedTags,
        release: releaseYear,
        dateAdded: new Date().toISOString().slice(0, 10),
      };
      const res = await fetch(apiUrl("/api/records/create"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify(payload),
      });
      if (res.ok) {
        // Navigate back to collection after successful add
        navigate("/mycollection");
      } else {
        const problem = await res.json().catch(() => ({}));
        setAddError(problem.error || `Failed to add record (${res.status})`);
      }
    } catch (e) {
      setAddError("Network error adding record");
    } finally {
      setAdding(false);
    }
  };

  const columns: GridColDef[] = [
    {
      field: "cover",
      headerName: "",
      width: 110,
      sortable: false,
      renderCell: (params) => {
        const src = params.value || placeholderCover;
        const title = params.row.record ?? "cover";
        return (
          <img
            src={src}
            alt={title}
            style={{
              maxWidth: 100,
              maxHeight: 100,
              objectFit: "cover",
              borderRadius: 4,
            }}
          />
        );
      },
    },
    {
      field: "record",
      headerName: "Album",
      flex: 1.5,
      minWidth: 100,
      cellClassName: "wrapCell",
      renderCell: (params) => (
        <div className="wrapText" style={{ width: "100%" }}>
          {params.value}
        </div>
      ),
    },
    {
      field: "artist",
      headerName: "Artist",
      flex: 1,
      minWidth: 100,
      cellClassName: "wrapCell",
      renderCell: (params) => (
        <div className="wrapText" style={{ width: "100%" }}>
          {params.value}
        </div>
      ),
    },
  ];

  return (
    <ThemeProvider theme={darkTheme}>
      <CssBaseline />
      <Box
        sx={{
          p: 1.5,
          height: "100vh",
          display: "flex",
          flexDirection: "column",
        }}
      >
        <TopBar
          title="Find Record"
          onSearchChange={handleSearchSubmit}
          onLogout={handleLogout}
          username={username}
          searchMode="submit"
          searchPlaceholder="Search All Albums (By Title)"
        />
        <Box
          sx={{
            flex: "0 0 auto",
            textAlign: "left",
          }}
        >
          <Button
            variant="contained"
            size="small"
            onClick={() => navigate("/mycollection")}
            sx={{ fontWeight: 700 }}
            startIcon={<LibraryMusicIcon />}
          >
            My Collection
          </Button>
        </Box>
        <Grid
          container
          spacing={2}
          sx={{ flex: "1 1 0", minHeight: 0, overflow: "hidden" }}
        >
          <Grid
            size={{ xs: 7, sm: 8, md: 9 }}
            height={"100%"}
            sx={{ display: "flex", flexDirection: "column", minHeight: 0 }}
          >
            {error && (
              <Alert severity="error" sx={{ mb: 1 }}>
                {error}
              </Alert>
            )}
            <Box sx={{ flex: 1, minHeight: 0, position: "relative" }}>
              <DataGrid
                rows={rows}
                columns={columns}
                density="comfortable"
                hideFooter
                rowHeight={90}
                onRowClick={(p) => setSelectedAlbumId(p.id as string)}
                getRowClassName={(params) =>
                  params.id === selectedAlbumId ? "selected-row" : ""
                }
                sx={{
                  border: "none",
                  height: "100%",
                  "& .MuiDataGrid-cell": {
                    display: "flex",
                    alignItems: "center",
                    py: 1,
                    minWidth: 0,
                  },
                  "& .wrapCell .MuiDataGrid-cellContent": {
                    whiteSpace: "normal",
                    overflow: "hidden",
                    textOverflow: "clip",
                    overflowWrap: "anywhere",
                    lineHeight: 1.2,
                    display: "block",
                  },
                  "& .wrapCell": {
                    whiteSpace: "normal !important",
                  },
                  "& .wrapCell .wrapText": {
                    whiteSpace: "normal",
                    overflowWrap: "anywhere",
                    wordBreak: "break-word",
                    lineHeight: 1.2,
                    alignSelf: "center",
                  },
                  "& .selected-row": {
                    bgcolor: (theme) =>
                      `${theme.palette.action.selected} !important`,
                  },
                }}
              />
              {loading && (
                <Box
                  sx={{
                    position: "absolute",
                    inset: 0,
                    display: "flex",
                    alignItems: "center",
                    justifyContent: "center",
                    bgcolor: "rgba(0,0,0,0.35)",
                  }}
                >
                  <CircularProgress />
                </Box>
              )}
            </Box>
          </Grid>
          <Grid
            size={{ xs: 5, sm: 4, md: 3 }}
            sx={{ height: "100%", minHeight: 0 }}
          >
            <FindRecordSidebar
              availableTags={availableTags}
              selectedTags={selectedTags}
              onToggleTag={handleToggleTag}
              onAddNewTag={handleAddNewTag}
              rating={rating}
              onRatingChange={setRating}
              releaseYear={releaseYear}
              onReleaseYearChange={setReleaseYear}
              canAdd={!!selectedAlbumId && !adding}
              onAddRecord={handleAddRecord}
            />
            {addError && (
              <Alert severity="error" sx={{ mt: 1 }}>
                {addError}
              </Alert>
            )}
          </Grid>
        </Grid>
      </Box>
    </ThemeProvider>
  );
}
