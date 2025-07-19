#include "ServiceProvider.h"

/* 成员唯一定义，防止每个包含头文件的 cpp 文件都会尝试生成一个 noop_，导致
 * ODR（One Definition Rule）违规 */
thread_local NonOverlapObjectPool ServiceProvider::noop_;
