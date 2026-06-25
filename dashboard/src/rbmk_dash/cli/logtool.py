"""rbmk-logtool: inspect, export, and verify HDF5 run logs.

Usage:
    rbmk-logtool info   runs/example.h5
    rbmk-logtool export runs/example.h5 --out example.csv [--fields power_mw ...]
    rbmk-logtool verify runs/example.h5
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

from rbmk_dash.core.recorder import INT_SIGNALS, SCALAR_SIGNALS, load_run, verify_run


def _cmd_info(args: argparse.Namespace) -> int:
    data = load_run(args.logfile)
    meta = data.meta
    t = data.timeseries["time_s"]
    print(f"file            : {args.logfile}")
    print(f"scenario        : {meta.get('scenario')}")
    print(f"model version   : {meta.get('model_version')} (ABI {meta.get('abi_version')})")
    print(f"created (UTC)   : {meta.get('created_utc')}")
    print(f"dt              : {float(meta.get('dt_s', 0.0)):.3f} s")
    print(f"sample stride   : {data.sample_stride}")
    print(f"total steps     : {data.total_steps}")
    print(f"samples         : {t.size}")
    if t.size:
        print(f"sim time span   : 0.0 .. {t[-1]:.1f} s")
        power = data.timeseries["power_mw"]
        print(f"power range     : {power.min():.1f} .. {power.max():.1f} MW")
    print(f"commands        : {len(data.commands)}")
    for c in data.commands[:20]:
        print(f"  step {c.step:>8d}  {c.name}{c.args}")
    if len(data.commands) > 20:
        print(f"  ... and {len(data.commands) - 20} more")
    print(f"events          : {len(data.events)}")
    for e in data.events[:10]:
        print(f"  step {e.step:>8d}  [{e.kind}] {e.text}")
    if len(data.events) > 10:
        print(f"  ... and {len(data.events) - 10} more")
    return 0


def _cmd_export(args: argparse.Namespace) -> int:
    data = load_run(args.logfile)
    fields = args.fields or [*SCALAR_SIGNALS, *INT_SIGNALS]
    unknown = [f for f in fields if f not in data.timeseries]
    if unknown:
        print(f"unknown fields: {', '.join(unknown)}", file=sys.stderr)
        return 2

    out = Path(args.out)
    columns = {name: data.timeseries[name] for name in fields}
    n = min(arr.shape[0] for arr in columns.values())
    with out.open("w", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(fields)
        for i in range(n):
            writer.writerow([columns[name][i] for name in fields])
    print(f"wrote {n} rows x {len(fields)} columns to {out}")
    return 0


def _cmd_verify(args: argparse.Namespace) -> int:
    report = verify_run(args.logfile)
    worst = max(report.values()) if report else float("inf")
    width = max(len(name) for name in report)
    for name in sorted(report):
        marker = "OK " if report[name] == 0.0 else "FAIL"
        print(f"{marker} {name:<{width}} max|recorded - replayed| = {report[name]:g}")
    if worst == 0.0:
        print("verify: PASS (bit-exact deterministic replay)")
        return 0
    print("verify: FAIL (replay deviated from the recorded run)", file=sys.stderr)
    return 1


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        prog="rbmk-logtool",
        description="Inspect, export, and verify RBMK-SIM deterministic run logs.",
    )
    sub = parser.add_subparsers(dest="command", required=True)

    p_info = sub.add_parser("info", help="print log metadata, commands, and events")
    p_info.add_argument("logfile", type=Path)
    p_info.set_defaults(func=_cmd_info)

    p_export = sub.add_parser("export", help="export timeseries to CSV")
    p_export.add_argument("logfile", type=Path)
    p_export.add_argument("--out", type=Path, required=True)
    p_export.add_argument("--fields", nargs="+", help="subset of signals to export")
    p_export.set_defaults(func=_cmd_export)

    p_verify = sub.add_parser("verify", help="replay the log and check bit-exactness")
    p_verify.add_argument("logfile", type=Path)
    p_verify.set_defaults(func=_cmd_verify)

    args = parser.parse_args(argv)
    return int(args.func(args))


if __name__ == "__main__":
    raise SystemExit(main())
