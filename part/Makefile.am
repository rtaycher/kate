
lib_LTLIBRARIES = libkantpart.la
libkantpart_la_SOURCES = dummy.cpp
libkantpart_la_LDFLAGS = $(all_libraries) -version-info 1:0:0 -no-undefined
libkantpart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS) ../document/libkantdocument.la ../view/libkantview.la

dummy.cpp:
	echo > dummy.cpp

INCLUDES= $(all_includes)

METASOURCES = AUTO
