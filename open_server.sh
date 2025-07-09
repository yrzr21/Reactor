#!/bin/bash
SERVER="./build/Reactor"
# ip port backlog nIOThreads connection_timeout_second loop_timer_interval n_work_threads
SERVER_ARGUMENT="192.168.58.128 5005 128 3 60 60 3"

"$SERVER" $SERVER_ARGUMENT
