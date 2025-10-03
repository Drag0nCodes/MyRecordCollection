import { useEffect, useMemo, useState } from "react";
import apiUrl from "../../api";
import {
  Box,
  Typography,
  TextField,
  Button,
  Stack,
  Divider,
  Alert,
  Snackbar,
  InputAdornment,
  IconButton,
} from "@mui/material";
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";

interface ProfileSettingsProps {
  username: string;
  displayName: string;
  onProfileUpdated?: (user: { username: string; displayName: string }) => void;
}

const usernameRegex = /^[a-zA-Z0-9_]+$/;
const passwordRegex = /(?=.*[A-Za-z])(?=.*\d)(?=.*[^A-Za-z0-9])/;

export default function ProfileSettings({
  username,
  displayName,
  onProfileUpdated,
}: ProfileSettingsProps) {
  const [usernameValue, setUsernameValue] = useState(username);
  const [displayNameValue, setDisplayNameValue] = useState(displayName);
  const [profileError, setProfileError] = useState<string | null>(null);
  const [profileSuccess, setProfileSuccess] = useState<string | null>(null);
  const [profileLoading, setProfileLoading] = useState(false);
  const [usernameError, setUsernameError] = useState<string | null>(null);
  const [displayNameError, setDisplayNameError] = useState<string | null>(null);

  const [currentPassword, setCurrentPassword] = useState("");
  const [newPassword, setNewPassword] = useState("");
  const [confirmPassword, setConfirmPassword] = useState("");
  const [currentPasswordError, setCurrentPasswordError] = useState<
    string | null
  >(null);
  const [newPasswordError, setNewPasswordError] = useState<string | null>(null);
  const [confirmPasswordError, setConfirmPasswordError] = useState<
    string | null
  >(null);
  const [passwordAlert, setPasswordAlert] = useState<string | null>(null);
  const [passwordSuccess, setPasswordSuccess] = useState<string | null>(null);
  const [passwordLoading, setPasswordLoading] = useState(false);
  const [showPasswords, setShowPasswords] = useState({
    current: false,
    next: false,
    confirm: false,
  });

  useEffect(() => {
    setUsernameValue(username);
  }, [username]);

  useEffect(() => {
    setDisplayNameValue(displayName);
  }, [displayName]);

  const profileDirty = useMemo(() => {
    return (
      usernameValue.trim() !== username ||
      displayNameValue.trim() !== displayName
    );
  }, [usernameValue, displayNameValue, username, displayName]);

  const validateProfile = () => {
    let hasError = false;
    const trimmedUsername = usernameValue.trim();
    const trimmedDisplayName = displayNameValue.trim();

    if (trimmedUsername.length < 3 || trimmedUsername.length > 30) {
      setUsernameError("Username must be between 3 and 30 characters");
      hasError = true;
    } else if (!usernameRegex.test(trimmedUsername)) {
      setUsernameError(
        "Username may only contain letters, numbers, and underscores"
      );
      hasError = true;
    } else {
      setUsernameError(null);
    }

    if (!trimmedDisplayName) {
      setDisplayNameError("Display name is required");
      hasError = true;
    } else if (trimmedDisplayName.length > 50) {
      setDisplayNameError("Display name must be 50 characters or fewer");
      hasError = true;
    } else {
      setDisplayNameError(null);
    }

    return !hasError;
  };

  const handleSaveProfile = async () => {
    setProfileSuccess(null);
    setProfileError(null);
    if (!profileDirty) {
      setProfileError("No changes to save");
      return;
    }
    if (!validateProfile()) return;

    const payload: Record<string, string> = {};
    if (usernameValue.trim() !== username) {
      payload.username = usernameValue.trim();
    }
    if (displayNameValue.trim() !== displayName) {
      payload.displayName = displayNameValue.trim();
    }
    if (Object.keys(payload).length === 0) {
      setProfileError("No changes to save");
      return;
    }

    setProfileLoading(true);
    try {
      const res = await fetch(apiUrl("/api/profile"), {
        method: "PATCH",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify(payload),
      });
      const data = await res.json();
      if (!res.ok) {
        setProfileError(data.error || "Failed to update profile");
        return;
      }
      setProfileSuccess("Profile updated successfully");
      const updated = data.user ?? {
        username: payload.username ?? username,
        displayName: payload.displayName ?? displayName,
      };
      setUsernameValue(updated.username);
      setDisplayNameValue(updated.displayName);
      if (onProfileUpdated) {
        onProfileUpdated(updated);
      }
    } catch (err) {
      setProfileError("Network error");
    } finally {
      setProfileLoading(false);
    }
  };

  const validatePasswordFields = () => {
    let hasError = false;

    if (!currentPassword) {
      setCurrentPasswordError("Current password is required");
      hasError = true;
    } else {
      setCurrentPasswordError(null);
    }

    if (newPassword.length < 8) {
      setNewPasswordError("Password must be at least 8 characters");
      hasError = true;
    } else if (!passwordRegex.test(newPassword)) {
      setNewPasswordError(
        "Password must include a letter, a number, and a special character"
      );
      hasError = true;
    } else {
      setNewPasswordError(null);
    }

    if (!confirmPassword) {
      setConfirmPasswordError("Please retype the new password");
      hasError = true;
    } else if (confirmPassword !== newPassword) {
      setConfirmPasswordError("Passwords do not match");
      hasError = true;
    } else {
      setConfirmPasswordError(null);
    }

    return !hasError;
  };

  const handleChangePassword = async () => {
    setPasswordSuccess(null);
    setPasswordAlert(null);

    if (!validatePasswordFields()) {
      return;
    }

    setPasswordLoading(true);
    try {
      const res = await fetch(apiUrl("/api/profile/password"), {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify({
          currentPassword,
          newPassword,
          confirmPassword,
        }),
      });
      const data = await res.json();
      if (!res.ok) {
        if (res.status === 401) {
          setCurrentPasswordError(
            data.error || "Current password is incorrect."
          );
        } else {
          setPasswordAlert(data.error || "Failed to change password");
        }
        return;
      }
      setPasswordSuccess("Password updated successfully");
      setCurrentPassword("");
      setNewPassword("");
      setConfirmPassword("");
      setCurrentPasswordError(null);
      setNewPasswordError(null);
      setConfirmPasswordError(null);
    } catch (err) {
      setPasswordAlert("Network error");
    } finally {
      setPasswordLoading(false);
    }
  };

  return (
    <Box display="flex" flexDirection="column" gap={4}>
      <Box>
        <Typography variant="h5" gutterBottom>
          Profile Settings
        </Typography>
        <Typography variant="body2" color="text.secondary" sx={{ mb: 3 }}>
          Update your username, display name, or change your password.
        </Typography>

        <Stack spacing={2}>
          <TextField
            label="Username"
            value={usernameValue}
            onChange={(e) => {
              setUsernameValue(e.target.value);
              setProfileError(null);
              setProfileSuccess(null);
            }}
            error={!!usernameError}
            helperText={usernameError ?? ""}
            size="small"
            autoComplete="username"
          />
          <TextField
            label="Display name"
            value={displayNameValue}
            onChange={(e) => {
              setDisplayNameValue(e.target.value);
              setProfileError(null);
              setProfileSuccess(null);
            }}
            error={!!displayNameError}
            helperText={displayNameError ?? ""}
            size="small"
          />
          {profileError && <Alert severity="error">{profileError}</Alert>}
          <Button
            variant="contained"
            onClick={handleSaveProfile}
            disabled={profileLoading || !profileDirty}
            sx={{ alignSelf: "flex-start" }}
          >
            {profileLoading ? "Saving..." : "Save changes"}
          </Button>
        </Stack>
      </Box>

      <Divider />

      <Box>
        <Typography variant="h6" gutterBottom>
          Change Password
        </Typography>
        <Stack spacing={2}>
          <TextField
            label="Current password"
            type={showPasswords.current ? "text" : "password"}
            value={currentPassword}
            onChange={(e) => {
              const value = e.target.value;
              setCurrentPassword(value);
              setPasswordAlert(null);
              setPasswordSuccess(null);
              if (!value) {
                setCurrentPasswordError("Current password is required");
              } else {
                setCurrentPasswordError(null);
              }
            }}
            size="small"
            autoComplete="current-password"
            error={!!currentPasswordError}
            helperText={currentPasswordError ?? ""}
            InputProps={{
              endAdornment: (
                <InputAdornment position="end">
                  <IconButton
                    aria-label="toggle current password visibility"
                    onClick={() =>
                      setShowPasswords((prev) => ({
                        ...prev,
                        current: !prev.current,
                      }))
                    }
                    edge="end"
                  >
                    {showPasswords.current ? <VisibilityOff /> : <Visibility />}
                  </IconButton>
                </InputAdornment>
              ),
            }}
          />
          <TextField
            label="New password"
            type={showPasswords.next ? "text" : "password"}
            value={newPassword}
            onChange={(e) => {
              const value = e.target.value;
              setNewPassword(value);
              setPasswordAlert(null);
              setPasswordSuccess(null);
              if (value.length < 8) {
                setNewPasswordError("Password must be at least 8 characters");
              } else if (!passwordRegex.test(value)) {
                setNewPasswordError(
                  "Password must include a letter, a number, and a special character"
                );
              } else {
                setNewPasswordError(null);
              }
              if (confirmPassword) {
                if (confirmPassword !== value) {
                  setConfirmPasswordError("Passwords do not match");
                } else {
                  setConfirmPasswordError(null);
                }
              }
            }}
            size="small"
            autoComplete="new-password"
            error={!!newPasswordError}
            helperText={newPasswordError ?? ""}
            InputProps={{
              endAdornment: (
                <InputAdornment position="end">
                  <IconButton
                    aria-label="toggle new password visibility"
                    onClick={() =>
                      setShowPasswords((prev) => ({
                        ...prev,
                        next: !prev.next,
                      }))
                    }
                    edge="end"
                  >
                    {showPasswords.next ? <VisibilityOff /> : <Visibility />}
                  </IconButton>
                </InputAdornment>
              ),
            }}
          />
          <TextField
            label="Confirm new password"
            type={showPasswords.confirm ? "text" : "password"}
            value={confirmPassword}
            onChange={(e) => {
              const value = e.target.value;
              setConfirmPassword(value);
              setPasswordAlert(null);
              setPasswordSuccess(null);
              if (!value) {
                setConfirmPasswordError("Please retype the new password");
              } else if (value !== newPassword) {
                setConfirmPasswordError("Passwords do not match");
              } else {
                setConfirmPasswordError(null);
              }
            }}
            size="small"
            autoComplete="new-password"
            error={!!confirmPasswordError}
            helperText={confirmPasswordError ?? ""}
            InputProps={{
              endAdornment: (
                <InputAdornment position="end">
                  <IconButton
                    aria-label="toggle confirm password visibility"
                    onClick={() =>
                      setShowPasswords((prev) => ({
                        ...prev,
                        confirm: !prev.confirm,
                      }))
                    }
                    edge="end"
                  >
                    {showPasswords.confirm ? <VisibilityOff /> : <Visibility />}
                  </IconButton>
                </InputAdornment>
              ),
            }}
          />
          {passwordAlert && <Alert severity="error">{passwordAlert}</Alert>}
          <Button
            variant="outlined"
            onClick={handleChangePassword}
            disabled={
              passwordLoading ||
              !currentPassword ||
              !newPassword ||
              !confirmPassword
            }
            sx={{ alignSelf: "flex-start" }}
          >
            {passwordLoading ? "Updating..." : "Update password"}
          </Button>
          <Typography variant="caption" color="text.secondary">
            Password must be at least 8 characters and include a letter, a
            number, and a special character.
          </Typography>
        </Stack>
      </Box>

      <Snackbar
        open={!!profileSuccess}
        autoHideDuration={4000}
        onClose={() => setProfileSuccess(null)}
        anchorOrigin={{ vertical: "bottom", horizontal: "center" }}
      >
        <Alert
          severity="success"
          onClose={() => setProfileSuccess(null)}
          variant="filled"
        >
          {profileSuccess}
        </Alert>
      </Snackbar>
      <Snackbar
        open={!!passwordSuccess}
        autoHideDuration={4000}
        onClose={() => setPasswordSuccess(null)}
        anchorOrigin={{ vertical: "bottom", horizontal: "center" }}
      >
        <Alert
          severity="success"
          onClose={() => setPasswordSuccess(null)}
          variant="filled"
        >
          {passwordSuccess}
        </Alert>
      </Snackbar>
    </Box>
  );
}
