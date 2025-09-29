import React, { useState } from "react";
import {
  Box,
  Typography,
  TextField,
  IconButton,
  Menu,
  MenuItem,
  Grid,
} from "@mui/material";
import AccountCircle from "@mui/icons-material/AccountCircle";

interface TopBarProps {
  onSearchChange: (value: string) => void;
  onLogout?: () => void;
  title: string;
  username?: string;
  /** When set to 'submit', only fire onSearchChange when user presses Enter */
  searchMode?: "change" | "submit";
  /** Optional placeholder override */
  searchPlaceholder?: string;
}

export default function TopBar({
  onSearchChange,
  onLogout,
  title,
  username,
  searchMode = "change",
  searchPlaceholder,
}: TopBarProps) {
  const [anchorEl, setAnchorEl] = useState<null | HTMLElement>(null);
  const open = Boolean(anchorEl);
  const [text, setText] = useState("");

  const handleSearchChange = (event: React.ChangeEvent<HTMLInputElement>) => {
    const v = event.target.value;
    setText(v);
    if (searchMode === "change") {
      onSearchChange(v);
    }
  };

  const handleKeyDown = (e: React.KeyboardEvent<HTMLInputElement>) => {
    if (searchMode === "submit" && e.key === "Enter") {
      e.preventDefault();
      onSearchChange(text.trim());
    }
  };

  const handleMenuOpen = (event: React.MouseEvent<HTMLElement>) => {
    setAnchorEl(event.currentTarget);
  };

  const handleMenuClose = () => {
    setAnchorEl(null);
  };

  const handleLogoutClick = () => {
    if (onLogout) {
      onLogout();
    }
    handleMenuClose();
  };

  return (
    <Grid>
      <Box sx={{ display: "flex", alignItems: "center", mb: 1, gap: 1 }}>
        <Typography
          variant="h4"
          sx={{
            mr: "auto",
            fontWeight: "bold",
            whiteSpace: "nowrap", // Prevents text from wrapping
          }}
        >
          {title}
        </Typography>
        <TextField
          variant="outlined"
          placeholder={searchPlaceholder || "Search My Collection"}
          sx={{ width: 300 }}
          value={text}
          onChange={handleSearchChange}
          onKeyDown={handleKeyDown}
        />
        {username && (
          <Box sx={{ mx: -1 }}>
            <IconButton
              size="large"
              aria-label="account of current user"
              aria-haspopup="true"
              onMouseEnter={handleMenuOpen} // Open menu on hover
              color="inherit"
            >
              <AccountCircle fontSize="large" />
            </IconButton>
            <Menu
              anchorEl={anchorEl}
              anchorOrigin={{
                vertical: "bottom",
                horizontal: "right",
              }}
              transformOrigin={{
                vertical: "top",
                horizontal: "right",
              }}
              open={open}
              onClose={handleMenuClose}
              slotProps={{
                paper: { onMouseLeave: handleMenuClose },
              }}
            >
              <MenuItem
                disabled
                sx={{
                  // Target the disabled state and increase its opacity
                  "&.Mui-disabled": {
                    opacity: 0.75, // The default is around 0.38
                  },
                }}
              >
                User: {username}
              </MenuItem>
              {onLogout && (
                <MenuItem onClick={handleLogoutClick}>Logout</MenuItem>
              )}
            </Menu>
          </Box>
        )}
      </Box>
    </Grid>
  );
}
