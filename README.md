## 特点
- 线程池
- reactor 模式
	- 事件循环运行在线程池，生产-消费者模型
	- 其中工作线程由 echoserver 持有，io线程由tcpserver持有
	- channel 作为 net 和 event 的桥梁
- 回调解耦
- 支持自动解析、添加4B报文头的 ring_buffer 应用层缓冲区
- 智能指针管理生命周期，lambda，std::function
- 如何异步唤醒事件循环处理异步io任务、如何定时清理空闲连接
- 使用了 move_only_function 优化
## 可调用对象
#### std::bind 与 lambda
后者更清晰、易读、高性能
#### 目标函数必须可拷贝
- 例如**捕获了 unique_ptr 的 lambda** 就不行，因为那会导致lambda自身也变为仅移动
## 生命周期管理
#### RAII
- 资源获取即初始化
- 本项目中即：Socket、Epoll、TcpServer
#### shared_from_this
- 若一个类要传递他自己的智能指针出去，需要：
	1. 继承 enable_shared_from_this 
	2. 通过 shared_from_this() 传递智能指针
- 本项目中即：Connection
#### shared_ptr 和 unique_ptr
- 前者开销不小，能用后者就用后者
- 而为了异步传递 msg，并用 unique_ptr 管理，引出 C++23 move_only_function
#### Buffer 拷贝次数
- 报文有2/3次拷贝：
	1. 网卡->内存中的内核缓冲区(可并行)
	2. 内核缓冲区->用户缓冲区
	3. 用户缓冲区->上层处理
-  为何不用 `string_view` 优化拷贝次数
	- 1的优化即DMA，2 的优化即 proactor 模式
	- 由于无法保证用户 ring_buffer 缓冲区中的数据在被处理前不被覆盖，因此无法使用 string_view，3不可优化

## ring_buffer
>拷贝次数优化见上文
- 实现方式：环形缓冲区+读写指针分离
#### why ring_buffer？
- 理想情况下，不需要多余内存申请
- 无内存碎片
- 缓存友好

#### 扩容机制
- 当数据量超过容器大小，需要扩容
- 仅拷贝未处理数据到新容器开头

#### 4B 报文头
- 提供接口：popMessage、pushMessage，传递完整报文并自动补全/解析报文头
- 解决TCP半包/粘包/分包问题

## 多线程竞争
- 一个连接运行在一个事件循环中，所以不需要加锁






## 其他问题
#### 命名规范
- 成员变量：小写+下划线
- 成员函数：小驼峰
- 类类型：大驼峰
#### 回调函数命名
- 函数的“**被动**”或“**主动**”属性，取决于当前类的**角色与立场**，而不取决于函数本身
- 可以据此命名
	- on：被动
	- handle：主动
#### 别名
- 多用 using，减少认知负担
- 但要注意直观性，不能掩盖原类型本质
#### 减少函数
- 如果几个函数仅仅只是对同一函数的不同参数的封装，那可合并为一个
#### 是否裸使用宏
- 可封装为更易读的类，且是零开销抽象
- 但是为了兼容需要写更多的代码，带来更多的负担，其收益小于成本...所以还是算了吧
![](assets/Pasted%20image%2020250617134752.png)
![](assets/Pasted%20image%2020250617134804.png)

#### core dump
- 找到出事的pid
```undefined
coredumpctl info
```
- 进入调试
```bash
coredumpctl debug pid
```
- gdb 中
```bash
bt
info locals
```

#### 实用工具
- 代码行数统计：
```swift
cloc /path/to/your/project
```


- `cppcheck`：分析代码潜在问题以及复杂度
```bash
cppcheck --enable=style,performance,unusedFunction --inline-suppr /path/to/project 2>&1 | tee cppcheck_report.txt

```




#### gcc-11
- gcc-11 不支持C++23，需要升级