# tests project inside kate,
project (katetests)

# includes
include_directories (${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_SOURCE_DIR}/playground)

# test macro for unit tests
MACRO(KATE_UNIT_TESTS)
  FOREACH(_testname ${ARGN})
    kde4_add_unit_test(${_testname} NOGUI ${_testname}.cpp)
    target_link_libraries(${_testname} kateplayground ${QT_QTTEST_LIBRARY} ktexteditor)
  ENDFOREACH(_testname)
ENDMACRO(KATE_UNIT_TESTS)

# unit tests
kate_unit_tests (katetextbuffertest)
