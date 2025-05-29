#pragma once
#include "utility.h"
#include <array>
#include <vector>   


namespace MemoryPool
{


class ThreadPool{
private:
    std::array<void*, FREE_LIST_SIZE> free_list_;
    std::array<size_t, FREE_LIST_SIZE> free_list_nums_;
    TimeLog log_;

public:
    // 0526
    // 网上看到单例模式用的是引用
    // 为什么这里用指针？
    static ThreadPool* getInstance(){
        static thread_local ThreadPool instance;
        return &instance;
    }

    void* allocate(size_t bytes);
    void deallocate(void* ptr, size_t bytes);

    ~ThreadPool();

private:
    ThreadPool()
        : log_("thread pool"){
        free_list_.fill(nullptr);
        free_list_nums_.fill(0);
    }



    void* getMemoryFromCenter(size_t bytes);

    // void setListNums(size_t index);

    // size_t getCount(void* current);

    void  returnMemoryToCenter(size_t bytes);

    void returnAllMemoryTocenter(void* memory_list, size_t bytes);

    bool  shouldReturn(size_t bytes);

};





} // namespace MemoryPool