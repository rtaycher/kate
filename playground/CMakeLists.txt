# testing project inside kate, just for experiments
project (kateplayground)

# includes
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

# our sources
set (kateplayground_srcs
  katetextbuffer.cpp
  katetextblock.cpp
  katetextcursor.cpp
  katetextrange.cpp
)

# testing library
kde4_add_library (kateplayground ${kateplayground_srcs})
