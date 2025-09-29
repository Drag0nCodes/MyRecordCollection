import { BrowserRouter, Routes, Route } from "react-router-dom";
import { useEffect, useState } from "react";
import { Navigate } from "react-router-dom";
import LandingPage from "./LandingPage";
import MyCollection from "./MyCollection";
import FindRecord from "./FindRecord";
import Login from "./Login";
import Register from "./Register";
import RequireAuth from "./RequireAuth";

// Component that prevents authenticated users from seeing auth pages
function RedirectIfAuthed({ children }: { children: React.ReactNode }) {
  const [status, setStatus] = useState<"loading" | "authed" | "anon">(
    "loading"
  );
  useEffect(() => {
    let cancelled = false;
    (async () => {
      try {
        const res = await fetch("/api/me", { credentials: "include" });
        if (!cancelled) setStatus(res.ok ? "authed" : "anon");
      } catch {
        if (!cancelled) setStatus("anon");
      }
    })();
    return () => {
      cancelled = true;
    };
  }, []);
  if (status === "loading") return null; // or a spinner if desired
  if (status === "authed") return <Navigate to="/mycollection" replace />;
  return <>{children}</>;
}

export default function AppRouter() {
  return (
    <BrowserRouter>
      <Routes>
        <Route path="/" element={<LandingPage />} />
        <Route
          path="/mycollection"
          element={
            <RequireAuth>
              <MyCollection />
            </RequireAuth>
          }
        />
        <Route
          path="/findrecord"
          element={
            <RequireAuth>
              <FindRecord />
            </RequireAuth>
          }
        />
        <Route
          path="/login"
          element={
            <RedirectIfAuthed>
              <Login />
            </RedirectIfAuthed>
          }
        />
        <Route
          path="/register"
          element={
            <RedirectIfAuthed>
              <Register />
            </RedirectIfAuthed>
          }
        />
      </Routes>
    </BrowserRouter>
  );
}
