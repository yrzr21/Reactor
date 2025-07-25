#!/bin/bash

FLAME_GRAPH_DIR="$HOME/opt/FlameGraph"

# 路径配置
SERVER="../../build/Reactor"
CLIENT="../../build/performance_test"
OUTPUT_DIR="./output"
PERF_DATA="$OUTPUT_DIR/perf.data"
PERF_SCRIPT="$OUTPUT_DIR/perf.unfolded"
FLAME_OUTPUT="$OUTPUT_DIR/flamegraph.svg"

# 启动参数
# ip port n_client requests_per_client
CLIENT_ARGUMENT="192.168.58.128 5005 100 1000"
# ip port backlog nIOThreads connection_timeout_second loop_timer_interval n_work_threads
SERVER_ARGUMENT="192.168.58.128 5005 128 8 60 60 8"

mkdir -p "$OUTPUT_DIR"

# 启动服务端
"$SERVER" $SERVER_ARGUMENT &
SERVER_PID=$!

sleep 1

echo "服务端已启动，perf 采样开始..."
setsid sudo perf record -g -F 999 -o "$PERF_DATA" -p "$SERVER_PID" &> "$OUTPUT_DIR/perf.log" &
PERF_PID=$!

echo "CLIENT START"
"$CLIENT" $CLIENT_ARGUMENT
echo "CLIENT END"

echo "perf 采样已结束，杀死 perf 和服务端进程..."
sudo kill -INT "$PERF_PID"
wait "$PERF_PID" 2>/dev/null
kill $SERVER_PID
wait $SERVER_PID 2>/dev/null

echo "Generating FlameGraph..."
sudo perf script -i "$PERF_DATA" > "$PERF_SCRIPT"
"$FLAME_GRAPH_DIR/stackcollapse-perf.pl" "$PERF_SCRIPT" > "$OUTPUT_DIR/perf.folded"
"$FLAME_GRAPH_DIR/flamegraph.pl" "$OUTPUT_DIR/perf.folded" > "$FLAME_OUTPUT"
echo "FlameGraph generated at: $FLAME_OUTPUT"

