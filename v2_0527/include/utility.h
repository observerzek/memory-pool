#pragma once
// #include <cstddef>
#include <iostream>
#include <chrono>

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

class TimeLog{

private:
    std::chrono::time_point<std::chrono::high_resolution_clock> check_point_;
    double duration_time_;  
    std::string name_;
    size_t count_;
    
public:
    TimeLog(const std::string & name)
        : name_(name)
        , count_(0){
        // check_point_ = std::chrono::high_resolution_clock::now();
        duration_time_ = 0;
    }

    void reset(){
        duration_time_ = 0.0;
        count_ = 0;
    }

    void begin(){
        check_point_ = std::chrono::high_resolution_clock::now();
        count_++;
    }

    void end(){
        std::chrono::time_point<std::chrono::high_resolution_clock> now;
        now = std::chrono::high_resolution_clock::now();
        duration_time_ += std::chrono::duration_cast<std::chrono::microseconds>(now - check_point_).count();
    }

    void showDurationTime(){
        std::cout << name_ << " : " << duration_time_ / 1000.0 << "ms, count time : " 
                   << count_ << std::endl;
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

inline void setBlockNextPointer(void* ptr, void* target){
    *reinterpret_cast<void**>(ptr) = target;
}


} // namespace MemoryPool