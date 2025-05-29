#include "include/MemoryPool.h"
#include <iostream>
#include <random>
#include <chrono>
#include <string>
#include <thread>
#include <vector>
#include <mutex>


using std::cout;
using std::endl;

class Timer{
private:
    std::chrono::time_point<std::chrono::high_resolution_clock> begin_;
    std::string name_;

public:
    Timer(const std::string &name){
        begin_ = std::chrono::high_resolution_clock::now();
        name_ = name;
    }

    void count(){
        double duration  = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - begin_).count() / 1000.0;
        cout << name_ << " experiment : " << duration << "ms" << endl;
    }



};


void warmMemoryPool(){
    Timer now("warm process");
    std::vector<std::pair<void*, size_t>> log;
    for(int i = 1; i < 1501; i++){
        for(int j = 0; j < 100; j++){
            size_t bytes = i * MemoryPool::ALIGNMENT;
            void* ptr = MemoryPool::allocate(bytes);
            log.push_back(std::pair<void*, size_t>(ptr, bytes));
        }
    }
    MemoryPool::ThreadPool::getInstance()->~ThreadPool();
    MemoryPool::CenterCache::getInstance().showDurationTime();
    MemoryPool::PageCache::getInstance().showDurationTime();
    now.count();
}


void test(){
    std::random_device rd;
    std::mt19937 gen(rd());
    std::mutex muxte;


    size_t TEST_TIMES = 2e5;

    auto function = [&](const std::string & name, bool use_pool){
        if(use_pool){
            MemoryPool::CenterCache::getInstance().resetDurationTime();
            MemoryPool::PageCache::getInstance().resetDurationTime();
        }
        std::vector<std::pair<void*, size_t>>log;
        log.reserve(TEST_TIMES);
        Timer now(name);
        for(size_t i = 0; i < TEST_TIMES; i++){
            size_t bytes = ((rand() % 1500) + 1) * MemoryPool::ALIGNMENT;
            void* ptr = use_pool ? MemoryPool::allocate(bytes) : new char[bytes]();
            log.push_back(std::pair<void*, size_t>(ptr, bytes));
            if((gen() % 100) < 90){
                if(log.size() >= 1){
                    auto it = log.back();
                    if(use_pool){
                        MemoryPool::deallocate(it.first, it.second);
                    }
                    else{
                        delete [] static_cast<char*>(it.first);
                    }
                    // log[index] = log.back();
                    log.pop_back();
                }
            }
        }
        for(auto &it : log){
            if(use_pool){
                MemoryPool::deallocate(it.first, it.second);
            }
            else{
                delete [] static_cast<char*>(it.first);
            }
        }
        std::lock_guard<std::mutex> lock(muxte);
        
        if(use_pool){
            MemoryPool::ThreadPool::getInstance()->~ThreadPool();
            MemoryPool::CenterCache::getInstance().showDurationTime();
            MemoryPool::PageCache::getInstance().showDurationTime();
        }
        now.count();
    };

    std::thread thread_1(function, "1", true);
    // std::thread thread_2(function, "2", true);
    // std::thread thread_3(function, "3", true);
    // std::thread thread_4(function, "4", true);

    std::thread thread_5(function, "5", false);
    // std::thread thread_6(function, "6", false);
    // std::thread thread_7(function, "7", false);
    // std::thread thread_8(function, "8", false);

    thread_1.join();
    // thread_2.join();
    // thread_3.join();
    // thread_4.join();
    thread_5.join();
    // thread_6.join();
    // thread_7.join();
    // thread_8.join();
}


void originalTest(){
    std::random_device rd;
    std::mt19937 gen(rd());

    size_t TEST_TIMES = 2e5;
    Timer now("original test");
    for(size_t i = 0; i < TEST_TIMES; ++i){
        size_t t = 128;
        int* p = new int();
        *p = i;
        if((gen() % 100) < 90){
            delete p;
        }
        // cout << *p << endl;
    }
    now.count();
}


int main(){
    std::thread warm(warmMemoryPool);
    warm.join();
    test();
    // originalTest();
    return 0;
}