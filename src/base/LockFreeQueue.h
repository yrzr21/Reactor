#pragma once

#include <atomic>

#include "../types.h"

/* 

todo：ABA问题解决

*/

template <typename T>
class LockFreeQueue {
   public:
    LockFreeQueue();
    ~LockFreeQueue();

    void enqueue(T&& task);
    bool dequeue(T& task_dest);

   private:
    struct Node {
        T data;
        std::atomic<Node*> next{nullptr};

        Node() = default;
        Node(T&& task) : data(std::move(task)) {}
    };

    std::atomic<Node*> dummy_head_;  // dummy 队头
    std::atomic<Node*> tail_;        // 队尾
};

//==================== 实现 ====================

template <typename T>
inline LockFreeQueue<T>::LockFreeQueue() {
    Node* dummy = new Node;
    dummy_head_.store(dummy, std::memory_order_relaxed);
    tail_.store(dummy, std::memory_order_relaxed);
}

template <typename T>
inline LockFreeQueue<T>::~LockFreeQueue() {
    Node* cur = dummy_head_.load(std::memory_order_relaxed);
    while (cur) {
        Node* next = cur->next;
        delete cur;
        cur = next;
    }
}

template <typename T>
inline void LockFreeQueue<T>::enqueue(T&& task) {
    Node* new_node = new Node(std::move(task));

    Node* tail;
    do {
        // 先获取尾部状态
        tail = tail_.load(std::memory_order_acquire);
        Node* next = tail->next.load(std::memory_order_acquire);
        // 两个热点：tail_指针所在的缓存行，tail_ 指针指向的节点所在的缓存行
        // new_node 指针是本地的，不是热点

        /*
            为什么不直接 tail->next.CAS(nullptr, new_node)，而要先读取
        old_next，判断为 nullptr 再 CAS?
            - 在竞争激烈的情况下，old_tail->next 更新迅速
            - 这意味着直接 CAS 会产生大量的失败 CAS
            - 而又因为 MESI，CAS 需要独占，一定会导致远程缓存行失效
            - 这对于大量的失败 CAS 是不值得的
            - 不如先 load，看一下是不是失效了再 CAS，而不是直接独占然后再看
        */

        if (next == nullptr) {
            // 若当前状态最新(next==nullptr)，则获得修改权，插入节点
            if (tail->next.compare_exchange_weak(next, new_node)) {
                break;
            }
        } else {
            /*
                为什么这里却直接CAS了，而不需要什么先看再推，而是直接推然后看？
                - tail_ 所在的缓存行必然是高频失效的
                - 相比节省CAS，tail_卡住整个生产线才是最恐怖的
                - 用户态下无法中断，可能被抢占，这是可能卡住很久很久的
                - 所以这里直接 CAS 才是更快的，虽然 tail_ 会高频失效
            */

            // 非最新状态，帮忙推进 tail_
            tail_.compare_exchange_weak(tail, next);
        }

    } while (true);

    // 已插入节点，推进 tail_
    tail_.compare_exchange_weak(tail, new_node);
}

template <typename T>
inline bool LockFreeQueue<T>::dequeue(T& data_dest) {
    Node* head;
    while (true) {
        head = dummy_head_.load(std::memory_order_acquire);
        Node* next = head->next.load(std::memory_order_acquire);
        // 热点：1. dummy_head_指针所在的缓存行
        // 2. dummy_head_指向的数据(next)所在的缓存行

        if (next == nullptr) return false;  // 空队列

        /*
            为什么相比于入队，出队简单得多？
            - 入队需要挂链+推进，出队只需要推进
            - 出队是天然串行的，而入队不是，必须协作同步
        */
        if (dummy_head_.compare_exchange_weak(head, next)) {
            data_dest = std::move(head->data);
            delete head;
            return true;
        }
    }
}
