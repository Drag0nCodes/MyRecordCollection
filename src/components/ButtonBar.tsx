import { Box, Button } from "@mui/material";
import { useNavigate } from "react-router-dom";
import EditIcon from "@mui/icons-material/Edit";
import AddIcon from "@mui/icons-material/Add";
import SearchIcon from "@mui/icons-material/Search";
import DeleteIcon from "@mui/icons-material/Delete";

interface ButtonBarProps {
  onEditRecord?: () => void;
  onCreateRecord?: () => void;
  onDeleteRecord?: () => void;
  editEnabled?: boolean; // indicates a record is selected
}

export default function ButtonBar({
  onEditRecord,
  onCreateRecord,
  onDeleteRecord,
  editEnabled,
}: ButtonBarProps) {
  const navigate = useNavigate();
  return (
    <Box
      sx={{
        display: "flex",
        alignItems: "center",
      }}
    >
      <Box sx={{ display: "flex", gap: 1 }}>
        <Button
          variant="contained"
          color="primary"
          size="small"
          sx={{ fontWeight: 700 }}
          startIcon={<SearchIcon />}
          onClick={() => navigate("/findrecord")}
        >
          Find New
        </Button>
        <Button
          variant="outlined"
          onClick={onCreateRecord}
          size="small"
          sx={{ fontWeight: 700 }}
          startIcon={<AddIcon />}
        >
          Create
        </Button>
        <Button
          variant="outlined"
          color="primary"
          onClick={onEditRecord}
          disabled={!editEnabled}
          size="small"
          sx={{ fontWeight: 700 }}
          startIcon={<EditIcon />}
        >
          Edit
        </Button>
        <Button
          variant="outlined"
          color="error"
          onClick={onDeleteRecord}
          disabled={!editEnabled}
          size="small"
          sx={{ fontWeight: 700 }}
          startIcon={<DeleteIcon />}
        >
          Delete
        </Button>
      </Box>
    </Box>
  );
}
