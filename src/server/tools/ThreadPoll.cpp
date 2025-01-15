#include "ThreadPoll.h"

// 启动threadnum个线程，并让他们阻塞在 wait 上
ThreadPoll::ThreadPoll(size_t nThreads) : isStop_(false)
{
    for (size_t ii = 0; ii < nThreads; ii++)
        threads_.emplace_back(std::bind(&ThreadPoll::thread_func, this));
}

ThreadPoll::~ThreadPoll()
{
    this->isStop_ = true;
    this->condition_.notify_all();
    for (auto &t : this->threads_)
        t.join();
}

void ThreadPoll::addTask(std::function<void()> task)
{
    {
        lk lock(this->mutex_);
        this->tasks_.push(task);
    }
    this->condition_.notify_one();
}

// 线程的启动函数，启动后阻塞在 wait 上
void ThreadPoll::thread_func() // 用 lambda 函数也可以
{
    printf("create thread(%d).\n", syscall(SYS_gettid)); // 显示线程ID。os分配的唯一id
    // std::cout << "子线程：" << std::this_thread::get_id() << std::endl; // C++11库分配的id

    while (isStop_ == false) // 判断执不执行
    {
        std::function<void()> task; // 执行的任务
        {                           // 锁保护
            lk lock(this->mutex_);

            // 阻塞，等待唤醒，唤醒后检查：isStop、任务是否有剩余
            this->condition_.wait(lock, [this]
                                  { return ((this->isStop_ == true) || (this->tasks_.empty() == false)); });
            // 要求停止且无任务则直接返回
            if ((this->isStop_ == true) && (this->tasks_.empty() == true))
                return;

            task = std::move(this->tasks_.front()); // 取任务
            this->tasks_.pop();
        }

        printf("thread is %d.\n", syscall(SYS_gettid));
        task(); // 执行
    }
}
