#pragma once
#include <map>
#include "utility.h"
#include <mutex>
#include <memory>

namespace MemoryPool
{


class PageCache{
private:
    // struct PageNode{
    //     std::shared_ptr<PageNode> next;
    //     size_t page_nums;
    //     void* page_add;
    // };

    // using NodePtr = std::shared_ptr<PageNode>;

    // 页面数量 到 span 的映射
    // 1 <= size_t <= 64 (256 * 1024 / 4096)
    std::map<size_t, Span*> page_pool_;
    // 根据页地址 到 该页对应的 span结点 的映射
    std::map<void*, Span*> address_to_span_;
    std::mutex mutex_;

public:
    // 函数PageCache前忘记加static
    // 导致 PageCache::static调用失败
    static PageCache& getInstance(){
        static PageCache page_cache;
        return page_cache;
    }

    Span* allocate(size_t page_nums);  
    void  deallocate(Span* span, size_t bytes);

    Span* getPageSpan(void* page_add){
        return address_to_span_[page_add];
    }

private:
    PageCache() = default;

    void getMemoryFromSys();

    void spiltSpan(size_t main_page,size_t sub_page);

    void mergeSpan(Span* span);

    void pushSpanToPageList(Span* span, size_t page);

    Span* popSpanFromPageList(size_t page);

};




} // namespace MemoryPool