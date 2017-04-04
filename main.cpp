//
// Created by Zhirui Li on 2017/4/4.
//

#include <iostream>
#include <cassert>
#include "MemoryPool.h"

using namespace std;

void test1() {
    mm::MemoryPool pool(4 * 4 + 16 + 2 * 4);
    void* p1 = nullptr;
    p1 = pool.malloc(32);
    assert(p1 == nullptr);
    p1 = pool.malloc(17);
    assert(p1 == nullptr);
    p1 = pool.malloc(16);
    assert(p1 != nullptr);
    void* p2 = nullptr;
    p2 = pool.malloc(1);
    assert(p2 == nullptr);
    pool.free(p1);
    p1 = nullptr;
    p2 = pool.malloc(16);
    assert(p2 != nullptr);
    pool.free(p2);
}

void test2() {
    mm::MemoryPool pool(4 * 4 + (16 + 2 * 4) * 2);
    void* p1 = nullptr;
    void* p2 = nullptr;
    p1 = pool.malloc(41);
    assert(p1 == nullptr);
    p1 = pool.malloc(40);
    assert(p1 != nullptr);
    p2 = pool.malloc(1);
    assert(p2 == nullptr);
    pool.free(p1);
    p1 = pool.malloc(16);
    assert(p1 != nullptr);
    p2 = pool.malloc(17);
    assert(p2 == nullptr);
    p2 = pool.malloc(16);
    assert(p2 != nullptr);
    pool.free(p1);
    p1 = nullptr;
    pool.free(p2);
    p2 = nullptr;
    p1 = pool.malloc(40);
    assert(p1 != nullptr);
    pool.free(p2);
}

void test3() {
    try {
        mm::MemoryPool pool(1);
        assert(false);
    } catch (runtime_error e) {
        assert(true);
    }
}

int main() {
    test1();
    test2();
    test3();
    cout << "All testes pass" << endl;
    return 0;
}

