include_directories("../core")
add_definitions("-O3")
add_executable(bindump main.cpp other.cpp)
target_link_libraries(bindump cleo_core pthread)
