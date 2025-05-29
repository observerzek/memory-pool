#include "../include/CenterCache.h"
#include "../include/PageCache.h"
#include <atomic>
#include <thread>
#include <cstring>
#include <algorithm>
#include <math.h>
#include <cassert>

namespace MemoryPool
{



void* CenterCache::allocate(size_t bytes, size_t nums){
    size_t index = SizeClass::getIndex(bytes);

    while(center_list_lock_[index].test_and_set(std::memory_order_acquire)){
        std::this_thread::yield();
    }

    log_allocate_.begin();
    Span* now_span = center_list_[index].load(std::memory_order_relaxed);
    // memory_list_head
    // 为返回block list的头指针
    void* memory_list_head = nullptr;
    void* memory_list_new = nullptr;
    void* memory_list_new_next = nullptr;
    size_t allocted_block_nums = 0;

    while(allocted_block_nums < nums){
        if(now_span){
            if(now_span->block_list){
                // 记录该空闲block
                memory_list_new = now_span->block_list;
                // 要先把block_list指针 
                // 及时转移到下一个空闲block
                now_span->block_list = getNextBlock(now_span->block_list);

                // 25.05.27
                // 采用头插法构建返回block list
                // 将当前空闲block的next指针，指向memory_list_head
                // 这样空闲block与原span的block_list的连接就断开了
                setBlockNextPointer(memory_list_new, memory_list_head);
                // 使用新方法设置指针
                // *reinterpret_cast<void**>(memory_list_new) = memory_list_head;
                memory_list_head = memory_list_new;

                allocted_block_nums++;
                now_span->block_allocated++;
                now_span->block_non_allocated--;
            }
            else{
                now_span = now_span->next;
            }
        }
        else{
            // 25.05.28
            // 如果center_list_里的span队列
            // 找不到足够的空闲block 给thread
            // 就要从page cache里申请span
            // 再采用头插法
            // 插入到center_list_里
            now_span = getMemoryFromPage(bytes);


            // now_span = center_list_[index].load(std::memory_order_relaxed);
            // Span* new_span = getMemoryFromPage(bytes);
            // new_span->next = now_span;
            // now_span->pre = new_span;
            // center_list_[index].store(new_span, std::memory_order_relaxed);
        }
    }  
    log_allocate_.end();
    center_list_lock_[index].clear(std::memory_order_release);
    
    
    return memory_list_head; 
    
}


Span* CenterCache::getMemoryFromPage(size_t bytes){
    // 因为PAGE_SIZE = 4096 = 2^12
    // 所以可以写成
    // page_nums = (bytes + PAGE_SIZE - 1) >> 12;
    bytes = SizeClass::byteAlignment(bytes);
    size_t index = SizeClass::getIndex(bytes);
    size_t page_nums = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
    Span* new_span = PageCache::getInstance().allocate(page_nums);
    Span* list_head = center_list_[index].load(std::memory_order_relaxed);
    


    size_t block_nums = 0;

    assert(new_span->page_nums >= 1);
    void* page_add_end = reinterpret_cast<char*>(new_span->page_add) 
                       + new_span->page_nums * PAGE_SIZE;
    void* current_block = new_span->page_add;
    void* current_block_next = reinterpret_cast<char*>(current_block) + bytes;

    int count = 0;
    while(current_block_next < page_add_end){
        block_nums++;
        setBlockNextPointer(current_block, current_block_next);
        current_block = current_block_next;
        current_block_next = reinterpret_cast<char*>(current_block) + bytes;
        // current_block_next = getNextBlock(current_block);
    }
    setBlockNextPointer(current_block, nullptr);
    block_nums++;

    new_span->block_non_allocated = block_nums;
    new_span->block_nums = block_nums;
    new_span->block_allocated = 0;
    new_span->block_list = new_span->page_add;
    new_span->is_use = true;
    new_span->next = list_head;
    new_span->pre = nullptr;
    
    if(list_head){
        list_head->pre = new_span;
    }
    center_list_[index].store(new_span, std::memory_order_relaxed);

    return new_span;
}



void CenterCache::deallocate(void* memory_list, size_t bytes){
    size_t index = SizeClass::getIndex(bytes);

    while(center_list_lock_[index].test_and_set(std::memory_order_acquire)){
        std::this_thread::yield();
    }

    log_deallocate_.begin();
    void* current_block = memory_list;
    void* current_block_next = getNextBlock(current_block);
    void* page_add = nullptr;
    while(current_block_next){
        // 通过右移 再左移获取 该block对应的页地址。
        page_add = reinterpret_cast<void*>(size_t(current_block) >> 12 << 12);
        Span* span = PageCache::getInstance().getPageSpan(page_add);

        // 头插法
        // 插入到span 的空闲block_list里
        setBlockNextPointer(current_block, span->block_list);
        span->block_list = current_block;

        span->block_allocated--;
        span->block_non_allocated++;

        current_block = current_block_next;
        current_block_next = getNextBlock(current_block);

        if(shouldReturn(span)){
            returnMemoryToPage(span, bytes);
        }
    }

    log_deallocate_.end();
    center_list_lock_[index].clear(std::memory_order_release);

}

inline bool CenterCache::shouldReturn(Span* (&span)){
    return span->block_non_allocated == span->block_nums;
}


void CenterCache::returnMemoryToPage(Span* span, size_t bytes){
    // 断开 span 在 center_list_[index] 队列的连接
    // 不做指针判断 就会出现 segment fault !!
    if(span->next){
        span->next->pre = span->pre;
    }
    // 不做指针判断 就会出现 segment fault !!
    if(span->pre){
        span->pre->next = span->next;
    }

    if(center_list_[SizeClass::getIndex(bytes)] == span){
        center_list_[SizeClass::getIndex(bytes)].store(span->next, std::memory_order_relaxed);
    }

    // 还原span的相关信息
    span->next = nullptr;
    span->pre = nullptr;
    span->block_list = nullptr;
    span->is_use = false;
    span->block_nums = 0;
    span->block_allocated = 0;
    span->block_non_allocated = 0;

    PageCache::getInstance().deallocate(span, bytes);
    
}





} //namespace MemoryPool