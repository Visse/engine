#pragma once

#include <cstdint>
#include <type_traits>

namespace Core 
{
    class Allocator;
    
    // a dynamic array for POD Types
    template< typename Type >
    struct Array {
        static_assert( std::is_trivial<Type>::value, "Array only supports trivial types!" );
        
        Array() = default;
        Array( const Array &copy );
        Array( Array &&move );
        Array( Allocator *allocator );
        ~Array();
        
        Array& operator = ( const Array &copy );
        Array& operator = ( Array &&move );
        
        Type& operator [] ( std::size_t index );
        const Type& operator [] ( std::size_t index ) const;
        
        Allocator *_allocator = nullptr;
        Type *_data = nullptr;
        std::size_t _size = 0,
                    _capasity = 0;
                    
    };
    
}