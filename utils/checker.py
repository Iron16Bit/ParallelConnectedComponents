#!/usr/bin/env python3
import os
import sys
from typing import List, Set, Iterable
import networkx as nx

DATA_DIR = os.path.join(os.path.dirname(__file__), "..", "Data")
DATA_DIR = os.path.normpath(DATA_DIR)

def list_data_files() -> List[str]:
    if not os.path.isdir(DATA_DIR):
        return []
    files = [f for f in os.listdir(DATA_DIR) if os.path.isfile(os.path.join(DATA_DIR, f))]
    return sorted(files)

def choose_file_interactive(files: List[str]) -> str:
    if not files:
        print(f"No files found in {DATA_DIR}")
        sys.exit(1)
    print("Files in Data/:")
    for i, fn in enumerate(files, start=1):
        print(f"  {i}. {fn}")
    while True:
        choice = input("Enter file number (or path): ").strip()
        if not choice:
            continue
        if choice.isdigit():
            idx = int(choice) - 1
            if 0 <= idx < len(files):
                return os.path.join(DATA_DIR, files[idx])
        candidate = choice
        if not os.path.isabs(candidate):
            candidate = os.path.join(DATA_DIR, candidate)
        if os.path.isfile(candidate):
            return os.path.normpath(candidate)
        print("Invalid selection, try again.")

def _lines_no_comments(path: str) -> Iterable[str]:
    with open(path, "r", encoding="utf-8") as fh:
        for raw in fh:
            line = raw.strip()
            if not line or line.startswith("%") or line.startswith("#"):
                continue
            yield line

def compute_connected_components(path: str) -> List[Set[str]]:
    """
    Read a graph file and compute connected components.
    Supports:
      - MatrixMarket coordinate format (header starts with '%%MatrixMarket')
        lines: i j value  -> edge between i and j if i != j
      - Simple edge list: "u v" per line (allows string or integer node ids)
    """
    G = nx.Graph()
    # detect MatrixMarket by reading first non-empty line
    with open(path, "r", encoding="utf-8") as fh:
        first_noncomment = None
        for raw in fh:
            s = raw.strip()
            if not s or s.startswith("%") or s.startswith("#"):
                continue
            first_noncomment = s
            break

    if first_noncomment is None:
        return []

    if first_noncomment.lower().startswith("%%matrixmarket"):
        # MatrixMarket: skip comments until size line, then triples
        with open(path, "r", encoding="utf-8") as fh:
            # skip header and comments
            for raw in fh:
                line = raw.strip()
                if not line or line.startswith("%"):
                    continue
                # first non-comment after header should be size line with three ints
                parts = line.split()
                if len(parts) >= 3 and all(p.isdigit() for p in parts[:3]):
                    # size line consumed; proceed to data lines
                    break
            # now parse remaining lines as i j value
            for raw in fh:
                line = raw.strip()
                if not line or line.startswith("%"):
                    continue
                parts = line.split()
                if len(parts) < 2:
                    continue
                try:
                    i = int(parts[0])
                    j = int(parts[1])
                except ValueError:
                    # fall back to strings
                    i = parts[0]
                    j = parts[1]
                if i == j:
                    continue
                G.add_edge(i, j)
    else:
        # treat as simple edge list (u v ...)
        with open(path, "r", encoding="utf-8") as fh:
            for raw in fh:
                line = raw.strip()
                if not line or line.startswith("#") or line.startswith("%"):
                    continue
                parts = line.split()
                if len(parts) < 2:
                    continue
                u, v = parts[0], parts[1]
                # try to convert to int where possible
                try:
                    u = int(u)
                except ValueError:
                    pass
                try:
                    v = int(v)
                except ValueError:
                    pass
                if u == v:
                    continue
                G.add_edge(u, v)

    comps = list(nx.connected_components(G))
    return comps

def main():
    files = list_data_files()
    chosen = choose_file_interactive(files)
    print(f"Using file: {chosen}")
    comps = compute_connected_components(chosen)
    comps_sorted = sorted(comps, key=lambda c: len(c), reverse=True)
    print(f"Found {len(comps_sorted)} connected component(s).")

if __name__ == "__main__":
    main()