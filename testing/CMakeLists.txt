# testing project inside kate, just for experiments
project (katetesting)

# includes
include_directories (${CMAKE_CURRENT_SOURCE_DIR})

# our sources
set (katepart_testing_srcs
  katetextbuffer.cpp
)

# testing library
kde4_add_library (kateparttesting ${katepart_testing_srcs})
