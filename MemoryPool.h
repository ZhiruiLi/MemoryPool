//
// Created by Zhirui Li on 2017/4/4.
//

#ifndef MEMORYPOOL_MEMORYPOOL_H
#define MEMORYPOOL_MEMORYPOOL_H

#include <cstdint>

namespace mm {

    class MemoryPool final {
    public:
        using size_type = uint32_t;
        using byte = unsigned char;
        using word = uint32_t;
        MemoryPool(size_type maxPoolSize);
        ~MemoryPool();
        MemoryPool(const MemoryPool&) = delete;
        auto operator=(const MemoryPool&) = delete;
        auto malloc(size_type size) -> void*;
        auto free(void* ptr) -> void;

    private:
        class RowMemoryHandler {
        public:
            RowMemoryHandler(size_type maxMemorySize);
            ~RowMemoryHandler();
            RowMemoryHandler(const RowMemoryHandler&) = delete;
            auto operator=(const RowMemoryHandler&) = delete;
            auto sbrk(int incr) -> void*;
        private:
            byte* const pool_;
            byte* const maxAddr_;
            byte* brk_;
        };
        auto findFirstFit(size_type size) -> void*;
        auto splitAndPlace(void *blockPtr, size_type size) -> void*;
        auto extendPool(size_type words) -> void*;
        void* poolHead_ = nullptr;
        RowMemoryHandler handler_;
    };
}

#endif //MEMORYPOOL_MEMORYPOOL_H
