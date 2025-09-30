import React, { useEffect } from "react";
import apiUrl from "./api";
import { useNavigate } from "react-router-dom";

export default function RequireAuth({
  children,
}: {
  children: React.ReactNode;
}) {
  const navigate = useNavigate();

  useEffect(() => {
    const checkAuth = async () => {
      try {
        const res = await fetch(apiUrl("/api/me"), { credentials: "include" });
        if (!res.ok) {
          navigate("/login");
        }
      } catch {
        navigate("/login");
      }
    };
    checkAuth();
  }, [navigate]);

  return <>{children}</>;
}
