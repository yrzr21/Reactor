#pragma once

#include <condition_variable>
#include <fstream>
#include <map>
#include <mutex>
#include <semaphore>
#include <string>
#include <vector>

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
};

class VirtualRegionManager {
   public:
    uintptr_t allocRegion();                   // 4MB 对齐
    static size_t addrRegion(uintptr_t addr);  // 返回该地址所属的 region 号
    static size_t regionBytes();

   private:
    std::vector<Area> parseMappings();
    uintptr_t findFreeRegion(const std::vector<Area>& mappings);
    uintptr_t alignUp(uintptr_t addr);

   private:
    /* 4MB 对齐，虚拟内存可按 4MB 编号 */
    static constexpr size_t ALIGN_BYTES = 4 * 1024 * 1024;
    static constexpr size_t ALIGN_SHIFT = 22;  // 2^22 B = 4 MB

    std::binary_semaphore sem_{1};
};

inline uintptr_t VirtualRegionManager::allocRegion() {
    sem_.acquire();  // 睡眠锁
    std::vector<Area> mappings = parseMappings();
    uintptr_t region = findFreeRegion(mappings);
    sem_.release();
    return region;
}

inline size_t VirtualRegionManager::addrRegion(uintptr_t addr) {
    return addr >> ALIGN_SHIFT;
}

inline size_t VirtualRegionManager::regionBytes() { return ALIGN_BYTES; }

inline std::vector<Area> VirtualRegionManager::parseMappings() {
    std::ifstream maps("/proc/self/maps");

    std::vector<Area> mappings;
    std::string line;
    while (std::getline(maps, line)) {
        auto dash = line.find('-');
        if (dash != std::string::npos) {
            const char* range = line.c_str();
            uintptr_t start = strtoull(range, nullptr, 16);
            size_t bytes = strtoull(range + dash + 1, nullptr, 16) - start;
            mappings.push_back({start, bytes});
        }
    }
    return mappings;
}

inline uintptr_t VirtualRegionManager::findFreeRegion(
    const std::vector<Area>& mappings) {
    for (size_t i = 0; i + 1 < mappings.size(); i++) {
        uintptr_t gap_start = alignUp(mappings[i].start + mappings[i].bytes);
        size_t gap_bytes = mappings[i + 1].start - gap_start;

        if (gap_bytes >= regionBytes()) return gap_start;
    }
    return 0;
}

inline uintptr_t VirtualRegionManager::alignUp(uintptr_t addr) {
    return (addr + ALIGN_BYTES - 1) & ~(ALIGN_BYTES - 1);
}
