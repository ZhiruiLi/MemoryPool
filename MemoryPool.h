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
        void* malloc(size_type size);
        void free(void* ptr);

    private:
        class RowMemoryHandler {
        public:
            RowMemoryHandler(size_type maxMemorySize);
            ~RowMemoryHandler();
            RowMemoryHandler(const RowMemoryHandler&) = delete;
            auto operator=(const RowMemoryHandler&) = delete;
            void* sbrk(int incr);
        private:
            byte* const pool_;
            byte* const maxAddr_;
            byte* brk_;
        };
        void* findFirstFit(size_type size);
        void* splitAndPlace(void *blockPtr, size_type size);
        void* extendPool(size_type words);
        void* poolHead_ = nullptr;
        RowMemoryHandler handler_;
    };
}

#endif //MEMORYPOOL_MEMORYPOOL_H
