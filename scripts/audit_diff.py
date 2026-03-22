#!/usr/bin/env python3
"""
audit_diff.py - Compare behavioral baseline JSON files from MUSHclient and Mushkin.

Usage:
    python3 scripts/audit_diff.py <original.json> <mushkin.json> [--findings PATH]

The script flattens nested dicts to dot-separated keys, applies the allowlist
from scripts/audit_allowlist.json, and writes a markdown findings file for triage.

Exit codes:
    0 - no findings (all MATCH or skipped)
    1 - one or more MISMATCH, MISSING, or EXTRA findings
"""

import argparse
import json
import os
import re
import sys
from datetime import datetime, timezone
from pathlib import Path


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def flatten(obj, prefix="", out=None):
    """Recursively flatten a nested dict into dot-separated keys."""
    if out is None:
        out = {}
    if isinstance(obj, dict):
        for k, v in obj.items():
            flat_key = f"{prefix}.{k}" if prefix else str(k)
            flatten(v, flat_key, out)
    else:
        out[prefix] = obj
    return out


def normalize_value(value):
    """Strip drive letters and normalise backslashes to forward slashes."""
    if not isinstance(value, str):
        return value
    # Strip Windows drive letter (e.g. "C:")
    normalized = re.sub(r"^[A-Za-z]:", "", value)
    # Normalize backslashes to forward slashes
    normalized = normalized.replace("\\", "/")
    return normalized


def matches_any(key, patterns):
    """Return the first pattern that fully matches *key*, or None."""
    for pattern in patterns:
        if re.fullmatch(pattern, key):
            return pattern
    return None


def within_tolerance(a, b, tol):
    """Return True if both values are numeric and |a - b| <= tol."""
    try:
        return abs(float(a) - float(b)) <= tol
    except (TypeError, ValueError):
        return False


def load_allowlist(script_dir):
    """Load audit_allowlist.json from the same directory as this script."""
    path = Path(script_dir) / "audit_allowlist.json"
    if not path.exists():
        print(f"WARNING: allowlist not found at {path}; proceeding with empty allowlist",
              file=sys.stderr)
        return {"skip_keys": [], "normalize_paths": [], "tolerance": {}}
    with open(path, encoding="utf-8") as f:
        return json.load(f)


# ---------------------------------------------------------------------------
# Core comparison
# ---------------------------------------------------------------------------

Finding = dict  # {"category": str, "kind": str, "key": str, "original": any, "mushkin": any}


def compare(orig_flat, mush_flat, allowlist):
    """Compare two flattened dicts and return a list of Finding dicts."""
    skip_set = set(allowlist.get("skip_keys", []))
    normalize_patterns = allowlist.get("normalize_paths", [])
    tolerance_map = allowlist.get("tolerance", {})

    all_keys = sorted(set(orig_flat) | set(mush_flat))
    findings = []

    for key in all_keys:
        if key in skip_set:
            continue

        in_orig = key in orig_flat
        in_mush = key in mush_flat
        category = key.split(".")[0]

        if not in_orig:
            findings.append({
                "category": category,
                "kind": "EXTRA",
                "key": key,
                "original": None,
                "mushkin": mush_flat[key],
            })
            continue

        if not in_mush:
            findings.append({
                "category": category,
                "kind": "MISSING",
                "key": key,
                "original": orig_flat[key],
                "mushkin": None,
            })
            continue

        oval = orig_flat[key]
        mval = mush_flat[key]

        # Check tolerance first (numeric proximity)
        tol_pattern = matches_any(key, list(tolerance_map.keys()))
        if tol_pattern is not None:
            tol = tolerance_map[tol_pattern]
            if within_tolerance(oval, mval, tol):
                continue  # MATCH within tolerance

        # Apply path normalization if key matches normalize patterns
        if matches_any(key, normalize_patterns) is not None:
            oval = normalize_value(oval)
            mval = normalize_value(mval)

        if oval == mval:
            continue  # MATCH

        findings.append({
            "category": category,
            "kind": "MISMATCH",
            "key": key,
            "original": orig_flat[key],
            "mushkin": mush_flat[key],
        })

    return findings


# ---------------------------------------------------------------------------
# Reporting
# ---------------------------------------------------------------------------

def print_console_report(findings):
    """Print a grouped console summary."""
    if not findings:
        print("No findings — all keys match.")
        return

    by_category = {}
    for f in findings:
        by_category.setdefault(f["category"], []).append(f)

    totals = {"MISMATCH": 0, "MISSING": 0, "EXTRA": 0}
    for cat in sorted(by_category):
        items = by_category[cat]
        print(f"\n[{cat}]")
        for f in items:
            totals[f["kind"]] += 1
            k = f["kind"]
            key = f["key"]
            if k == "MISMATCH":
                print(f"  MISMATCH  {key}")
                print(f"            original = {f['original']!r}")
                print(f"            mushkin  = {f['mushkin']!r}")
            elif k == "MISSING":
                print(f"  MISSING   {key}  (original={f['original']!r})")
            else:
                print(f"  EXTRA     {key}  (mushkin={f['mushkin']!r})")

    print(
        f"\nSummary: {totals['MISMATCH']} mismatch, "
        f"{totals['MISSING']} missing, {totals['EXTRA']} extra"
    )


def write_markdown_report(findings, path):
    """Write a markdown findings file for human triage."""
    ts = datetime.now(timezone.utc).isoformat(timespec="seconds")

    by_category = {}
    for f in findings:
        by_category.setdefault(f["category"], []).append(f)

    lines = [
        "# Audit v2 Runtime Findings",
        "",
        f"Generated: {ts}",
        "",
        "Triage these into `docs/behavioral_worklist.md` with appropriate severity.",
        "",
    ]

    if not findings:
        lines.append("_No findings — all compared keys match._")
    else:
        for cat in sorted(by_category):
            lines.append(f"## {cat}")
            lines.append("")
            for f in by_category[cat]:
                k = f["kind"]
                key = f["key"]
                if k == "MISMATCH":
                    lines.append(
                        f"- **MISMATCH** `{key}`: "
                        f"original=`{f['original']}` mushkin=`{f['mushkin']}`"
                    )
                elif k == "MISSING":
                    lines.append(f"- **MISSING** `{key}`: original=`{f['original']}`")
                else:
                    lines.append(f"- **EXTRA** `{key}`: mushkin=`{f['mushkin']}`")
            lines.append("")

    output = "\n".join(lines) + "\n"
    Path(path).parent.mkdir(parents=True, exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        fh.write(output)
    print(f"Findings written to: {path}")


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main():
    script_dir = Path(__file__).parent

    parser = argparse.ArgumentParser(
        description="Compare MUSHclient vs Mushkin behavioral baseline JSON files."
    )
    parser.add_argument("original", help="Path to original (MUSHclient/Wine) JSON baseline")
    parser.add_argument("mushkin", help="Path to Mushkin JSON baseline")
    parser.add_argument(
        "--findings",
        default=str(script_dir.parent / "docs" / "audit_v2_runtime_findings.md"),
        help="Path to write the markdown findings file (default: docs/audit_v2_runtime_findings.md)",
    )
    args = parser.parse_args()

    # Load input files
    with open(args.original, encoding="utf-8") as f:
        orig_data = json.load(f)
    with open(args.mushkin, encoding="utf-8") as f:
        mush_data = json.load(f)

    # Validate metadata
    orig_meta = orig_data.get("metadata", {})
    mush_meta = mush_data.get("metadata", {})

    orig_world = orig_meta.get("world_name", "<unknown>")
    mush_world = mush_meta.get("world_name", "<unknown>")
    if orig_world != mush_world:
        print(
            f"WARNING: world_name mismatch: original={orig_world!r} mushkin={mush_world!r}",
            file=sys.stderr,
        )

    orig_plugins = orig_meta.get("plugin_count", 0)
    mush_plugins = mush_meta.get("plugin_count", 0)
    if abs(int(orig_plugins) - int(mush_plugins)) > 5:
        print(
            f"WARNING: plugin_count differs by more than 5: "
            f"original={orig_plugins} mushkin={mush_plugins}",
            file=sys.stderr,
        )

    # Flatten and compare
    orig_flat = flatten(orig_data)
    mush_flat = flatten(mush_data)

    allowlist = load_allowlist(script_dir)
    findings = compare(orig_flat, mush_flat, allowlist)

    # Report
    print_console_report(findings)
    write_markdown_report(findings, args.findings)

    sys.exit(1 if findings else 0)


if __name__ == "__main__":
    main()
