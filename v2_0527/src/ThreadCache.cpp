#include "../include/ThreadCache.h"
#include "../include/CenterCache.h"
#include <cstdlib>


namespace MemoryPool
{



void* ThreadPool::allocate(size_t bytes){
    if(bytes == 0) bytes = 8;
    if(bytes > MAX_BYTES) return operator new(bytes);

    size_t index = SizeClass::getIndex(bytes);
    void* memory = free_list_[index];
    if(!memory){
        // void* memory_from_center = getMemoryFromCenter(bytes);
        memory = getMemoryFromCenter(bytes);
        
    }
    void *next = getNextBlock(memory);
    free_list_[index] = next;
    breakBlockConnect(memory);
    free_list_nums_[index]--;
    return memory;
}


void* ThreadPool::getMemoryFromCenter(size_t bytes){
    size_t index = SizeClass::getIndex(bytes);
    size_t nums_block;

    // const size_t max_batch_bytes = 2 * 1024; 
    if(bytes <= 32) nums_block = 64;
    else if (bytes <= 64) nums_block = 32;
    else if (bytes <= 128) nums_block = 16;
    else if (bytes <= 256) nums_block = 8;
    else if (bytes <= 512) nums_block = 4;
    else if (bytes <= 1024) nums_block = 2;
    else nums_block = 1;

    void* memory = CenterCache::getInstance().allocate(bytes, nums_block);
    free_list_nums_[index] = nums_block;
    // setListNums(index);
    return memory;
}

void ThreadPool::setListNums(size_t index){
    void* current_block = free_list_[index];
    size_t count = getCount(current_block);
    free_list_nums_[index] = count;
}


size_t ThreadPool::getCount(void* current){
    size_t count = 0;
    void* next = current;
    while(next){
        count ++;
        next = getNextBlock(next);
    }
    return count;
}


void ThreadPool::deallocate(void* ptr, size_t bytes){
    if(bytes == 0) bytes = 8;
    if(!ptr) return;
    if(bytes > MAX_BYTES) operator delete(ptr);

    size_t index = SizeClass::getIndex(bytes);
    void* next = getNextBlock(ptr);
    next = free_list_[index];
    free_list_[index] = ptr;
    free_list_nums_[index] += getCount(ptr);
    if(shouldReturn(bytes)){
        returnMemoryToCenter(bytes);
    }
}

bool ThreadPool::shouldReturn(size_t bytes){
    size_t index = SizeClass::getIndex(bytes);
    return free_list_nums_[index] > BLOCK_NUMS_MAX;
}

void ThreadPool::returnMemoryToCenter(size_t bytes){
    size_t index = SizeClass::getIndex(bytes);
    void* split_block = free_list_[index];
    int count = 0;
    while(count < (free_list_nums_[index] / 4) - 1){
        split_block = getNextBlock(split_block);
        count++;
    }
    void* return_memory = getNextBlock(split_block);
    breakBlockConnect(split_block);
    CenterCache::getInstance().deallocate(return_memory, bytes);
    free_list_nums_[index] = count + 1;
}

// ThreadPool::~ThreadPool(){
//     std::cout << "end" << std::endl;
// }


} // namespace MemoryPool

