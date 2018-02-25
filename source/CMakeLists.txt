if(APPLE)
	add_definitions("-Wno-unused-local-typedef")
endif(APPLE)
include_directories(".")

add_subdirectory("bindump")
add_subdirectory("core")
add_subdirectory("extlib")
add_subdirectory("repl")
