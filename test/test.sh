#!/bin/bash

set -eo pipefail

SERVER="../debug/Reactor"
# CLIENT="../debug/client"
CLIENT="../debug/test_max_concurrent_client"

# <ip> <port> <maxListen> <n io loop> <n work loop>  <timeout> <timer_interval>
SERVER_ARGUMENT="192.168.58.128 5005 4096 8 0 60 60"
# <ip> <port> <concurrency>
CLIENT_ARGUMENT="192.168.58.128 5005 40000"
# <ip> <port> <concurrency> <send_count> <payload_size>
# CLIENT_ARGUMENT="192.168.58.128 5005 100 40000 508"

OUTPUT_DIR="./output"
mkdir -p "$OUTPUT_DIR"

SERVER_LOG="$OUTPUT_DIR/server.log"
CLIENT_LOG="$OUTPUT_DIR/client.log"
PERF_DATA="$OUTPUT_DIR/perf.data"
PERF_SCRIPT="$OUTPUT_DIR/perf.unfolded"
FLAME_OUTPUT="$OUTPUT_DIR/flamegraph.svg"

CPU_CORES_SERVER="0-5"
CPU_CORES_CLIENT="6-7"
FLAME_GRAPH_DIR="$HOME/opt/FlameGraph"


sudo swapoff -a

echo "[INFO] Starting server normally..." >&2

# 启动服务器并绑定 CPU
taskset -c $CPU_CORES_SERVER "$SERVER" $SERVER_ARGUMENT > "$SERVER_LOG" 2>&1 &
SERVER_PID=$!
sleep 3

if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "[ERROR] Server failed to start, check $SERVER_LOG" >&2
    exit 1
fi
echo "[INFO] Server PID = $SERVER_PID" >&2

echo "[INFO] Starting perf record..." >&2
sudo perf record -g -F 999 -o "$PERF_DATA" -p "$SERVER_PID" > "$OUTPUT_DIR/perf.log" 2>&1 &

echo "[INFO] Running client..." >&2
date >&2
taskset -c $CPU_CORES_CLIENT "$CLIENT" $CLIENT_ARGUMENT > "$CLIENT_LOG" 2>&1
date >&2

echo "[INFO] Stopping server..." >&2
kill -TERM "$SERVER_PID"
wait "$SERVER_PID" 2>/dev/null

echo "[INFO] Generating FlameGraph..." >&2
sudo perf script -i "$PERF_DATA" > "$PERF_SCRIPT"
"$FLAME_GRAPH_DIR/stackcollapse-perf.pl" "$PERF_SCRIPT" > "$OUTPUT_DIR/perf.folded"
"$FLAME_GRAPH_DIR/flamegraph.pl" "$OUTPUT_DIR/perf.folded" > "$FLAME_OUTPUT"

echo "FlameGraph generated at: $FLAME_OUTPUT" >&2
echo "To view: open it in browser or VSCode with Live Preview plugin" >&2
