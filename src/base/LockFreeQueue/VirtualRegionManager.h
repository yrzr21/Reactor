#pragma once

#include <fstream>
#include <map>
#include <string>
#include <vector>

/*

负责查找不重叠的区域、查询当前地址所属的区域号

强硬控制与选择解析区域

应保证全局唯一，或者加全局锁

todo：巨大页？尽量用这种东西
todo：多个区域合为一个分配？
todo：锁
*/

class VirtualRegionManager {
   public:
    static uintptr_t allocRegion(size_t bytes = 1);  // 4MB 对齐
    static size_t addrRegion(uintptr_t addr);  // 返回该地址所属的 region 号
    static size_t regionBytes();

   private:
    static std::vector<Area> parseMappings();
    static uintptr_t findFreeArea(const std::vector<Area>& mappings,
                                  size_t bytes);
    static uintptr_t alignUp(uintptr_t addr);

   private:
    /* [start, start+bytes)，不用 char* 是因为可能被误解指向字符串 */
    struct Area {
        uintptr_t start;
        size_t bytes;
    };

    /* 4MB 对齐，虚拟内存可按 4MB 编号 */
    static constexpr size_t ALIGN_BYTES = ::getpagesize() * 1024;
    static constexpr size_t ALIGN_SHIFT = 22;  // 2^22 B = 4 MB
};

inline uintptr_t VirtualRegionManager::allocRegion(size_t bytes) {
    std::vector<Area> mappings = parseMappings();
    return findFreeArea(mappings, bytes);
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

inline Area VirtualRegionManager::findFreeArea(
    const std::vector<Area>& mappings, size_t bytes) {
    for (size_t i = 0; i + 1 < mappings.size(); i++) {
        uintptr_t gap_start = alignUp(mappings[i].end);
        size_t gap_bytes = mappings[i + 1].start - gap_start;

        if (gap_bytes >= bytes) return {gap_start, bytes};
    }
    return {0, 0};
}

inline uintptr_t VirtualRegionManager::alignUp(uintptr_t addr) {
    return (addr + ALIGN_BYTES - 1) & ~(ALIGN_BYTES - 1);
}
