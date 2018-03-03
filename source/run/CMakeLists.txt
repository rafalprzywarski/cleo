include_directories("../core")

add_executable(cleo main.cpp)
target_link_libraries(cleo cleo_core)
