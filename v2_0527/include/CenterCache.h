#pragma once
#include "utility.h"
#include <atomic>
#include <array>

namespace MemoryPool
{

class CenterCache{
private:
    std::array<std::atomic<Span*>, FREE_LIST_SIZE> center_list_;
    std::array<std::atomic_flag, FREE_LIST_SIZE> center_list_lock_;

public:
    // 在这里我返回的是指针
    // 但是原文返回的是引用
    // 为什么？
    // 有什么区别吗？
    // ------------------
    // 0526：
    // 我看到网上单例模式用的是引用
    // 改回去了
    static CenterCache& getInstance(){
        static CenterCache center_cache;
        return center_cache;
    }

    void* allocate(size_t bytes, size_t nums);

    void deallocate(void* ptr, size_t bytes);

private:
    CenterCache(){
        for(auto &i : center_list_){
            i.store(nullptr, std::memory_order_relaxed);
        }
        for(auto &i : center_list_lock_){
            i.clear(std::memory_order_relaxed);
        }
    }

    void* getMemoryFromPage(size_t bytes);
    
    // 0526
    // 要实现这个函数好像挺难的
    // 因为从PageCache里申请到的最小单位是 8 x 4096字节
    // (8页，每页4096B)
    // 这些申请到的单个空间是连续的
    // 意味着要归还的空间也是连续的
    // 要找出连续的 4096 B 个连续的空间单位
    // 好像比较难
    void  returnMemoryToPage(void* ptr, size_t bytes);

    bool  shouldReturn(size_t bytes);
};




} // namespace MemoryPool