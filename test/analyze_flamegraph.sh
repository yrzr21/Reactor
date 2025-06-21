#!/bin/bash

FOLDED_FILE="./output/perf.folded"

if [ ! -f "$FOLDED_FILE" ]; then
    echo "File not found: $FOLDED_FILE"
    exit 1
fi

# 计算总采样数
TOTAL=$(awk '{sum+=$NF} END{print sum}' "$FOLDED_FILE")
echo "Total samples: $TOTAL"
echo

# 去除 abi 修饰等干扰信息
TEMP_FILE=$(mktemp)
sed -E 's/\[abi:[^]]+\]//g' "$FOLDED_FILE" > "$TEMP_FILE"

# 提取并统计 Class::Function 格式的函数
awk -v total="$TOTAL" '
{
    for (i = 1; i < NF; ++i) {
        if ($i ~ /::/) {
            fn[$i] += $NF
        }
    }
}
END {
    for (f in fn) {
        pct = fn[f] / total * 100
        printf "%-50s %10d samples  %6.2f %%\n", f, fn[f], pct
    }
}
' "$TEMP_FILE" | sort -k3 -nr

rm -f "$TEMP_FILE"
