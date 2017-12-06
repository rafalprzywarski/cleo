include_directories("../core")

add_library(cleo_core
  cleo/equality.cpp
  cleo/eval.cpp
  cleo/global.cpp
  cleo/hash.cpp
  cleo/list.cpp
  cleo/memory.cpp
  cleo/multimethod.cpp
  cleo/print.cpp
  cleo/small_vector.cpp
  cleo/value.cpp
  cleo/var.cpp
)
