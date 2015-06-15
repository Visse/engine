#include "Containers.h"
#include "Allocator.h"
#include "Assume.h"

#include <cstring>
#include <utility>
#include <algorithm>

namespace Core
{
    template< typename Type >
    Array<Type>::Array( const Array &copy )
    {
        *this = copy;
    }
    
    template< typename Type >
    Array<Type>::Array( Array &&move )
    {
        _allocator = move._allocator;
        _data = move._data;
        _size = move._size;
        _capasity = move._capasity;
        
        move._allocator = nullptr;
        move._data = nullptr;
        move._size = 0;
        move._capasity = 0;
    }
    
    template< typename Type >
    Array<Type>::Array( Allocator *allocator ) :
        _allocator(allocator)
    {
    }
    
    template< typename Type >
    Array<Type>::~Array()
    {
        if( _allocator && _data ) {
            _allocator->free( _data );
        }
    }

    template< typename Type >
    Array<Type>& Array<Type>::operator = ( const Array &copy )
    {
        ASSUME_TRUE( copy._allocator != nullptr );
        
        if( this == &copy ) return;
        if( _allocator && _data ) {
            _allocator->free( _data );
        }
        
        _allocator = copy._allocator;
        _size = copy._size;
        _capasity = _size;
        
        
        _data = _allocator->allocate( _size, alignof(Type) );
        memcpy( _data, copy._data, sizeof(Type)*_size );
        
        return *this;
    }
    
    template< typename Type >
    Array<Type>& Array<Type>::operator = ( Array &&move )
    {
        if( _allocator && _data ) {
            _allocator->free( _data );
        }
        
        _allocator = move._allocator;
        _data = move._data;
        _size = move._size;
        _capasity = move._capasity;
        
        move._allocator = nullptr;
        move._data = nullptr;
        move._size = 0;
        move._capasity = 0;
        
        return *this;
    }
    
    template< typename Type >
    Type& Array<Type>::operator [] ( std::size_t index )
    {
        ASSUME_TRUE( index < _size );
        return _data[index];
    }
    
    template< typename Type >
    const Type& Array<Type>::operator [] ( std::size_t index ) const
    {
        ASSUME_TRUE( index < _size );
        return _data[index];
    }
    
    
    namespace array 
    {
        template< typename Type >
        bool isNull( Array<Type> &array )
        {
            return array._data == nullptr;
        }
        
        template< typename Type >
        std::size_t size( const Array<Type> &array )
        {
            return array._size;
        }
        
        template< typename Type >
        std::size_t _capasity( const Array<Type> &array )
        {
            return array._capasity;
        }
        
        template< typename Type >
        Type* begin( Array<Type> &array )
        {
            return array._data;
        }
        
        template< typename Type >
        Type* end( Array<Type> &array )
        {
            return array._data + array._size;
        }
        
        template< typename Type >
        const Type* begin( const Array<Type> &array )
        {
            return array._data;
        }
        
        template< typename Type >
        const Type* end( const Array<Type> &array )
        {
            return array._data + array._size;
        }
        
        /* Resizes the array to size elements, 
         * if the new size is bigger than the old one, 
         * initilize the rest of the memory to '\0'
         */
        template< typename Type >
        void resize( Array<Type> &array, std::size_t size )
        {
            ASSUME_TRUE( array._allocator != nullptr );
            Type *newData = reinterpret_cast<Type*>( array._allocator->allocate(size*sizeof(Type), alignof(Type)) );
            
            std::size_t copySize = size < array._size ? size : array._size;
            std::size_t initSize = size < array._size ? 0 : size - array._size;
            
            std::memcpy( newData, array._data, copySize*sizeof(Type) );
            std::memset( newData+copySize, 0, initSize * sizeof(Type) );
            
            array._allocator->free( array._data );
            array._data = newData;
            array._size = size;
            array._capasity = size;
        }
        
        /* Resizes the array to size elements, 
         * if the new size is bigger than the old one, 
         * initilize the rest of the elements to value
         */
        template< typename Type >
        void resize( Array<Type> &array, std::size_t size, const Type &value )
        {
            ASSUME_TRUE( array._allocator != nullptr );
            Type *newData = array._allocator->allocate( size * sizeof(Type) );
            
            std::size_t copySize = size < array->_size ? size : array->_size;
            std::size_t initSize = size < array->_size ? 0 : size - array->_size;
            
            std::memcpy( newData, array._data, copySize*sizeof(Type) );
            for( std::size_t i = 0; i < initSize; ++i ) {
                newData[i + copySize] = value;
            }
            
            array._allocator->free( array._data );
            array._data = newData;
            array._size = size;
            array._capasity = size;
        }
        
        
        /* Reserves space for size elements
         * If the new size is smaller than the old capasity, do nothing
         * If you want to shrink the capasity, use trim
         */
        template< typename Type >
        void reserve( Array<Type> &array, std::size_t size )
        {
            ASSUME_TRUE( array._allocator != nullptr );
            
            // use trim to shrink the capasity
            if( size < array._capasity ) return;
            
            Type *newData = array._allocator->allocate( size * sizeof(Type) );
            
            std::memcpy( newData, array._data, array._size * sizeof(Type) );
            
            array._allocator->free( array._data );
            array._data = newData;
            array._capasity = size;
        }
        
        /* Trim space for the array to size + excess
         * Set excess to 0 if capasity should be the same as the current size
         */
        template< typename Type >
        void trim( Array<Type> &array, std::size_t excess = 0 )
        {
            ASSUME_TRUE( array._allocator != nullptr );
            
            std::size_t size = array._size + excess;
            
            Type *newData = array._allocator->allocate( size * sizeof(Type) );
            std::memcpy( newData, array._data, array._size * sizeof(Type) );
            
            array._allocator->free( array._data );
            array._data = newData;
            array._capasity = size;
        }
        
        template< typename Type >
        void sort( Array<Type> &array )
        {
            if( isNull(array) ) return;
            std::sort( begin(array), end(array) );
        }
        
        template< typename Type >
        void pushBack( Array<Type> &array, const Type &value )
        {
            if( array._capasity == array._size ) {
                reserve( array, array._capasity*2+10 );
            }
            
            array._data[array._size] = value;
            array._size++;
        }
        
        template< typename Type >
        Type popBack( Array<Type> &array )
        {
            ASSUME_TRUE( array._size > 0 );
            
            array._size--;
            return array._data[array._size];
        }
    }
    
}