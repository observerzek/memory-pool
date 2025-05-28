#include "../include/PageCache.h"
#include <cassert>
#include <sys/mman.h>


namespace MemoryPool
{

Span* PageCache::allocate(size_t page_nums){
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
    return span;

}


void PageCache::deallocate(Span* span, size_t bytes){
    span->is_use = false;
    size_t page_num = span->page_nums;
    mergeSpan(span);
    pushSpanToPageList(span, page_num);
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

        size_t new_page_nums = span->page_nums + span_merged->page_nums;
        if(new_page_nums > MAX_PAGE_NUMS){
            break;
        }

        address_to_span_.erase(pre_page);

        span->page_nums = new_page_nums;
        span->page_add = span_merged->page_add;
        delete span_merged;

        pre_page = reinterpret_cast<char*>(span->page_add) - PAGE_SIZE;
    }

    void* pos_page = reinterpret_cast<char*>(span->page_add) + PAGE_SIZE * span->page_nums;
    while(address_to_span_.find(pos_page) != address_to_span_.end()){
        Span* span_merged = address_to_span_[pos_page];

        size_t new_page_nums = span->page_nums + span_merged->page_nums;
        if(new_page_nums > MAX_PAGE_NUMS){
            break;
        }

        address_to_span_.erase(pos_page);

        span->page_nums = new_page_nums;
        span->page_add = span_merged->page_add;
        delete span_merged;

        pos_page = reinterpret_cast<char*>(span->page_add) + PAGE_SIZE * span->page_nums;

    }
}


void PageCache::pushSpanToPageList(Span* span, size_t page){
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
        return pop_span;
    }

    head->pre = nullptr;
    pop_span->next = nullptr;
    page_pool_[page] = head;

    return pop_span;

}



void PageCache::getMemoryFromSys(){
    void* page = mmap(nullptr, MAX_PAGE_NUMS, 
                      PROT_READ | PROT_WRITE,
                      MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    Span* new_span = new Span();
    new_span->page_add = page;

    pushSpanToPageList(new_span, MAX_PAGE_NUMS);
}


    
} //namespace MemoryPool