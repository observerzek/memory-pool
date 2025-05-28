#include "include/MemoryPool.h"
#include <iostream>


using std::cout;
using std::endl;


void test(){
    size_t TEST_TIMES = 2e5;
    for(size_t i = 0; i < TEST_TIMES; ++i){
        size_t t = 8;
        int* p = reinterpret_cast<int*>(MemoryPool::allocate(t));
        *p = i;
        cout << *p << endl;
    }

}



int main(){

    test();
    return 0;
}