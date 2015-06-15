#include "catch.hpp"

#include "core/Array.h"



TEST_CASE( "[Core][Array]" )
{
    Core::initAllocators();
    
    Core::Allocator *allocator = Core::getDefaultAllocator();
    
    
    using Core::Array;
    using namespace Core::array;
    
    SECTION( "Array Construction / Destruction" ) {
        Array<int> array( Core::getDefaultAllocator() );
        
        resize( array, 10 );
        REQUIRE( size(array) == 10 );
    }
    
    
    
    
    
    Core::destroyAllocators();
}