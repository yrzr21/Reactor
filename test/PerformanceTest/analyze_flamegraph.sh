#!/bin/bash

FOLDED_FILE="output/perf.folded"
OUTPUT_FILE="output/analyse_simplify.txt"

if [ ! -f "$FOLDED_FILE" ]; then
    echo "Input file not found: $FOLDED_FILE"
    exit 1
fi

sed -E 's/^.*;//' "$FOLDED_FILE" | awk '
{
    name = ""; count = 0
    for (i = 1; i <= NF; ++i) {
        if (i == NF) {
            count = $i
        } else {
            name = name (name=="" ? "" : " ") $i
        }
    }
    data[name] += count
    total += count
}
END {
    for (f in data) {
        pct = data[f] / total * 100
        if (pct >= 1)  # 可选过滤小于1%的项
            printf "%.2f%% %s\n", pct, f
    }
}
' | sort -nr > "$OUTPUT_FILE"
