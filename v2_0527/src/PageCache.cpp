#include "../include/PageCache.h"
#include <cassert>
#include <sys/mman.h>


namespace MemoryPool
{

Span* PageCache::allocate(size_t page_nums){
    std::lock_guard<std::mutex> lock(mutex_);
    log_allocate_.begin();
    auto it = page_pool_.lower_bound(page_nums);
    assert(page_nums <= MAX_BYTES / PAGE_SIZE && page_nums >= 1);

    if(it == page_pool_.end()){
        getMemoryFromSys();
        it = page_pool_.lower_bound(page_nums);
    }

    if(it->first > page_nums){
        spiltSpan(it->first, page_nums);
    }

    Span* span = popSpanFromPageList(page_nums);
    span->is_use = true;
    log_allocate_.end();
    return span;

}


void PageCache::deallocate(Span* span, size_t bytes){
    std::lock_guard<std::mutex> lock(mutex_);
    log_deallocate_.begin();
    span->is_use = false;
    mergeSpan(span);
    // 合并玩span后，获取的到页面数量才是真实的
    size_t page_num = span->page_nums;
    pushSpanToPageList(span, page_num);
    log_deallocate_.end();
}

void PageCache::spiltSpan(size_t main_page, size_t sub_page){
    Span* main_span = popSpanFromPageList(main_page);
    
    assert(main_page > sub_page);

    Span* sub_span = new Span();
    sub_span->page_add = reinterpret_cast<char*>(main_span->page_add) 
                       + PAGE_SIZE * (main_page - sub_page);
    sub_span->page_nums = sub_page;

    main_span->page_nums = main_page - sub_page;

    pushSpanToPageList(sub_span, sub_page);
    pushSpanToPageList(main_span, main_page - sub_page);

}

void PageCache::mergeSpan(Span* span){
    void* pre_page = reinterpret_cast<char*>(span->page_add) - PAGE_SIZE;
    while(address_to_span_.find(pre_page) != address_to_span_.end()){
        Span* span_merged = address_to_span_[pre_page];

        if(span_merged->is_use){
            break;
        }

        size_t new_page_nums = span->page_nums + span_merged->page_nums;
        if(new_page_nums > MAX_PAGE_NUMS){
            break;
        }

        // 这个span_merged下的
        // 所有页对应的span都要修改
        // address_to_span_.erase(pre_page);
        for(size_t i = 0; i < span_merged->page_nums; ++i){
            void* span_merged_page = reinterpret_cast<char*>(span_merged->page_add) + i * PAGE_SIZE;
            address_to_span_[span_merged_page] = span;           
        }

        span->page_nums = new_page_nums;
        span->page_add = span_merged->page_add;
        // 25.05.29
        // 这个span_merged 可能在 page_pool的某个队列里
        // 直接delete会出事的
        // 要找到它对应的队列，并弹出去

        popSpanFromPageList(span_merged, span_merged->page_nums);
        // 
        delete span_merged;

        pre_page = reinterpret_cast<char*>(span->page_add) - PAGE_SIZE;
    }

    void* pos_page = reinterpret_cast<char*>(span->page_add) + PAGE_SIZE * span->page_nums;
    while(address_to_span_.find(pos_page) != address_to_span_.end()){
        Span* span_merged = address_to_span_[pos_page];

        if(span_merged->is_use){
            break;
        }

        size_t new_page_nums = span->page_nums + span_merged->page_nums;
        if(new_page_nums > MAX_PAGE_NUMS){
            break;
        }

        // address_to_span_.erase(pos_page);
        for(size_t i = 0; i < span_merged->page_nums; ++i){
            void* span_merged_page = reinterpret_cast<char*>(span_merged->page_add) + i * PAGE_SIZE;
            address_to_span_[span_merged_page] = span;           
        }

        span->page_nums = new_page_nums;

        // span->page_add = span_merged->page_add;


        popSpanFromPageList(span_merged, span_merged->page_nums);
        delete span_merged;

        pos_page = reinterpret_cast<char*>(span->page_add) + PAGE_SIZE * span->page_nums;

    }
}


void PageCache::pushSpanToPageList(Span* span, size_t page){
    assert(span->page_nums == page);

    auto it = page_pool_.find(page);
    if(it != page_pool_.end()){
        Span* head = it->second;
        span->pre = nullptr;
        span->next = head;
        head->pre = span;
    }
    page_pool_[page] = span;
    for(int i = 0; i < span->page_nums; i++){
        void* page_add = reinterpret_cast<char*>(span->page_add) 
                       + i * PAGE_SIZE;
        size_t page_alignment_check = size_t(page_add) & (PAGE_SIZE - 1);
        assert(page_alignment_check== 0);
        address_to_span_[page_add] = span;
    }
}

Span* PageCache::popSpanFromPageList(size_t page){
    auto it = page_pool_.find(page);
    if(it == page_pool_.end()) return nullptr;
    Span* pop_span = it->second;
    Span* head = pop_span->next;
    if(!head){
        page_pool_.erase(it->first);
        pop_span->pre = nullptr;
        return pop_span;
    }

    head->pre = nullptr;

    pop_span->next = nullptr;
    pop_span->pre = nullptr;

    page_pool_[page] = head;

    return pop_span;

}

void PageCache::popSpanFromPageList(Span* span, size_t page){
    if(page_pool_[page] == span){
        popSpanFromPageList(page);
        return;
    }
    Span* pre = span->pre;
    Span* next = span->next;
    if(pre){
        pre->next = next;
    }
    if(next){
        next->pre = pre;
    }
    // span->page_add = nullptr;
    // span->page_nums = 0;
    span->pre = nullptr;
    span->next = nullptr;
}


void PageCache::getMemoryFromSys(){
    void* page = mmap(nullptr, MAX_PAGE_NUMS * PAGE_SIZE, 
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    Span* new_span = new Span();
    new_span->page_add = page;
    new_span->page_nums = MAX_PAGE_NUMS;
    pushSpanToPageList(new_span, MAX_PAGE_NUMS);
}


    
} //namespace MemoryPool