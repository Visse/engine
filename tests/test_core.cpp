#include "catch.hpp"

#include "core/Allocator.h"


TEST_CASE( "[Core][ScrapAllocator" )
{
    Core::initAllocators();
    
    char BUFFER[1024];
    
    Core::Allocator *allocator = Core::createScrapAllocator( BUFFER, 1024, nullptr );
    
    
    SECTION( "Repeated single allocation / deallocation" )
    {
        for( int i=0; i < 100; ++i ) {
            for( int s=1; s < 1000; ++s ) {
                void *ptr = allocator->allocate( s, 1 );
                REQUIRE( ptr != nullptr );
                allocator->free( ptr );
            }
        }
    }
    
    SECTION( "Repeated multiple allocations / deallocation" )
    {
        for( int i=0; i < 100; ++i ) {
            void *ptrs[30];
            // allocate approximate 435 bytes + headers
            for( int s=1; s <= 30; ++s ) {
                ptrs[s-1] = allocator->allocate( s, 1 );
            }
            // free in reverse order, to see if it tracks deallocation correcly
            for( int s=30; s > 0; --s ) {
                allocator->free( ptrs[s-1] );
            }
        }
    }
    
    Core::destroyAllocators();
}