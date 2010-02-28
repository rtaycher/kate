# test executable for encoding
kde4_add_executable(kateencodingtest NOGUI kateencodingtest.cpp)
target_link_libraries(kateencodingtest kateplayground ${KDE4_KTEXTEDITOR_LIBS} ${QT_QTTEST_LIBRARY} ${KDE4_KIO_LIBS} ${KDE4_KPARTS_LIBS} )
