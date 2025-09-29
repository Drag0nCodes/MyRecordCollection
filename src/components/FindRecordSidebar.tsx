import {
  Paper,
  Typography,
  Box,
  List,
  ListItem,
  ListItemButton,
  ListItemIcon,
  Checkbox,
  ListItemText,
  TextField,
  Button,
  Slider,
} from "@mui/material";
import AddIcon from "@mui/icons-material/Add";

export interface AlbumListItem {
  id: string;
  cover?: string;
  record: string;
  artist: string;
}

interface FindRecordSidebarProps {
  availableTags: string[];
  selectedTags: string[];
  onToggleTag: (tag: string) => void;
  onAddNewTag: (tag: string) => void;
  rating: number;
  onRatingChange: (value: number) => void;
  releaseYear: number;
  onReleaseYearChange: (value: number) => void;
  canAdd: boolean;
  onAddRecord: () => void;
}

export default function FindRecordSidebar({
  availableTags,
  selectedTags,
  onToggleTag,
  onAddNewTag,
  rating,
  onRatingChange,
  releaseYear,
  onReleaseYearChange,
  canAdd,
  onAddRecord,
}: FindRecordSidebarProps) {
  const handleSlider = (_: Event, val: number | number[]) => {
    onRatingChange(val as number);
  };

  const handleAddTag = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (e.key === "Enter") {
      const target = e.target as HTMLInputElement;
      const val = target.value.trim();
      if (val && !availableTags.includes(val)) {
        onAddNewTag(val);
      }
      target.value = "";
    }
  };

  return (
    <Paper
      sx={{
        p: 2,
        display: "flex",
        flexDirection: "column",
        height: "100%",
        borderRadius: 2,
        overflow: "hidden",
      }}
    >
      <Box
        sx={{
          overflowY: "auto",
          pr: 1,
          flex: 1,
          minHeight: 0,
          display: "flex",
          flexDirection: "column",
        }}
      >
        <Typography variant="subtitle1">Add Tags</Typography>
        <Box
          sx={{
            flexGrow: 0,
            maxHeight: 300,
            overflowY: "auto",
            border: "1px solid grey",
            borderRadius: 2,
            my: 1,
          }}
        >
          <List dense>
            {availableTags.map((tag) => (
              <ListItem disablePadding key={tag}>
                <ListItemButton
                  dense
                  onClick={() => onToggleTag(tag)}
                  sx={{ py: 0 }}
                >
                  <ListItemIcon sx={{ minWidth: 35 }}>
                    <Checkbox
                      edge="start"
                      checked={selectedTags.includes(tag)}
                      tabIndex={-1}
                      disableRipple
                    />
                  </ListItemIcon>
                  <ListItemText primary={tag} />
                </ListItemButton>
              </ListItem>
            ))}
            {availableTags.length === 0 && (
              <ListItem>
                <ListItemText primary="No tags yet" />
              </ListItem>
            )}
          </List>
        </Box>
        <TextField
          placeholder="Add New Tag"
          size="small"
          onKeyDown={handleAddTag}
          sx={{ mb: 2 }}
        />
        <Typography variant="subtitle1" sx={{ mt: 1 }}>
          Rating
        </Typography>
        <Box sx={{ display: "flex", justifyContent: "center", width: "100%" }}>
          <Slider
            value={rating}
            onChange={handleSlider}
            valueLabelDisplay="auto"
            min={0}
            max={10}
            step={1}
            sx={{
              mx: 1.7,
              width: "90%",
              "& .MuiSlider-rail, & .MuiSlider-track": { height: 6 },
              height: 0,
              "& .MuiSlider-thumb": { width: 18, height: 18 },
            }}
          />
        </Box>
        <Typography variant="subtitle1" sx={{ mt: 1 }}>
          Release Year
        </Typography>
        <TextField
          value={releaseYear}
          type="number"
          size="small"
          onChange={(e) => onReleaseYearChange(Number(e.target.value))}
          sx={{ mb: 2, width: "60%" }}
          inputProps={{ min: 1877, max: 2100 }}
        />
      </Box>
      <Box sx={{ mt: 1 }}>
        <Button
          disabled={!canAdd}
          variant="contained"
          fullWidth
          onClick={onAddRecord}
          sx={{ fontWeight: 700 }}
          endIcon={<AddIcon />}
        >
          Add to Collection
        </Button>
      </Box>
    </Paper>
  );
}
