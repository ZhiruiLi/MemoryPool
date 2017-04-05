//
// Created by Zhirui Li on 2017/4/4.
//

#include <cassert>
#include <stdexcept>
#include <cstdlib>
#include "MemoryPool.h"

namespace mm {

    namespace detail {

        using byte = MemoryPool::byte;
        using size_type = MemoryPool::size_type;
        using word = MemoryPool::word;

        static constexpr size_type wsize = 4;
        static constexpr size_type dwsize = 2 * wsize;

        auto castByteP(void* ptr) -> byte* {
            return static_cast<byte*>(ptr);
        }

        auto castWordP(void* ptr) -> word* {
            return static_cast<word*>(ptr);
        }

        auto getWordAt(void* ptr) -> word& {
            return *castWordP(ptr);
        }

        auto putWordAt(void* ptr, const size_type wd) {
            getWordAt(ptr) = wd;
        }

        auto pack(const word size, const word allocBits) -> word {
            return size | allocBits;
        }

        auto getSize(void* headPtr) -> size_type {
            return static_cast<size_type>(*castWordP(headPtr) & static_cast<word>(~0x7));
        }

        auto getAllocBit(void* headPtr) -> bool {
            return (*castWordP(headPtr) & static_cast<word>(0x1)) != 0;
        }

        auto getHeadAddr(void* blockPtr) -> void* {
            return castByteP(blockPtr) - wsize;
        }

        auto getFootAddr(void* blockPtr) -> void* {
            return castByteP(blockPtr) - wsize
                   + getSize(getHeadAddr(blockPtr)) - wsize;
        }

        auto getNextBlock(void* blockPtr) -> void* {
            return castByteP(blockPtr) + getSize(getHeadAddr(blockPtr));
        }

        auto getPrevBlock(void* blockPtr) -> void* {
            auto prevFoot = castByteP(blockPtr) - dwsize;
            return castByteP(blockPtr) - getSize(prevFoot);
        }

        auto mergeFreeBlocks(void* blockPtr) -> void* {
            const auto prevBlock = getPrevBlock(blockPtr);
            const auto nextBlock = getNextBlock(blockPtr);
            const auto prevAlloc = getAllocBit(getHeadAddr(prevBlock));
            const auto nextAlloc = getAllocBit(getHeadAddr(nextBlock));
            if (prevAlloc && nextAlloc) {
                return blockPtr;
            } else if (nextAlloc) { // prev block is free
                const auto size = getSize(getHeadAddr(prevBlock))
                                  + getSize(getHeadAddr(blockPtr));
                putWordAt(getHeadAddr(prevBlock), pack(size, 0));
                putWordAt(getFootAddr(prevBlock), pack(size, 0));
                return prevBlock;
            } else if (prevAlloc) { // next block is free
                const auto size = getSize(getHeadAddr(nextBlock))
                                  + getSize(getHeadAddr(blockPtr));
                putWordAt(getHeadAddr(blockPtr), pack(size, 0));
                putWordAt(getFootAddr(blockPtr), pack(size, 0));
                return blockPtr;
            } else {
                const auto size = getSize(getHeadAddr(blockPtr))
                                  + getSize(getHeadAddr(prevBlock))
                                  + getSize(getHeadAddr(nextBlock));
                putWordAt(getHeadAddr(prevBlock), pack(size, 0));
                putWordAt(getFootAddr(prevBlock), pack(size, 0));
                return prevBlock;
            }
        }
    }

    MemoryPool::RowMemoryHandler::RowMemoryHandler(size_type maxMemorySize)
            : pool_(detail::castByteP(std::malloc(maxMemorySize)))
            , maxAddr_(pool_ + maxMemorySize)
            , brk_(pool_) {
        if (pool_ == nullptr) {
            throw std::runtime_error("MemoryPool: Can not allocate enough memory");
        }
    }

    MemoryPool::RowMemoryHandler::~RowMemoryHandler() {
        std::free(pool_);
    }

    auto MemoryPool::RowMemoryHandler::sbrk(int incr) -> void* {
        auto oldBrk = brk_;
        if ((incr < 0) || ((brk_ + incr) > maxAddr_)) {
            return nullptr;
        }
        brk_ += incr;
        return oldBrk;
    }

    MemoryPool::MemoryPool(size_type maxPoolSize): handler_(maxPoolSize) {
        using namespace detail;
        if ((poolHead_ = handler_.sbrk(4 * wsize)) == nullptr) {
            throw std::runtime_error("MemoryPool: Pool size is too small");
        }
        auto putAtOffset = [this](int offset, size_type sz, bool isAlloc) {
            putWordAt(castByteP(this->poolHead_) + (offset * wsize),
                      pack(sz, isAlloc?1:0));
        };
        putAtOffset(0, 0, false);           // padding block
        putAtOffset(1, dwsize, true);       // begin prologue block
        putAtOffset(2, dwsize, true);       // end prologue block
        putAtOffset(3, 0, true);            // epilogue block
        poolHead_ = castByteP(poolHead_) + (2 * wsize);
    }

    MemoryPool::~MemoryPool() { }

    auto MemoryPool::malloc(size_type size) -> void* {
        using namespace detail;
        if (size == 0) {
            return nullptr;
        }
        // need size: must be at least 4 words, and must double word aligned
        const auto needSz = size <= dwsize ?
                            4 * wsize :
                            ((size + (dwsize - 1)) / dwsize) * dwsize + 2 * wsize;
        auto bp = findFirstFit(needSz);
        if (bp == nullptr) {
            bp = extendPool(needSz / wsize);
        }
        if (bp == nullptr) {
            return nullptr;
        }
        return splitAndPlace(bp, needSz);
    }

    auto MemoryPool::findFirstFit(MemoryPool::size_type size) -> void* {
        using namespace detail;
        const auto top = handler_.sbrk(0);
        for (auto ptr = poolHead_; ptr < top; ) {
            if (!getAllocBit(getHeadAddr(ptr)) && getSize(getHeadAddr(ptr)) >= size) {
                return ptr;
            }
            ptr = getNextBlock(ptr);
        }
        return nullptr;
    }

    auto MemoryPool::splitAndPlace(void* blockPtr, MemoryPool::size_type needSize) -> void* {
        using namespace detail;
        const auto totalSize = getSize(getHeadAddr(blockPtr));
        assert(needSize % dwsize == 0);
        assert(totalSize % dwsize == 0);
        assert(totalSize >= needSize);
        assert(!getAllocBit(getHeadAddr(blockPtr)));
        if (totalSize <= needSize + dwsize) {
            putWordAt(getHeadAddr(blockPtr), pack(totalSize, 1));
            putWordAt(getFootAddr(blockPtr), pack(totalSize, 1));
        } else {
            putWordAt(getHeadAddr(blockPtr), pack(needSize, 1));
            putWordAt(getFootAddr(blockPtr), pack(needSize, 1));
            const auto nextBlock = getNextBlock(blockPtr);
            putWordAt(getHeadAddr(nextBlock), pack(totalSize - needSize, 0));
            putWordAt(getFootAddr(nextBlock), pack(totalSize - needSize, 0));
        }
        return blockPtr;
    }

    auto MemoryPool::free(void* ptr) -> void {
        if (ptr == nullptr) { return; }
        using namespace detail;
        const auto size = getSize(getHeadAddr(ptr));
        putWordAt(getHeadAddr(ptr), pack(size, 0));
        putWordAt(getFootAddr(ptr), pack(size, 0));
        mergeFreeBlocks(ptr);
    }

    auto MemoryPool::extendPool(MemoryPool::size_type words) -> void* {
        using namespace detail;
        size_type bytes = ((words % 2 == 0) ? words : (words + 1)) * wsize;
        byte* bp = castByteP(handler_.sbrk(bytes));
        if (bp == nullptr) {
            return nullptr;
        }
        putWordAt(getHeadAddr(bp), pack(bytes, 0));             // set new block head size
        putWordAt(getFootAddr(bp), pack(bytes, 0));             // set new block foot size
        putWordAt(getHeadAddr(getNextBlock(bp)), pack(0, 1));   // new epilogue block
        return mergeFreeBlocks(bp);
    }

}


