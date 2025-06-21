#!/bin/bash

PERF_DATA="./perf.data"
SERVER="../debug/Reactor"
SERVER_IP="192.168.58.128"
SERVER_PORT=5005

# 启动 server，并用 perf 记录
echo "Starting server under perf..."

sudo env LD_LIBRARY_PATH=/home/toki/opt/gcc-13.2.0/lib64 \
    perf record -o "$PERF_DATA" "$SERVER" "$SERVER_IP" "$SERVER_PORT" > server.log 2>&1 &


SERVER_PID=$!
echo "Server PID: $SERVER_PID"

# 等待 server 启动
sleep 2

# 启动 Python 客户端测试
echo "Starting Python test script..."
python3 -u test.py > client_output.log 2>&1

# 等待所有客户端结束
echo "Waiting for server to finish..."
sleep 1
kill "$SERVER_PID"
wait "$SERVER_PID" 2>/dev/null

echo "perf record complete. Now run: sudo perf report -i $PERF_DATA"
