#pragma once

#include <cstdint>

namespace Core 
{
    class Allocator {
        Allocator( const Allocator& ) = delete;
        Allocator& operator = ( const Allocator& ) = delete;
    public:
        Allocator() = default;
        virtual ~Allocator() {}
        
        virtual void* allocate( std::size_t size, std::size_t alignment ) = 0;
        virtual void free( void *ptr ) = 0;
    };
    
    void initAllocators();
    void destroyAllocators();
    
    Allocator* getDefaultAllocator();
    Allocator* getScrapAllocator();
    
    
    Allocator* createHeapAllocator( void *heap, std::size_t size );
    Allocator* createScrapAllocator( void *heap, std::size_t size, Allocator *backer );
}