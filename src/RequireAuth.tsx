import React, { useEffect } from "react";
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
        const res = await fetch("/api/me", { credentials: "include" });
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
