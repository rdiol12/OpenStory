import { NextRequest, NextResponse } from "next/server";
import { execFile, spawn } from "child_process";
import { promisify } from "util";
import { isInternalOrAdmin } from "@/lib/api-guard";
const pexecFile = promisify(execFile);

export const dynamic = "force-dynamic";
export const maxDuration = 600;

// Ship a pre-built factory weapon bundle (build_weapon.py output: weapon.png canonical + icon.png +
// meta.json) into the live game via ship_factory_weapon.py. No art generation happens here — the
// bundle's canonical PNG is embedded verbatim.
const TYPES = new Set([130, 131, 132, 137, 138, 140, 143, 144]);

export async function POST(req: NextRequest) {
  if (!(await isInternalOrAdmin(req))) return NextResponse.json({ error: "unauthorized" }, { status: 401 });
  let body: Record<string, unknown>;
  try { body = await req.json(); } catch { return NextResponse.json({ error: "invalid JSON" }, { status: 400 }); }
  const bundleDir = String(body.bundleDir ?? "").trim();
  const name = String(body.name ?? "").trim();
  if (!name) return NextResponse.json({ error: "name is required" }, { status: 400 });
  if (!bundleDir.startsWith("/opt/cosmic-creature-gen/") || bundleDir.includes(".."))
    return NextResponse.json({ error: "bundleDir must live under /opt/cosmic-creature-gen/" }, { status: 400 });

  const args = ["/opt/cosmic-creature-gen/ship_factory_weapon.py", bundleDir, "--name", name, "--commit"];
  if (body.desc) args.push("--desc", String(body.desc));
  if (typeof body.reqLevel === "number" && Number.isFinite(body.reqLevel)) args.push("--req-level", String(Math.trunc(body.reqLevel)));
  if (typeof body.type === "number" && TYPES.has(body.type)) args.push("--type", String(body.type));

  try {
    const out = (await pexecFile("python3", args, { encoding: "utf-8", timeout: 540000, maxBuffer: 64 * 1024 * 1024 })).stdout;
    const res = JSON.parse(out.trim().split("\n").filter((l) => l.trim().startsWith("{")).pop() || "{}") as { itemId?: number };
    if (!res.itemId) return NextResponse.json({ error: "shipper returned no itemId", raw: out.slice(-500) }, { status: 500 });
    spawn("/bin/bash", ["-c", "/usr/local/bin/cosmic-build-updates.sh Character.wz String.wz && systemctl restart cosmic.service"], { detached: true, stdio: "ignore" }).unref();
    return NextResponse.json({ ok: true, itemId: res.itemId, name, note: "Factory weapon shipped. NX rebuild + restart dispatched — allow a few minutes, then !item " + res.itemId });
  } catch (e: unknown) {
    return NextResponse.json({ error: `factory ship failed: ${(e instanceof Error ? e.message : String(e)).slice(-600)}` }, { status: 500 });
  }
}
