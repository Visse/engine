

add_executable( test 
    tests.cpp
    test_core.cpp
    test_array.cpp
)

target_link_libraries( test core )