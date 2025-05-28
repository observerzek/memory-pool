#pragma once
// #include <cstddef>
#include <iostream>


namespace MemoryPool
{

constexpr size_t ALIGNMENT = 8;
// 最大为256KB
constexpr size_t MAX_BYTES = 256 * 1024;
// 表长为 32K
constexpr size_t FREE_LIST_SIZE = MAX_BYTES / ALIGNMENT;

constexpr size_t BLOCK_NUMS_MAX = 64;

constexpr size_t PAGE_SIZE = 4096;

constexpr size_t PAGE_NUMS = 64;

constexpr size_t MAX_PAGE_NUMS = MAX_BYTES / PAGE_SIZE;

class SizeClass{
public:

    inline static size_t byteAlignment(size_t bytes){
        return (getIndex(bytes) + 1) * ALIGNMENT;
    }

    // 1~8 byte -> 0
    // 9~16 byte -> 1
    // 17~24 byte -> 2
    inline static size_t getIndex(size_t bytes){
        return (bytes + ALIGNMENT - 1) / ALIGNMENT - 1;
    }



};

// Span是一个记录信息的结点
// 多个页面组合在一起形成一个Span
// page__add记录的是多个页面的起始地址
// page_nums就是页面数量
// is_use表示是否被分配到CenterCache里
// 每个Span在分配到CenterCache时
// 会根据bytes大小，分割成多个自由链表block
// block_list 则是记录该空闲链表头
// block_allocated表示分配出去的block数量
// block_non_allocated表示未被分配出去的block数量

struct Span{
    Span* pre = nullptr;
    Span* next= nullptr; 
    void* page_add = nullptr;
    size_t page_nums = 0;
    bool is_use = false;
    void* block_list = nullptr;
    size_t block_nums = 0;
    size_t block_allocated = 0;
    size_t block_non_allocated = 0;
};


inline void* getNextBlock(void* current){
    return *reinterpret_cast<void**>(current);
}

inline void breakBlockConnect(void* current){
    *reinterpret_cast<void**>(current) = nullptr;
}

inline void setBlockNextPointer(void* (&ptr), void* target){
    *reinterpret_cast<void**>(ptr) = target;
}


} // namespace MemoryPool