#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

/*

负责分配不重叠的区域，保证分配的 chunk 不会重叠

由于仅当 area 计数归 0 时取消映射，所以这里我们可以直接选取没映射的部分返回

应保证全局唯一，或者加全局锁
*/

/* [start, end)，不用 char* 是因为可能被误解指向字符串 */
struct Area {
    uintptr_t start;
    size_t bytes;
};

class NonOverlapPool {
   public:
    static Area findFreeArea(size_t bytes, size_t align = 4096);

   private:
    static bool overlap(uintptr_t start, size_t bytes);
    static Area findFreeArea(const std::vector<Area>& mappings, size_t bytes,
                             size_t align);

    static uintptr_t alignUp(uintptr_t addr, size_t align);
    static std::vector<Area> parseMappings();
};

inline std::vector<Area> NonOverlapPool::parseMappings() {
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

inline Area NonOverlapPool::findFreeArea(const std::vector<Area>& mappings,
                                         size_t bytes, size_t align) {
    for (size_t i = 0; i + 1 < mappings.size(); i++) {
        uintptr_t gap_start = alignUp(mappings[i].end, align);
        size_t gap_bytes = mappings[i + 1].start - gap_start;

        if (gap_bytes >= bytes && !overlap(gap_start, bytes)) {
            return {gap_start, bytes};
        }
    }
    return {0, 0};
}

inline Area NonOverlapPool::findFreeArea(size_t bytes, size_t align) {
    std::vector<Region> mappings = parseMappings();
    return findFreeArea(mappings, bytes, align);
}

inline uintptr_t NonOverlapPool::alignUp(uintptr_t addr, size_t align) {
    return (addr + align - 1) & ~(align - 1);
}
