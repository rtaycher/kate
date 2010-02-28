# test executable for encoding
kde4_add_executable(kateencodingtest NOGUI kateencodingtest.cpp)
target_link_libraries(kateencodingtest kateplayground ${KDE4_KTEXTEDITOR_LIBS} ${QT_QTTEST_LIBRARY} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS} )

# test macro for encoding tests
MACRO(KATE_ENCODING_TEST _encoding _testname)
  ADD_TEST (${_testname}_create kateencodingtest ${_encoding} ${CMAKE_CURRENT_SOURCE_DIR}/${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname} )
  ADD_TEST (${_testname}_diff ${CMAKE_COMMAND} -E compare_files ${CMAKE_CURRENT_SOURCE_DIR}/${_testname} ${CMAKE_CURRENT_BINARY_DIR}/${_testname} )
ENDMACRO(KATE_ENCODING_TEST)

# add tests

# this file is utf-8, simple
KATE_ENCODING_TEST ("utf-8" "utf8.txt")

# this file is latin15, but fallback should work!
KATE_ENCODING_TEST ("utf-8" "latin15.txt")
