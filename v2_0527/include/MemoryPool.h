#pragma once
#include "ThreadCache.h"



namespace MemoryPool
{

void* allocate(size_t bytes){
    return ThreadPool::getInstance()->allocate(bytes);
}

void deallocate(void* ptr, size_t bytes){
    ThreadPool::getInstance()->deallocate(ptr, bytes);
}



} // namespace MemoryPool