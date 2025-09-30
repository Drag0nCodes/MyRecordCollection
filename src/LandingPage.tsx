import {
  Box,
  Typography,
  Button,
  Stack,
  ThemeProvider,
  CssBaseline,
} from "@mui/material";
import { useNavigate } from "react-router-dom";
import "./LandingPage.css";
import { darkTheme } from "./theme";
import icon from "./assets/icon.png";

export default function LandingPage() {
  const navigate = useNavigate();
  return (
    <ThemeProvider theme={darkTheme}>
      <CssBaseline />
      <Box
        sx={{
          px: 3,
          minHeight: "100vh",
          display: "flex",
          flexDirection: "column",
          alignItems: "center",
          justifyContent: "center",
        }}
      >
        <img
          src={icon}
          alt="My Record Collection Logo"
          width={150}
          height={150}
          className="record"
        />
        <Typography variant="h2" fontWeight="bold" gutterBottom pt={3}>
          Welcome to My Record Collection
        </Typography>
        <Typography variant="h5" color="text.secondary" gutterBottom>
          Organize, filter, and explore your music collection with ease.
        </Typography>
        <Stack spacing={2} sx={{ mt: 4 }}>
          <Button
            variant="contained"
            size="large"
            onClick={() => navigate("/mycollection")}
          >
            View My Collection
          </Button>
          <Button
            variant="outlined"
            size="large"
            onClick={() => navigate("/login")}
          >
            Login
          </Button>
          <Button
            variant="outlined"
            size="large"
            onClick={() => navigate("/register")}
          >
            Register
          </Button>
        </Stack>
      </Box>
    </ThemeProvider>
  );
}
