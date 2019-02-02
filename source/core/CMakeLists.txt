include_directories("../core")

add_library(cleo_core
  cleo/array.cpp
  cleo/array_map.cpp
  cleo/array_set.cpp
  cleo/atom.cpp
  cleo/bytecode_fn.cpp
  cleo/clib.cpp
  cleo/compile.cpp
  cleo/cons.cpp
  cleo/equality.cpp
  cleo/error.cpp
  cleo/eval.cpp
  cleo/global.cpp
  cleo/hash.cpp
  cleo/lazy_seq.cpp
  cleo/list.cpp
  cleo/memory.cpp
  cleo/multimethod.cpp
  cleo/namespace.cpp
  cleo/persistent_hash_map.cpp
  cleo/print.cpp
  cleo/reader.cpp
  cleo/util.cpp
  cleo/value.cpp
  cleo/var.cpp
  cleo/vm.cpp
)

target_link_libraries(cleo_core dl)

install(FILES cleo/cleo.core.cleo cleo/cleo.compiler.cleo DESTINATION share/cleo/lib)
