#!/usr/bin/env python3
"""Merge the SignBenchmark harness's two output files into one results table.

Inputs (defaults assume this script is run from the project root, or you pass
--project-dir):
  Saved/SignBenchmark/benchmark_results.csv   - one row per SignBenchSpawn run,
                                                 written by ASignBenchmarkGameMode.
  Saved/Profiling/CSV/Profile(*).csv          - one file per run's csvprofile
                                                 capture window, one row per frame.

For each benchmark row, this finds the Profile(*).csv whose embedded timestamp
falls inside [ProfilingWindowStart, ProfilingWindowEnd] (with a small tolerance),
averages the relevant per-frame columns across that file's steady-state window,
and writes one merged row combining both. No manual transcription, no manual
matching.

Usage:
    python merge_benchmark_results.py [--project-dir PATH] [--out FILE] [--tolerance-seconds N]

Output: benchmark_merged.csv, one row per SignBenchSpawn run, written next to
benchmark_results.csv unless --out is given.
"""

import argparse
import csv
import glob
import os
import re
import statistics
import sys
from datetime import datetime, timedelta

# Per-frame columns from the CSV Profiler worth carrying into the merged table.
# (key in the Profile(*).csv header) -> (name for the merged output column)
PROFILE_COLUMNS = {
    "FrameTime": "FrameTime_ms",
    "GameThreadTime": "GameThreadTime_ms",
    "RenderThreadTime": "RenderThreadTime_ms",
    "GPUTime": "GPUTime_ms",
    "RHI/DrawCalls": "DrawCalls",
    "RHI/PrimitivesDrawn": "PrimitivesDrawn",
    "MemoryFreeMB": "MemoryFreeMB",
    "PhysicalUsedMB": "PhysicalUsedMB",
    "GPUMem/LocalUsedMB": "GPUMem_LocalUsedMB",
    "GPUMem/SystemUsedMB": "GPUMem_SystemUsedMB",
}

PROFILE_FILENAME_RE = re.compile(r"Profile\((\d{8})_(\d{6})\)\.csv$")


def parse_iso(ts: str) -> datetime:
    # FDateTime::ToIso8601() looks like "2026-09-22T22:46:49.303Z" — the trailing Z
    # claims UTC, but FDateTime::Now() is actually local system time; UE just labels
    # it as if it were UTC without converting. Strip the Z and treat as naive local
    # time throughout, consistent with the filename timestamps below (same clock).
    if ts.endswith("Z"):
        ts = ts[:-1]
    return datetime.fromisoformat(ts)


def parse_profile_filename(path: str):
    m = PROFILE_FILENAME_RE.search(os.path.basename(path))
    if not m:
        return None
    date_part, time_part = m.groups()
    return datetime.strptime(date_part + time_part, "%Y%m%d%H%M%S")


def load_profile_averages(path: str):
    """Returns (dict of averaged columns, frame_count) for one Profile(*).csv."""
    with open(path, newline="", encoding="utf-8-sig") as f:
        reader = csv.DictReader(f)
        sums = {col: 0.0 for col in PROFILE_COLUMNS}
        counts = {col: 0 for col in PROFILE_COLUMNS}
        frame_count = 0
        for row in reader:
            frame_count += 1
            for col in PROFILE_COLUMNS:
                raw = row.get(col)
                if raw is None or raw == "":
                    continue
                try:
                    sums[col] += float(raw)
                    counts[col] += 1
                except ValueError:
                    continue
        averages = {}
        for col, out_name in PROFILE_COLUMNS.items():
            averages[out_name] = (sums[col] / counts[col]) if counts[col] > 0 else None
        return averages, frame_count


def main():
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--project-dir", default=".", help="dt4mob-twin-viewer project root (default: current dir)")
    ap.add_argument("--out", default=None, help="output CSV path (default: Saved/SignBenchmark/benchmark_merged.csv)")
    ap.add_argument("--tolerance-seconds", type=float, default=2.0,
                     help="how far outside [WindowStart, WindowEnd] a Profile(*).csv's embedded timestamp may fall and still match (default 2s)")
    args = ap.parse_args()

    results_path = os.path.join(args.project_dir, "Saved", "SignBenchmark", "benchmark_results.csv")
    profiling_dir = os.path.join(args.project_dir, "Saved", "Profiling", "CSV")
    out_path = args.out or os.path.join(args.project_dir, "Saved", "SignBenchmark", "benchmark_merged.csv")

    if not os.path.isfile(results_path):
        sys.exit(f"Not found: {results_path} — run at least one SignBenchSpawn first.")
    if not os.path.isdir(profiling_dir):
        sys.exit(f"Not found: {profiling_dir} — no csvprofile capture has run yet.")

    profile_files = glob.glob(os.path.join(profiling_dir, "Profile(*).csv"))
    profile_index = []  # (timestamp, path)
    for p in profile_files:
        ts = parse_profile_filename(p)
        if ts is not None:
            profile_index.append((ts, p))
    profile_index.sort()

    with open(results_path, newline="", encoding="utf-8-sig") as f:
        bench_rows = list(csv.DictReader(f))

    merged_rows = []
    unmatched = []
    ambiguous = []
    tol = timedelta(seconds=args.tolerance_seconds)

    for row in bench_rows:
        try:
            win_start = parse_iso(row["ProfilingWindowStart"])
            win_end = parse_iso(row["ProfilingWindowEnd"])
        except (KeyError, ValueError) as e:
            unmatched.append((row, f"bad timestamp: {e}"))
            continue

        candidates = [(ts, p) for ts, p in profile_index if (win_start - tol) <= ts <= (win_end + tol)]

        merged = dict(row)
        if len(candidates) == 0:
            unmatched.append((row, "no Profile(*).csv found in window"))
            for out_name in PROFILE_COLUMNS.values():
                merged[out_name] = ""
            merged["NumFramesAveraged"] = 0
            merged["ProfilingFile"] = ""
            merged["MatchStatus"] = "MISSING"
        else:
            # Prefer whichever candidate's embedded timestamp is closest to this run's own
            # WindowStart, not just "any file inside the tolerance window" — back-to-back runs
            # a couple seconds apart can otherwise both fall inside a wide tolerance and collide.
            candidates.sort(key=lambda c: abs((c[0] - win_start).total_seconds()))
            best_ts, best_path = candidates[0]
            if len(candidates) > 1:
                second_ts, _ = candidates[1]
                best_diff = abs((best_ts - win_start).total_seconds())
                second_diff = abs((second_ts - win_start).total_seconds())
                if abs(best_diff - second_diff) < 0.5:  # genuine tie, not just "also nearby"
                    ambiguous.append((row, [p for _, p in candidates]))
                    for out_name in PROFILE_COLUMNS.values():
                        merged[out_name] = ""
                    merged["NumFramesAveraged"] = 0
                    merged["ProfilingFile"] = ";".join(os.path.basename(p) for _, p in candidates)
                    merged["MatchStatus"] = "AMBIGUOUS"
                    merged_rows.append(merged)
                    continue

            averages, frame_count = load_profile_averages(best_path)
            merged.update({k: ("" if v is None else f"{v:.4f}") for k, v in averages.items()})
            merged["NumFramesAveraged"] = frame_count
            merged["ProfilingFile"] = os.path.basename(best_path)
            merged["MatchStatus"] = "OK"

        merged_rows.append(merged)

    if merged_rows:
        fieldnames = list(merged_rows[0].keys())
        with open(out_path, "w", newline="", encoding="utf-8") as f:
            writer = csv.DictWriter(f, fieldnames=fieldnames)
            writer.writeheader()
            writer.writerows(merged_rows)

    print(f"Wrote {len(merged_rows)} merged row(s) to {out_path}")
    ok_count = sum(1 for r in merged_rows if r.get("MatchStatus") == "OK")
    print(f"  matched: {ok_count}  missing: {len(unmatched)}  ambiguous: {len(ambiguous)}")

    if unmatched:
        print("\nRows with no matching Profile(*).csv (check these manually):")
        for row, reason in unmatched:
            print(f"  {row.get('Timestamp','?')} {row.get('Strategy','?')} count={row.get('RequestedCount','?')}: {reason}")

    if ambiguous:
        print("\nRows matching MULTIPLE Profile(*).csv files (resolve manually, capture windows probably overlapped):")
        for row, candidates in ambiguous:
            names = ", ".join(os.path.basename(c) for c in candidates)
            print(f"  {row.get('Timestamp','?')} {row.get('Strategy','?')} count={row.get('RequestedCount','?')}: {names}")


if __name__ == "__main__":
    main()
