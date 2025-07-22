#pragma once

#include <condition_variable>
#include <fstream>
#include <iostream>
#include <iterator>
#include <map>
#include <mutex>
#include <optional>
#include <semaphore>
#include <string>
#include <vector>

#include "Region.h"

/*

负责查找不重叠的区域、查询当前地址所属的区域号

强硬控制与选择解析区域

单 Region 4MB，可分配 64K 个 64B对象

todo：懒查找

应保证全局唯一
*/

/* [start, start+bytes)，不用 char* 是因为可能被误解指向字符串 */
struct Area {
    uintptr_t start;
    size_t bytes;

    Area(uintptr_t start, size_t bytes) : start(start), bytes(bytes) {}
};

class VirtualRegionManager {
   public:
    VirtualRegionManager();

    std::unique_ptr<Region> allocRegion();     // 4MB 对齐
    static size_t addrRegion(uintptr_t addr);  // 返回该地址所属的 region 号
    static size_t regionBytes();

    void registerRegion(uintptr_t start, size_t bytes);

   private:
    void updateVMA();  // 解析 /proc/self/maps
    static void insertAndMerge(std::map<uintptr_t, Area>& vmas, Area area);
    static bool overlap(const Area& a, const Area& b);  // 辅助函数

    uintptr_t findFreeRegion();
    uintptr_t alignUp(uintptr_t addr);

   private:
    /* 4MB 对齐，虚拟内存可按 4MB 编号 */
    static constexpr size_t ALIGN_BYTES = 4 * 1024 * 1024;
    static constexpr size_t ALIGN_SHIFT = 22;  // 2^22 B = 4 MB

    std::binary_semaphore sem_{1};

    /* 用户层维护的 vma，可能与实际不符，需要在必要时同步 */
    std::map<uintptr_t, Area> os_vma_;    // 解析 /proc/self/maps 得到的
    std::map<uintptr_t, Area> user_vma_;  // allocRegion 分配的
};

inline VirtualRegionManager::VirtualRegionManager() { updateVMA(); }

inline std::unique_ptr<Region> VirtualRegionManager::allocRegion() {
    sem_.acquire();  // 睡眠锁
    uintptr_t addr = findFreeRegion();

    try {
        // 可能抛映射失败的异常
        Region* region = new Region(addr, regionBytes());
        sem_.release();
        return std::unique_ptr<Region>(region);
    } catch (...) {
        updateVMA();  // 映射失败时重新解析

        sem_.release();
        return {};
    }
}

inline size_t VirtualRegionManager::addrRegion(uintptr_t addr) {
    return addr >> ALIGN_SHIFT;
}

inline size_t VirtualRegionManager::regionBytes() { return ALIGN_BYTES; }

inline void VirtualRegionManager::updateVMA() {
    std::cout << "updateVMA: " << std::endl;

    std::ifstream maps("/proc/self/maps");

    std::string line;
    while (std::getline(maps, line)) {
        std::cout << line << std::endl;
        auto dash = line.find('-');
        if (dash != std::string::npos) {
            const char* range = line.c_str();
            uintptr_t start = strtoull(range, nullptr, 16);
            size_t bytes = strtoull(range + dash + 1, nullptr, 16) - start;

            insertAndMerge(os_vma_, {start, bytes});
        }
    }
    std::cout << "after update, os vma=" << std::endl;
    for (auto& iter : os_vma_) {
        std::cout << std::hex << "start=" << iter.first
                  << ", end=" << iter.first + iter.second.bytes << std::endl;
    }
}

inline void VirtualRegionManager::insertAndMerge(
    std::map<uintptr_t, Area>& vmas, Area area) {
    // cur
    uintptr_t cur_start = area.start;
    uintptr_t cur_end = area.start + area.bytes;
    // std::cout << "start = " << cur_start << ", end = " << cur_end <<
    // std::endl;
    if (vmas.empty()) {
        vmas.emplace(area.start, area);
        return;
    }

    auto next_iter = vmas.lower_bound(area.start);

    // prev，先处理 next 是为了防止 next_iter 失效
    if (next_iter != vmas.begin()) {
        auto prev_iter = std::prev(next_iter);
        Area& prev_area = prev_iter->second;
        assert(!overlap(prev_area, area));

        if (cur_start == prev_area.start + prev_area.bytes) {
            area.start = prev_area.start;  // 后面统一插入
            area.bytes += prev_area.bytes;
            vmas.erase(prev_iter);
        }
    }

    // next
    if (next_iter != vmas.end()) {
        Area& next_area = next_iter->second;
        assert(!overlap(area, next_area));

        if (cur_end == next_area.start) {
            vmas.erase(next_iter);
            area.bytes += next_area.bytes;  // 后面统一插入
        }
    }

    // insert
    vmas.emplace(area.start, area);
}

inline bool VirtualRegionManager::overlap(const Area& a, const Area& b) {
    return !((a.start < b.start && a.start + a.bytes <= b.start) ||
             (a.start > b.start && b.start + b.bytes <= a.start));
}

inline uintptr_t VirtualRegionManager::findFreeRegion() {
    auto cur = os_vma_.begin();
    auto next = std::next(cur);
    while (next != os_vma_.end()) {
        uintptr_t gap_start = alignUp(cur->second.start + cur->second.bytes);
        size_t gap_bytes = next->second.start - gap_start;

        if (gap_bytes >= regionBytes()) {
            std::cout << "free region at 0x" << std::hex << gap_start
                      << std::endl;
            return gap_start;
        }

        // upadte
        cur = next;
        next = std::next(cur);
    }

    std::cout << "VirtualRegionManager::findFreeRegion err" << std::endl;
    return 0;
}

inline uintptr_t VirtualRegionManager::alignUp(uintptr_t addr) {
    return (addr + ALIGN_BYTES - 1) & ~(ALIGN_BYTES - 1);
}
