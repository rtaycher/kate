lib_LTLIBRARIES = libkantpart.la
libkantpart_la_SOURCES = kantpartview.cpp kantpartdocument.cpp kantpartfactory.cpp
libkantpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN) -version-info 1:0:0 -no-undefined
libkantpart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS) ../document/libkantdocument.la ../view/libkantview.la

INCLUDES= $(all_includes)

METASOURCES = AUTO
