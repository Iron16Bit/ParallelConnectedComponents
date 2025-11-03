#!/usr/bin/env bash
set -euo pipefail

DATA_DIR="Data"
ZIP="$DATA_DIR/dataset.zip"

if [ ! -f "$ZIP" ]; then
    echo "No zip file found at $ZIP"
    exit 0
fi

echo "Extracting $ZIP into $DATA_DIR ..."

# 1) try unzip
if command -v unzip >/dev/null 2>&1; then
    echo "Using unzip"
    unzip -o "$ZIP" -d "$DATA_DIR"
    rm -f "$ZIP"
    echo "Extraction complete (unzip)."
    exit 0
fi

# 2) try python3/python
extract_with_python() {
  PY_CMD="$1"
  echo "Using $PY_CMD (zipfile)"
  "$PY_CMD" - <<PYCODE
import sys, zipfile
zip_path = sys.argv[1]
dest = sys.argv[2]
try:
    with zipfile.ZipFile(zip_path, 'r') as z:
        z.extractall(dest)
except Exception as e:
    sys.exit(2)
sys.exit(0)
PYCODE
  return $?
}

if command -v python3 >/dev/null 2>&1; then
    if extract_with_python python3 "$ZIP" "$DATA_DIR"; then
        rm -f "$ZIP"
        echo "Extraction complete (python3)."
        exit 0
    fi
fi

if command -v python >/dev/null 2>&1; then
    if extract_with_python python "$ZIP" "$DATA_DIR"; then
        rm -f "$ZIP"
        echo "Extraction complete (python)."
        exit 0
    fi
fi

# 3) try PowerShell (Windows)
PS_CANDIDATE=""
if command -v pwsh >/dev/null 2>&1; then
    PS_CANDIDATE="$(command -v pwsh)"
elif command -v powershell.exe >/dev/null 2>&1; then
    PS_CANDIDATE="$(command -v powershell.exe)"
fi

if [ -n "$PS_CANDIDATE" ]; then
    echo "Using PowerShell ($PS_CANDIDATE)"
    # Use single quotes inside the PowerShell command to avoid shell interpolation
    "$PS_CANDIDATE" -NoProfile -Command "try { Expand-Archive -LiteralPath '$ZIP' -DestinationPath '$DATA_DIR' -Force; exit 0 } catch { exit 2 }"
    if [ $? -eq 0 ]; then
        rm -f "$ZIP"
        echo "Extraction complete (PowerShell)."
        exit 0
    fi
fi

echo "Failed to extract $ZIP: no available extractor (unzip, python, or PowerShell)."
exit 1
# ...existing code...