#!/bin/bash
../debug/Reactor 192.168.58.128 5005 > ./output.txt 2>&1 &

# 等待服务器启动
sleep 3

python3 ./test.py

# 终止服务器
SERVER_PID=$(pgrep -f 'debug/Reactor 192.168.58.128 5005')
if [ -n "$SERVER_PID" ]; then
    kill $SERVER_PID
fi