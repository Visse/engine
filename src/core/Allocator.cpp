#include "core/Assume.h"
#include "core/Allocator.h"

#include "dlmalloc.h"

#include <new>

namespace Core 
{
    namespace {
        static const std::size_t SCRAP_DEFAULT_BUFFER_SIZE = 2*1024*1024; // 2 MB
        static const std::size_t SCRAP_MIN_BUFFER_SIZE = 128; 
        
        inline void* pointerAdd( void *ptr, std::size_t amount ) {
            int8_t *tmp = static_cast<int8_t*> (ptr);
            tmp += amount;
            return static_cast<void*> (tmp);
        }
        inline void* pointerSub( void *ptr, std::size_t amount ) {
            int8_t *tmp = static_cast<int8_t*> (ptr);
            tmp -= amount;
            return static_cast<void*> (tmp);
        }

        inline void *alignPointer( void *ptr, std::size_t alignment ) {
            uintptr_t tmp = reinterpret_cast<uintptr_t>(ptr);
            
            std::size_t offset = tmp % alignment;
            if( offset > 0 ) offset = alignment - offset;
            
            return pointerAdd( ptr, offset );
        }
        
        inline std::size_t alignSize( std::size_t size, std::size_t alignment ) 
        {
            // round up to nearest alignment
            return ((size + alignment-1)/alignment) * alignment;
        }
        
        struct Header {
            uint32_t free : 1,
                     size : 31;
        };
        
        static const uint32_t HEADER_PADDING = 0;
        static const std::size_t MIN_ALIGNMENT= sizeof(uint32_t);
        
        
        void fillHeader( void *start, void *data, Header header )
        {
            ASSUME_TRUE( pointerAdd(start,sizeof(header)) <= data );
            
            uint32_t *tmp = static_cast<uint32_t*>( data );
            while( start < tmp ) {
                *tmp = HEADER_PADDING;
                tmp--;
            }
            
            *reinterpret_cast<Header*>(start) = header;
        }
        
        Header* findHeader( void *start, void *data )
        {
            ASSUME_TRUE( pointerAdd(start,sizeof(uint32_t)) <= data );
            
            uint32_t *tmp = reinterpret_cast<uint32_t*>(data);
            while( tmp[-1] == HEADER_PADDING && tmp > start ) tmp--;
            
            ASSUME_TRUE( reinterpret_cast<Header*>(tmp-1) >= start );
            return reinterpret_cast<Header*>(tmp-1);
        }
        
        void* findData( Header *header, std::size_t alignment )
        {
            return alignPointer( header+1, alignment );
        }
    }
    
    class SystemAllocator :
        public Allocator
    {
    public:
        virtual void* allocate( std::size_t size, std::size_t alignment )
        {
            return dlmemalign( alignment, size );
        }
        
        virtual void free( void *ptr )
        {
            dlfree( ptr );
        }
    };
    
    class HeapAllocator :
        public Allocator
    {
    public:
        HeapAllocator( void *heap, std::size_t size ) {
            /// @todo figure out the 'locked' parater to create_mspace
            mSpace = nullptr;
            if( heap ) {
                mSpace = create_mspace_with_base( heap, size, 0 );
            }
            if( !mSpace ) {
                mSpace = create_mspace( size, 0 );
            }
        }
        virtual ~HeapAllocator()
        {
            destroy_mspace( mSpace );
        }
        
        virtual void* allocate( std::size_t size, std::size_t alignment )
        {
            return mspace_memalign( mSpace, size, alignment );
        }
        
        virtual void free( void *ptr )
        {
            mspace_free( mSpace, ptr );
        }
        
    private:
        mspace mSpace;
    };
    
    class ScrapAllocator :
        public Allocator
    {
    public:
        ScrapAllocator( Allocator *backer, void *mem, std::size_t size ) :
            mBacker(backer),
            mOwnsBuffer(false)
        {
            if( size == 0 ) {
                size = SCRAP_DEFAULT_BUFFER_SIZE;
            }
            if( mem == nullptr && mBacker ) {
                mem = mBacker->allocate( size, MIN_ALIGNMENT );
                mOwnsBuffer = true;
            }
            
            if( mem == nullptr || size < SCRAP_MIN_BUFFER_SIZE ) {
                mem = nullptr;
                size = 0;
            }
            
            mBuffStart = mem;
            mBuffEnd = pointerAdd( mem, size );
            
            mAllocAt = mBuffStart;
            mFreeAt = mBuffStart;
            
            if( mBuffStart ) {
                Header header;
                    header.free = 1;
                    header.size = size;
                fillHeader( mem, pointerAdd(mem,sizeof(Header)), header );
            }
        }
        virtual ~ScrapAllocator()
        {
            if( mOwnsBuffer ) {
                mBacker->free( mBuffStart );
            }
        }
        
        virtual void* allocate( std::size_t size, std::size_t alignment )
        {
            void *result = allocateFromBuffer( size, alignment );
            if( !result && mBacker ) {
                return mBacker->allocate( size, alignment );
            }
            return result;
        }
        
        void* allocateFromBuffer( std::size_t size, std::size_t alignment ) 
        {
            if( mBuffStart == nullptr ) return nullptr;
            
            if( alignment < MIN_ALIGNMENT ) alignment = MIN_ALIGNMENT;
            size = alignSize( size, sizeof(uint32_t) );
            
            Header *header = reinterpret_cast<Header*>(mAllocAt);
            void *data = findData( header, alignment );
            
            void *end = pointerAdd( data, size );
            if( end > mBuffEnd ) {
                // mark the rest of the buffer as free
                header->free = 1;
                header->size = (uint8_t*)mBuffEnd - (uint8_t*)header;
                
                header = reinterpret_cast<Header*>(mBuffStart);
                data = findData( header, alignment );
                end = pointerAdd( data, size );
            }
            
            if( isInUse(end) ) {
                return nullptr;
            }
            
            header->free = 0;
            header->size = (uint8_t*)end - (uint8_t*)header;
            
            mAllocAt = end;
            
            return data;
        }
        
        virtual void free( void *ptr )
        {
            if( ptr < mBuffStart || ptr >= mBuffEnd ) {
                mBacker->free( ptr );
                return;
            }
            
            Header *header = findHeader( mBuffStart, ptr );
            header->free = 1;
            
            freeBufferSpace();
        }
        
        bool isInUse( void *ptr ) {
            if( mAllocAt == mFreeAt ) {
                return false;
            }
            if( mAllocAt < mFreeAt ) {
                return ptr < mAllocAt || ptr > mFreeAt;
            }
            return ptr > mFreeAt && ptr < mAllocAt;
        }
        
        void freeBufferSpace()
        {
            Header *header = reinterpret_cast<Header*>(mFreeAt);
            while( header->free ) {
                mFreeAt = pointerAdd( mFreeAt, header->size );
                if( mFreeAt == mAllocAt ) break;
                if( mFreeAt >= mBuffEnd ) mFreeAt = mBuffStart;
                
                header = reinterpret_cast<Header*>(mFreeAt);
            }
        }
        
    private:
        Allocator *mBacker;
        bool mOwnsBuffer;
        void *mBuffStart,
             *mBuffEnd,
             *mAllocAt,
             *mFreeAt;
    };
    
    namespace {
        struct GlobalAllocators {
            uint8_t BUFFER[ sizeof(SystemAllocator) + sizeof(ScrapAllocator) ];
            
            Allocator *defaultAllocator = nullptr;
            Allocator *scrapAllocator = nullptr;
            
            bool initilized = false;
        };
        static GlobalAllocators globalAllocators;
    }
    
    void initAllocators()
    {
        ASSUME_TRUE( globalAllocators.initilized == false );
        
        globalAllocators.defaultAllocator = new ((void*)globalAllocators.BUFFER) SystemAllocator;
        globalAllocators.scrapAllocator = new ((void*)(globalAllocators.BUFFER+sizeof(SystemAllocator))) ScrapAllocator( globalAllocators.defaultAllocator, nullptr, 0 );
        globalAllocators.initilized = true;
    }

    void destroyAllocators()
    {
        ASSUME_TRUE( globalAllocators.initilized == true );
        
        globalAllocators.scrapAllocator->~Allocator();
        globalAllocators.defaultAllocator->~Allocator();
        
        globalAllocators.initilized = false;
    }
    
    Allocator* getDefaultAllocator()
    {
        ASSUME_TRUE( globalAllocators.initilized == true );
        return globalAllocators.defaultAllocator;
    }
    
    Allocator* getScrapAllocator()
    {
        ASSUME_TRUE( globalAllocators.initilized == true );
        return globalAllocators.scrapAllocator;
    }
    
    Allocator* createScrapAllocator( void *heap, std::size_t size, Allocator *backer)
    {
        void *memory = nullptr;
        if( backer ) {
             memory = backer->allocate( sizeof(ScrapAllocator), alignof(ScrapAllocator) );
        }
        else {
            memory = getDefaultAllocator()->allocate( sizeof(ScrapAllocator), alignof(ScrapAllocator) );
        }
        
        if( memory == nullptr ) return nullptr;
        
        return new (memory) ScrapAllocator( backer, heap, size );
    }
}