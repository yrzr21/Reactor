#include "ThreadPool.h"

// 启动threadnum个线程，并让他们阻塞在 wait 上
ThreadPool::ThreadPool(size_t nThreads, const std::string &threadType)
    : isStop_(false), threadType_(threadType)
{
    for (size_t ii = 0; ii < nThreads; ii++)
        threads_.emplace_back(std::bind(&ThreadPool::thread_func, this));
}

ThreadPool::~ThreadPool()
{
    this->isStop_ = true;
    this->condition_.notify_all();
    for (auto &t : this->threads_)
        t.join();
}

// void ThreadPool::addTask(std::function<void()> task)
// {
//     {
//         lk_t lock(this->mutex_);
//         this->tasks_.push(task);
//     }
//     this->condition_.notify_one();
// }

void ThreadPool::addTask(std::function<void()> task, const std::string &taskName)
{
    {
        lk_t lock(this->mutex_);
        this->tasksWithName_.emplace(std::make_pair(task, taskName));
    }
    this->condition_.notify_one();
}

size_t ThreadPool::size()
{
    return this->threads_.size();
}

// 线程的启动函数，启动后阻塞在 wait 上
void ThreadPool::thread_func() // 用 lambda 函数也可以
{
    // printf("create %s thread(%ld).\n", this->threadType_.c_str(), syscall(SYS_gettid)); // 显示线程ID。os分配的唯一id
    // std::cout << "子线程：" << std::this_thread::get_id() << std::endl; // C++11库分配的id

    while (isStop_ == false) // 判断执不执行
    {
        tsk_t task; // 执行的任务
        // std::function<void()> task; // 执行的任务
        { // 锁保护
            lk_t lock(this->mutex_);

            // 阻塞，等待唤醒，唤醒后检查：isStop、任务是否有剩余
            this->condition_.wait(lock, [this]
                                  { return ((this->isStop_ == true) || (this->tasksWithName_.empty() == false)); });
            // 要求停止且无任务则直接返回
            if ((this->isStop_ == true) && (this->tasksWithName_.empty() == true))
                return;

            task = std::move(this->tasksWithName_.front()); // 取任务
            this->tasksWithName_.pop();
        }

        // printf("%s thread %ld excecutes task %s.\n", this->threadType_.c_str(), syscall(SYS_gettid), task.second.c_str());
        task.first(); // 执行
    }
}
