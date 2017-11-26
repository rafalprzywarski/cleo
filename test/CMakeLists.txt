include_directories(${GoogleMock_INCLUDE_DIRS})
include_directories("../source/core")

add_executable(cleo_test
  equality_test.cpp
  list_test.cpp
  value_test.cpp
  main.cpp
)
target_link_libraries(cleo_test cleo_core ${Boost_LIBRARIES} gmock pthread)
