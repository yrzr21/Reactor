#include <signal.h>

#include <iostream>

#include "./server/EchoServer.h"

EchoServer *server;

void stopServer(int sig) {
    Logger::getConsoleLogger()->info("sig = {}", sig);
    Logger::getConsoleLogger()->info("Stopping...");

    delete server;
    LOG_INFO("server已停止");

    exit(0);
}

int main(int argc, char *argv[]) {
    if (argc != 8) {
        std::cout << "invalid argument： " << std::endl;
        for (size_t i = 0; i < argc; i++) {
            std::cout << argv[i] << std::endl;
        }

        return -1;
    }

    signal(SIGTERM, stopServer);
    signal(SIGINT, stopServer);

    EchoServerConfig config{
        .tcp_server_config{
            .ip = argv[1],
            .port = atoi(argv[2]),
            .backlog = atoi(argv[3]),
            .nIOThreads = atoi(argv[4]),
            .connection_timeout_second = atoi(argv[5]),
            .loop_timer_interval = atoi(argv[6]),
        },
        .service_provider_config{
            // 暂定每事件循环线程 40MB 缓冲，可满足10000个缓冲区
            .io_chunk_size = 4096 * 10000,
            // .io_chunk_size = 4096,
            .io_min_remain_bytes = 4096,
            .work_options =
                {
                    .max_blocks_per_chunk = 40,
                    .largest_required_pool_block = 1024,
                },
        },
        .n_work_threads = atoi(argv[7]),
        .echo_server_pool_options{
            .max_blocks_per_chunk = 1000,
            .largest_required_pool_block = 4096,
        },
    };

    // std::cout << "start" << std::endl;
    server = new EchoServer(config);
    server->start();

    return 0;
}