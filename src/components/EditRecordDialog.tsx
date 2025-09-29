import React, { useState, useEffect } from "react";
import {
  Dialog,
  DialogTitle,
  DialogContent,
  DialogActions,
  Button,
  Grid,
  Box,
  TextField,
  Slider,
  Chip,
  Autocomplete,
  Typography,
} from "@mui/material";
import { type Record } from "../types"; // Assuming types.ts is in the parent directory

// --- COMPONENT PROPS ---
interface EditRecordDialogProps {
  open: boolean;
  onClose: () => void;
  onSave: (record: Record) => void;
  record: Record | null; // The record to edit, or null if none
  tagOptions?: string[]; // existing tags to suggest
}

// --- MAIN COMPONENT ---
export default function EditRecordDialog({
  open,
  onClose,
  onSave,
  record,
  tagOptions = [],
}: EditRecordDialogProps) {
  const [editedRecord, setEditedRecord] = useState<Record | null>(null);
  const [imageUrl, setImageUrl] = useState("");

  // When the dialog opens or the record prop changes, reset the internal state
  useEffect(() => {
    if (record) {
      setEditedRecord(record);
      setImageUrl(record.cover || "");
    }
  }, [record, open]);

  // Generic handler for simple text field changes
  const handleChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const { name, value } = event.target;
    if (editedRecord) {
      setEditedRecord({
        ...editedRecord,
        [name]: name === "release" || name === "rating" ? Number(value) : value,
      });
    }
  };

  // Handler for the image URL input
  const handleImageUrlChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    setImageUrl(event.target.value);
    if (editedRecord) {
      setEditedRecord({
        ...editedRecord,
        cover: event.target.value,
      });
    }
  };

  // Handler for the Autocomplete tags input
  const handleTagsChange = (_: any, newValue: string[]) => {
    if (editedRecord) {
      setEditedRecord({ ...editedRecord, tags: newValue });
    }
  };

  // Handler for the Slider rating input
  const handleRatingChange = (_: Event, newValue: number | number[]) => {
    if (editedRecord) {
      setEditedRecord({ ...editedRecord, rating: newValue as number });
    }
  };

  const handleSaveChanges = () => {
    if (editedRecord) {
      onSave(editedRecord);
    }
  };

  // Prevent rendering if there's no record data yet
  if (!editedRecord) {
    return null;
  }

  return (
    <Dialog
      open={open}
      onClose={onClose}
      maxWidth="md"
      fullWidth
      PaperProps={{ sx: { bgcolor: "background.paper" } }}
    >
      <DialogTitle sx={{ bgcolor: "background.paper" }}>
        Record Settings
      </DialogTitle>
      <DialogContent sx={{ bgcolor: "background.paper" }}>
        <Grid container spacing={3} sx={{ mt: 1 }}>
          {/* Left Column: Cover Art */}
          <Grid size={{ xs: 8, sm: 4 }}>
            <Box
              sx={{
                width: "100%",
                paddingTop: "100%" /* Creates a square aspect ratio */,
                backgroundColor: "#333",
                backgroundImage: `url(${imageUrl})`,
                backgroundSize: "cover",
                backgroundPosition: "center",
                borderRadius: 2,
                border: "1px solid grey",
                mb: 2,
              }}
            />
            <TextField
              label="Image URL"
              fullWidth
              variant="outlined"
              value={imageUrl}
              onChange={handleImageUrlChange}
              size="small"
              sx={{
                "& .MuiOutlinedInput-root": {
                  backgroundColor: "background.paper",
                },
              }}
            />
          </Grid>

          {/* Right Column: Record Details */}
          <Grid size={{ xs: 12, sm: 8 }}>
            <TextField
              name="record"
              label="Record Title"
              fullWidth
              variant="outlined"
              size="small"
              value={editedRecord.record}
              onChange={handleChange}
              sx={{
                mb: 2,
                "& .MuiOutlinedInput-root": {
                  backgroundColor: "background.paper",
                },
              }}
            />
            <TextField
              name="artist"
              label="Artist"
              fullWidth
              variant="outlined"
              size="small"
              value={editedRecord.artist}
              onChange={handleChange}
              sx={{
                mb: 2,
                "& .MuiOutlinedInput-root": {
                  backgroundColor: "background.paper",
                },
              }}
            />
            <Grid container spacing={2}>
              <Grid size={{ xs: 6 }}>
                <TextField
                  name="release"
                  label="Release Year"
                  type="number"
                  fullWidth
                  variant="outlined"
                  value={editedRecord.release}
                  onChange={handleChange}
                  sx={{
                    "& .MuiOutlinedInput-root": {
                      backgroundColor: "background.paper",
                    },
                  }}
                />
              </Grid>
              <Grid size={{ xs: 6 }}>
                <Box
                  sx={{
                    display: "flex",
                    alignItems: "center",
                    gap: 2,
                    pt: 0.5,
                    ml: -0.5,
                  }}
                >
                  <Typography
                    variant="body2"
                    sx={{ whiteSpace: "nowrap", fontWeight: 500 }}
                  >
                    Rating
                  </Typography>
                  <Slider
                    name="rating"
                    value={editedRecord.rating}
                    onChange={handleRatingChange}
                    aria-label="rating"
                    valueLabelDisplay="auto"
                    step={1}
                    marks
                    min={0}
                    max={10}
                    sx={{ flex: 1, p: "auto" }}
                  />
                </Box>
              </Grid>
            </Grid>
            <Autocomplete
              multiple
              freeSolo // Allows adding new tags not in the options list
              options={tagOptions} // Use provided existing tags for suggestions
              filterSelectedOptions
              value={editedRecord.tags}
              onChange={handleTagsChange}
              size="small"
              fullWidth
              sx={{
                mt: 2,
                // allow chips to wrap so the input height expands when many tags are present
                "& .MuiAutocomplete-inputRoot": {
                  flexWrap: "wrap",
                  gap: "4px",
                },
                "& .MuiChip-root": { margin: "0px" },
                // ensure the text input doesn't expand to push chips apart
                "& .MuiAutocomplete-input": { minWidth: 120 },
                // make the Autocomplete input blend with the dialog background
                "& .MuiOutlinedInput-root": {
                  backgroundColor: "background.paper",
                },
              }}
              renderTags={(value: readonly string[], getTagProps) =>
                value.map((option: string, index: number) => {
                  const tagProps = getTagProps({ index });
                  return (
                    <Chip variant="outlined" label={option} {...tagProps} />
                  );
                })
              }
              renderInput={(params) => (
                <TextField
                  {...params}
                  variant="outlined"
                  label="Tags"
                  placeholder="Add tags"
                  sx={{
                    "& .MuiOutlinedInput-root": {
                      backgroundColor: "background.paper",
                    },
                  }}
                />
              )}
            />
          </Grid>
        </Grid>
      </DialogContent>
      <DialogActions sx={{ p: 2, bgcolor: "background.paper" }}>
        <Button onClick={onClose} color="inherit" sx={{ fontWeight: 700 }}>
          Cancel
        </Button>
        <Button
          onClick={handleSaveChanges}
          variant="contained"
          color="primary"
          sx={{ fontWeight: 700 }}
        >
          Save
        </Button>
      </DialogActions>
    </Dialog>
  );
}
