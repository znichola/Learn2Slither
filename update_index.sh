#!/bin/bash
set -euo pipefail

INDEX_HTML="${1:-index.html}"
COMMIT="${2:-unknown}"
DATE="${3:-unknown}"

if [[ ! -f "$INDEX_HTML" ]]; then
    echo "Error: $INDEX_HTML not found"
    exit 1
fi

# Replace contents of <commit>...</commit> and <date>...</date>
sed -i \
    -e "s|<commit>.*</commit>|<commit><a href=https://github.com/znichola/Learn2Slither/commit/${COMMIT}>#<span>${COMMIT}</span></a></commit>|" \
    -e "s|<date>.*</date>|<date>${DATE}</date>|" \
    "$INDEX_HTML"

echo "Updated $INDEX_HTML:"
echo "  commit: $COMMIT"
echo "  date:   $DATE"