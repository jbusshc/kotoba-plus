#!/usr/bin/env python3

import argparse
import glob
import os
import sys
from pathlib import Path

OUTPUT_FILE = "dump.txt"


def is_binary_file(path, blocksize=1024):
    try:
        with open(path, "rb") as f:
            chunk = f.read(blocksize)
            if b"\0" in chunk:
                return True
        return False
    except Exception:
        return True


def expand_patterns(patterns):
    files = set()
    for pattern in patterns:
        matched = glob.glob(pattern, recursive=True)
        for m in matched:
            p = Path(m)
            if p.is_file():
                files.add(p.resolve())
    return files


def filter_excluded(files, exclude_patterns):
    excluded = set()
    for pattern in exclude_patterns:
        matched = glob.glob(pattern, recursive=True)
        for m in matched:
            p = Path(m)
            if p.is_file():
                excluded.add(p.resolve())
            elif p.is_dir():
                for root, _, filenames in os.walk(p):
                    for name in filenames:
                        excluded.add(Path(root, name).resolve())

    return {f for f in files if f not in excluded}


def dump_files(files, output_file):
    with open(output_file, "w", encoding="utf-8") as out:
        for file_path in sorted(files):
            try:
                if is_binary_file(file_path):
                    continue

                out.write(f"{'='*80}\n")
                out.write(f"FILE: {file_path}\n")
                out.write(f"{'='*80}\n\n")

                with open(file_path, "r", encoding="utf-8", errors="replace") as f:
                    out.write(f.read())
                    out.write("\n\n")

            except Exception as e:
                print(f"Error leyendo {file_path}: {e}", file=sys.stderr)


def main():
    parser = argparse.ArgumentParser(description="Dump multiple files into dump.txt")
    parser.add_argument("patterns", nargs="+", help="File patterns (e.g. *.c dir/**/*)")
    parser.add_argument("--exclude", action="append", default=[], help="Exclude patterns")

    args = parser.parse_args()

    included_files = expand_patterns(args.patterns)
    final_files = filter_excluded(included_files, args.exclude)

    dump_files(final_files, OUTPUT_FILE)

    print(f"Dump generado en {OUTPUT_FILE} con {len(final_files)} archivos.")


if __name__ == "__main__":
    main()