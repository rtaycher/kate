lib_LTLIBRARIES = libkatepart.la libkwritepart.la

libkatepart_la_SOURCES = katepart.cpp
libkatepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkatepart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS) ../main/libkate.la

libkwritepart_la_SOURCES = kwritepart.cpp
libkwritepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkwritepart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS) ../main/libkate.la

INCLUDES= $(all_includes)

METASOURCES = AUTO
