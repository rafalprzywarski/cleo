include_directories("../core")

add_library(cleo_core
  cleo/array_map.cpp
  cleo/atom.cpp
  cleo/clib.cpp
  cleo/equality.cpp
  cleo/error.cpp
  cleo/eval.cpp
  cleo/fn.cpp
  cleo/global.cpp
  cleo/hash.cpp
  cleo/list.cpp
  cleo/memory.cpp
  cleo/multimethod.cpp
  cleo/namespace.cpp
  cleo/persistent_hash_map.cpp
  cleo/print.cpp
  cleo/reader.cpp
  cleo/small_set.cpp
  cleo/small_vector.cpp
  cleo/util.cpp
  cleo/value.cpp
  cleo/var.cpp
)

target_link_libraries(cleo_core dl)

install(FILES cleo/cleo.core.cleo DESTINATION share/cleo/lib)
