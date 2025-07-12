#!/bin/bash
CLIENT="../../build/performance_test"
# ip port n_client requests_per_client
CLIENT_ARGUMENT="192.168.58.128 5005 100 10000"
SERVER="../../build/Reactor"
# ip port backlog nIOThreads connection_timeout_second loop_timer_interval n_work_threads
SERVER_ARGUMENT="192.168.58.128 5005 128 3 60 60 3"

"$SERVER" $SERVER_ARGUMENT &
SERVER_PID=$!

sleep 1

"$CLIENT" $CLIENT_ARGUMENT

kill $SERVER_PID
wait $SERVER_PID 2>/dev/null