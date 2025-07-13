
基于 `epoll` 的 C++17 异步响应框架，实现了主从 Reactor 模型、应用层零拷贝、高效内存复用、连接管理与线程池

>使用了个别C++23特性，整体仍以C++17为主
### 特性
- **主从 Reactor +工作线程**：分别用于监听新连接、io、业务计算
- **回调机制**：采用 Channel → Connection -> TcpServer -> EchoServer 级联回调
- **std::pmr + writev 零拷贝**：设计思路见 [README/应用层IO零拷贝.md](README/应用层IO零拷贝.md)
- **定时连接清理 & 异步唤醒机制**：基于 `timerfd` / `eventfd`
- **RAII + 智能指针管理资源**
### 主要模块说明
#### 服务器
| 模块         | 简介                            |
| ---------- | ----------------------------- |
| Channel    | 封装可读/可写/关闭/错误事件，统一事件回调        |
| EventLoop  | 事件循环，负责执行回调                   |
| TcpServer  | 管理连接创建、分发、回收，可继续回调业务层         |
| ThreadPool | 线程池，执行业务任务/运行事件循环             |
| Connection | 抽象一个 TCP 连接，负责读写与生命周期管理       |
| Acceptor   | 主Reactor，负责监听新连接并回调 TcpServer |
#### 缓冲区
| 模块               | 简介                                             |
| ---------------- | ---------------------------------------------- |
| ServiceProvider  | 全局服务点，为线程提供私有内存池                               |
| SmartMonoPool    | 单线程一次性分配、多线程自动释放的无锁固定大小的内存池                    |
| SmartMonoManager | 用于管理SmartMonoPool，通过接口切换新SmartMonoPool，并等待旧的释放 |
| SyncPool         | 适用于已知大小、多线程、同一对象多次进行的内存分配                      |
| MsgView          | 对缓冲区的视图层，由指针+大小构成，析构时自动释放；另外也可从pmr::string 构造  |
| RecvBuffer       | 接收报文的缓冲区，自动处理半包、粘包、包过大的问题，自动换新池                |
| SendUnit         | 用于封装多个MsgView构成的一个发送单元，在发送完毕时统一释放              |
| SendBuffer       | 从SendUnit构建iov，并调用writev发送报文，然后更新发送状态          |
### 编译 & 运行
- 要求gcc 13.2.0以上，用于支持C++23 `move_only_function`

- 克隆、编译、安装必要库：
```bash
sudo apt install libspdlog-dev
git clone https://github.com/yrzr21/Reactor.git
cd Reactor
mkdir build && cd build
cmake ..
make -j
cd ..
```

- 修改运行脚本的参数，然后运行：
```bash
chmod +x open_server.sh 
./open_server.sh 
```
- 另开一个终端，修改参数，然后运行从命令行输入数据的客户端：
```bash
chmod +x open_client.sh 
./open_client.sh 
```
#### 性能测试
- 见[README/性能测试.md](README/性能测试.md)
### todo
- [x] 添加性能测试脚本
- [x] 降低`_raw_spin_unlock_irqrestore`耗时，发现原因为服务端客户端竞争网络协议栈
- [ ] 改写生成-消费者实现方式，去除条件变量同步；降低 `shared_ptr` 使用占比
- [ ] 使用协程构建服务器
- [ ] 使用 pmr 管理所有容器的内存
- [ ] 添加守护类，托管断开连接的资源