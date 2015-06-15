
add_definitions( -DUSE_DL_PREFIX=1 -DMSPACES=1 )
set_source_files_properties( dlmalloc.c PROPERTIES COMPILE_FLAGS -O3 )

add_library( core STATIC
            dlmalloc.c
            Allocator.cpp
            Assume.cpp
)