#include <iostream>
#include <utility>
#include <vector>
#include <queue>
#include <cstdlib>

const std::size_t DEFAULT_POOLS_AMOUNT = 1;
const std::size_t DEFAULT_CHUNKS_PER_BLOCK = 1000000;

template <class T>
class PoolAllocator {
public:

    typedef T value_type;
    typedef T* pointer;
    typedef std::size_t size_type;

private:

    struct FreeChunks {

        FreeChunks(void* start, size_type size): size(size), start(start), next(nullptr), prev(nullptr) {}

        size_type size;
        void* start;
        FreeChunks* next;
        FreeChunks* prev;
    };

    struct Block {
        explicit Block(void* start): start(start) {}

        void* start;
        Block* next;
    };

public:

    explicit PoolAllocator(size_type chunks_per_block = DEFAULT_CHUNKS_PER_BLOCK, size_type blocks_amount = DEFAULT_POOLS_AMOUNT):
    chunks_per_block(chunks_per_block), blocks_amount(blocks_amount) {
        first_chunk = new FreeChunks(malloc(sizeof(value_type) * chunks_per_block), chunks_per_block);
        current_chunk = first_chunk;
        FreeChunks* chunk = current_chunk;
        first_block = new Block(first_chunk->start);
        Block* block = first_block;
        FreeChunks* help;
        for (int i = 0; i < blocks_amount - 1; ++i) {
            chunk->next = new FreeChunks(malloc(sizeof(value_type) * chunks_per_block), chunks_per_block);
            block->next = new Block(chunk->next->start);
            block = block->next;
            help = chunk;
            chunk = chunk->next;
            chunk->prev = help;
        }
        chunk->next = nullptr;
        block->next = nullptr;
    }

    PoolAllocator(const PoolAllocator<T>& other): chunks_per_block(other.chunks_per_block), blocks_amount(other.blocks_amount) {
        first_chunk = new FreeChunks(malloc(sizeof(value_type) * other.first_chunk->size), other.first_chunk->size);
        current_chunk = first_chunk;
        FreeChunks* chunk = current_chunk;
        FreeChunks* iter_chunk = other.current_chunk;
        FreeChunks* help;
        while (iter_chunk->next) {
            chunk->next = new FreeChunks(malloc(sizeof(value_type) * iter_chunk->next->size), iter_chunk->next->size);
            iter_chunk = iter_chunk->next;
            help = chunk;
            chunk = chunk->next;
            chunk->prev = help;
        }
        chunk->next = nullptr;

        first_block = new Block(malloc(sizeof(value_type) * chunks_per_block));
        Block* block = first_block;
        Block* iter_block = other.first_block;
        while (iter_block->next) {
            block->next = new Block(malloc(sizeof(value_type) * chunks_per_block));
            block = block->next;
            iter_block = iter_block->next;
        }
        block->next = nullptr;
    }

    ~PoolAllocator() {
        Block* block = first_block;
        while (block) {
            free(block->start);
            block = block->next;
        }
    }

    void swap(PoolAllocator<T>& other) {
        std::swap(chunks_per_block, other.chunks_per_block);
        std::swap(blocks_amount, other.blocks_amount);
        std::swap(current_chunk, other.current_chunk);
        std::swap(first_chunk, other.first_chunk);
        std::swap(first_block, other.first_block);
    }

    PoolAllocator& operator=(PoolAllocator<T> other) {
        (*this).swap(other);
        return *this;
    }

    pointer allocate(size_type size) {
        FreeChunks* chunk = current_chunk;
        while (chunk) {
            if (chunk->size > size) {
                void* start = chunk->start;
                chunk->start = reinterpret_cast<void*>(reinterpret_cast<char*>(chunk->start) + sizeof(value_type) * size);
                chunk->size -= size;
                return reinterpret_cast<pointer>(start);
            } else if (chunk->size == size) {
                if (current_chunk == chunk) {
                    current_chunk = current_chunk->next;
                    if (current_chunk) {
                        current_chunk->prev = nullptr;
                    }
                } else {
                    FreeChunks* prev = chunk->prev;
                    prev->next = chunk->next;
                    if (chunk->next) {
                        chunk->next->prev = prev;
                    }
                }
                return reinterpret_cast<pointer>(chunk->start);
            } else {
                chunk = chunk->next;
            }
        }
        throw std::bad_alloc {};
    }

    void deallocate(pointer ptr, size_type size) {
        auto* new_chunk = new FreeChunks(reinterpret_cast<void*>(ptr), size);
        if (!current_chunk) {
            current_chunk = new_chunk;
        } else {
            if (new_chunk->start < current_chunk->start) {
                if (reinterpret_cast<char*>(new_chunk->start) + sizeof(value_type) * size == reinterpret_cast<char*>(current_chunk->start)) {
                    current_chunk->size += size;
                    current_chunk->start = reinterpret_cast<void*>(ptr);
                } else {
                    new_chunk->next = current_chunk;
                    current_chunk->prev = new_chunk;
                    current_chunk = new_chunk;
                }
            } else {
                FreeChunks* chunk = current_chunk;
                while (chunk->next) {
                    if (new_chunk->start > chunk->start && new_chunk->start < chunk->next->start) {
                        if (reinterpret_cast<char*>(chunk->start) + sizeof(value_type) * chunk->size == reinterpret_cast<char*>(new_chunk->start)
                            && reinterpret_cast<char*>(new_chunk->start) + sizeof(value_type) * new_chunk->size == reinterpret_cast<char*>(chunk->next->start)) {
                            chunk->size += new_chunk->size + chunk->next->size;
                            if (chunk->next->next) {
                                chunk->next->next->prev = chunk;
                            }
                            chunk->next = chunk->next->next;
                        } else if (reinterpret_cast<char*>(chunk->start) + sizeof(value_type) * chunk->size == reinterpret_cast<char*>(new_chunk->start)) {
                            chunk->size += new_chunk->size;
                        } else if (reinterpret_cast<char*>(new_chunk->start) + sizeof(value_type) * new_chunk->size == reinterpret_cast<char*>(chunk->next->start)) {
                            chunk->next->start = new_chunk->start;
                            chunk->next->size += new_chunk->size;
                        } else {
                            new_chunk->next = chunk->next;
                            new_chunk->prev = chunk->prev;
                            chunk->next = new_chunk;
                            new_chunk->next->prev = new_chunk;
                        }
                        return;
                    }
                }
                if (reinterpret_cast<char*>(chunk->start) + sizeof(value_type) * chunk->size == reinterpret_cast<char*>(new_chunk->start)) {
                    chunk->size += size;
                } else {
                    chunk->next = new_chunk;
                    new_chunk->prev = chunk;
                }
            }
        }
    }

    bool operator==(const PoolAllocator<T>& other) {
        if (chunks_per_block != other.chunks_per_block || blocks_amount != other.blocks_amount) {
            return false;
        }
        FreeChunks* chunk = current_chunk;
        FreeChunks* other_chunk = other.current_chunk;
        while (chunk && other_chunk) {
            if (chunk->size != other_chunk->size) {
                return false;
            }
            chunk = chunk->next;
            other_chunk = other_chunk->next;
        }
        return chunk == other_chunk;
    }

    bool operator!=(const PoolAllocator<T>& other) {
        return !(*this == other);
    }

private:

    size_type chunks_per_block;
    size_type blocks_amount;
    FreeChunks* current_chunk;
    FreeChunks* first_chunk;
    Block* first_block;

};
