#include "./EchoServer.h"

#include <string>

#include "EchoServer.h"

std::atomic<int> connection_count = 0;
std::atomic<int> packet_count = 0;

EchoServer::EchoServer(EchoServerConfig &config)
    : tcpServer_(config.tcp_server_config),
      work_thread_pool_(config.n_work_threads, "work"),
      upstream_(config.echo_server_pool_options) {
    Logger::init();
    LOG_INFO("Logger init success");

    config.service_provider_config.upstream = &upstream_;
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] Root Upstream = " << &upstream_ << std::endl;
    service_provider_.emplace(config.service_provider_config);

    // set handler
    tcpServer_.setNewConenctionHandler([this](ConnectionPtr connection) {
        this->onNewConnection(connection);
    });
    tcpServer_.setMessageHandler(
        [this](ConnectionPtr connection, MsgVec &&message) {
            this->onMessage(connection, std::move(message));
        });
    tcpServer_.setSendCompleteHandler(
        [this](ConnectionPtr connection) { this->onSendComplete(connection); });
    tcpServer_.setConnectionCloseHandler([this](ConnectionPtr connection) {
        this->onConnectionClose(connection);
    });
    tcpServer_.setConnectionErrorHandler([this](ConnectionPtr connection) {
        this->onConnectionError(connection);
    });
    tcpServer_.setLoopTimeoutHandler(
        [this](Eventloop *loop) { this->onLoopTimeout(loop); });
}

EchoServer::~EchoServer() { stop(); }

void EchoServer::start() {
    LOG_INFO("Start EchoServer");
    tcpServer_.start();
}

void EchoServer::stop() {
    work_thread_pool_.stopAll();
    LOG_INFO("工作线程已停止");

    // tcpServer_.stop();
    // 作为成员自动析构
    // LOG_INFO("tcpServer 已停止");
}

//  -- handler --
void EchoServer::onNewConnection(ConnectionPtr connection) {
    connection_count++;
    if (connection_count % 100 == 0) {
        LOG_INFO("Got {} Connections", connection_count);
    }
    // LOG_INFO("New Connection: fd={}", connection->fd());
}
void EchoServer::onMessage(ConnectionPtr connection, MsgVec &&message) {
    // std::cout << "onMessage: fd=" << connection->fd()
    //           << "msg=" << message->c_str() << std::endl;
    // printf("%ld HandleOnMessage: recv(eventfd=%d):%s\n", syscall(SYS_gettid),
    // connection->fd(), message.c_str());
    packet_count++;
    if (packet_count % 10000 == 0) {
        LOG_INFO("Handled {} packets", packet_count);
    }

    if (work_thread_pool_.size() == 0) {
        sendMessage(connection, std::move(message));
    } else {
        // 添加到工作线程。原shared_ptr计数+1，此前的级联调用中计数不变
        // std::cout << "将在工作线程中调用sendMessage" << std::endl;
        // ptr 将来可能被继续移动，需要加 mutable
        work_thread_pool_.addTask([this, con_ptr = connection,
                                   msg_ptr = std::move(message)]() mutable {
            // std::cout << "在工作线程中调用sendMessage" << std::endl;
            this->sendMessage(con_ptr, std::move(msg_ptr));
        });
    }
}
void EchoServer::onSendComplete(ConnectionPtr connection) {
    // printf("send complete\n");
}
void EchoServer::onConnectionClose(ConnectionPtr connection) {
    connection_count--;
}
void EchoServer::onConnectionError(ConnectionPtr connection) {
    connection_count--;
    LOG_ERR("client error, fd = {}", connection->fd());
}
void EchoServer::onLoopTimeout(Eventloop *loop) {
    // printf("loop time out\n");
}

// using MessagePtr = std::unique_ptr<std::string>;
void EchoServer::sendMessage(ConnectionPtr connection, MsgVec &&message) {
    // printf("onMessage runing on thread(%ld).\n", syscall(SYS_gettid));
    usleep(100);
    size_t nrecved = 0;
    // std::cout << "recv: ";
    for (size_t i = 0; i < message.size(); i++) {
        nrecved += message[i].size_;
        std::cout << message[i].data_;
    }
    // std::cout << std::endl;

    // 构造多个报文发回去
    auto &sync_pool = ServiceProvider::getLocalSyncPool();
    std::pmr::string msg{&sync_pool};
    msg = "received: " + std::to_string(nrecved) + " bytes";
    // std::cout << "[tid=" << std::this_thread::get_id()
    //           << "] msg1 constructed" << std::endl;

    std::pmr::string msg2{&sync_pool};
    msg2 = "data = ";
    for (size_t i = 0; i < message.size(); i++) {
        msg2.append(message[i].data_, message[i].size_);
    }
    // std::cout << "[tid=" << std::this_thread::get_id()
    //       << "] msg2 constructed" << std::endl;

    MsgVec msgs;
    msgs.emplace_back(std::move(msg));
    msgs.emplace_back(std::move(msg2));
    // std::cout << "[tid=" << std::this_thread::get_id()
    //       << "] msg vec constructed" << std::endl;
    // std::cout << "\nabout to send: " << std::endl;
    // std::cout << msgs[0].data_ << std::endl;
    // std::cout << msgs[1].data_ << std::endl;

    // std::cout << "sendMessage: fd=" << connection->fd() << ", msg=" << ret
    //           << ", size=" << ret.size() << std::endl;
    // sleep(2);
    connection->postSend(std::move(msgs));
}
