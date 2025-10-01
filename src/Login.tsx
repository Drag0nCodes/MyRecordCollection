import React, { useState } from "react";
import apiUrl from "./api";
import {
  Box,
  Typography,
  Button,
  ThemeProvider,
  CssBaseline,
  TextField,
  Alert,
} from "@mui/material";
import { useNavigate, Link } from "react-router-dom";
import { darkTheme } from "./theme";

export default function Login() {
  const navigate = useNavigate();
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setLoading(true);
    setError("");
    try {
      const res = await fetch(apiUrl("/api/login"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify({ username, password }),
      });
      const data = await res.json();
      if (data.success) {
        try {
          // fetch /api/me to get the userUuid (for analytics)
          const meRes = await fetch(apiUrl("/api/me"), {
            credentials: "include",
          });
          if (meRes.ok) {
            const meJson = await meRes.json();
            // dynamically import analytics to avoid SSR issues
            const { setUserId } = await import("./analytics");
            setUserId(meJson.userUuid);
          }
        } catch {
          // ignore analytics errors
        }
        navigate("/mycollection");
      } else {
        setError(data.error || "Login failed");
      }
    } catch {
      setError("Network error");
    }
    setLoading(false);
  };

  return (
    <>
      <style>
        {
          "@import url('https://fonts.googleapis.com/css2?family=Bungee&family=Saira+Extra+Condensed:wght@100;200;300;400;500;600;700;800;900&display=swap');"
        }
      </style>
      <ThemeProvider theme={darkTheme}>
        <CssBaseline />
        <Box
          sx={{
            p: 4,
            minHeight: "100vh",
            display: "flex",
            flexDirection: "column",
            alignItems: "center",
            justifyContent: "center",
          }}
        >
          <div
            style={{
              position: "fixed",
              top: 0,
              left: 0,
              right: 0,
              zIndex: 1400,
              borderBottom: "2px solid #555",
              background: "transparent",
              paddingLeft: 12,
              paddingTop: 10,
              paddingBottom: 10,
            }}
          >
            <Link to="/" style={{ textDecoration: "none", color: "#fff" }}>
              <Typography
                variant="h5"
                sx={{
                  fontFamily: "Bungee, sans-serif",
                  mb: 0,
                  cursor: "pointer",
                  color: "#fff",
                }}
              >
                My Record Collection
              </Typography>
            </Link>
          </div>
          <Typography variant="h4" fontWeight="bold" gutterBottom>
            Login
          </Typography>
          <Box
            component="form"
            onSubmit={handleSubmit}
            sx={{ width: "100%", maxWidth: 360 }}
          >
            <TextField
              label="Username"
              fullWidth
              size="small"
              margin="normal"
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              required
            />
            <TextField
              label="Password"
              type="password"
              fullWidth
              size="small"
              margin="normal"
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              required
            />
            {error && (
              <Alert severity="error" sx={{ mt: 2 }}>
                {error}
              </Alert>
            )}
            <Button
              type="submit"
              variant="contained"
              color="primary"
              fullWidth
              sx={{ mt: 2 }}
              disabled={loading}
            >
              {loading ? "Logging in..." : "Login"}
            </Button>
          </Box>
          <Button
            variant="text"
            sx={{ mt: 2 }}
            onClick={() => navigate("/register")}
          >
            Need an account? Register
          </Button>
        </Box>
      </ThemeProvider>
    </>
  );
}
