lib_LTLIBRARIES = libkantpart.la libkwritepart.la

libkantpart_la_SOURCES = kantpartview.cpp kantpartdocument.cpp kantpartfactory.cpp
libkantpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkantpart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS) ../document/libkantdocument.la ../view/libkantview.la

libkwritepart_la_SOURCES = kwritepart.cpp
libkwritepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkwritepart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS) libkantpart.la

INCLUDES= $(all_includes)

METASOURCES = AUTO
