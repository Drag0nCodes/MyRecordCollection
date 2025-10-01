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

export default function Register() {
  const navigate = useNavigate();
  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [password2, setPassword2] = useState("");
  const [error, setError] = useState("");
  const [loading, setLoading] = useState(false);

  const handleSubmit = async (e: React.FormEvent) => {
    e.preventDefault();
    setError("");
    if (password !== password2) {
      setError("Passwords do not match");
      return;
    }
    setLoading(true);
    try {
      const res = await fetch(apiUrl("/api/register"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify({ username, password }),
      });
      const data = await res.json();
      if (data.success) {
        try {
          const meRes = await fetch(apiUrl("/api/me"), {
            credentials: "include",
          });
          if (meRes.ok) {
            const meJson = await meRes.json();
            const { setUserId } = await import("./analytics");
            setUserId(meJson.userUuid);
          }
        } catch {
          // ignore analytics failures
        }
        navigate("/mycollection");
      } else {
        setError(data.error || "Registration failed");
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
          <Typography variant="h4" fontWeight="bold" gutterBottom>
            Register
          </Typography>
          <Box
            component="form"
            onSubmit={handleSubmit}
            sx={{ width: "100%", maxWidth: 360 }}
          >
            <TextField
              label="Username"
              fullWidth
              margin="normal"
              size="small"
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
            <TextField
              label="Retype Password"
              type="password"
              fullWidth
              size="small"
              margin="normal"
              value={password2}
              onChange={(e) => setPassword2(e.target.value)}
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
              {loading ? "Registering..." : "Register"}
            </Button>
          </Box>
          <Button
            variant="text"
            sx={{ mt: 2 }}
            onClick={() => navigate("/login")}
          >
            Already have an account? Login
          </Button>
        </Box>
      </ThemeProvider>
    </>
  );
}
