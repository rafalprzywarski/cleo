include_directories("../core")

add_executable(cleo_repl main.cpp)
target_link_libraries(cleo_repl cleo_core readline pthread)

install(TARGETS cleo_repl RUNTIME DESTINATION share/cleo/bin)
