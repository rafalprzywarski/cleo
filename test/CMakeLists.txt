include_directories(${GoogleMock_INCLUDE_DIRS})
include_directories("../source/core")

add_executable(cleo_test
  equality_test.cpp
  eval_test.cpp
  fn_test.cpp
  hash_test.cpp
  list_test.cpp
  memory_test.cpp
  multimethod_test.cpp
  print_test.cpp
  reader_test.cpp
  small_vector_test.cpp
  value_test.cpp
  var_test.cpp
  main.cpp
)
target_link_libraries(cleo_test cleo_core ${Boost_LIBRARIES} gmock pthread)
