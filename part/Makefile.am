lib_LTLIBRARIES = libkantpart.la libkwritepart.la

libkantpart_la_SOURCES = kantpart.cpp
libkantpart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkantpart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS) ../main/libkant.la

libkwritepart_la_SOURCES = kwritepart.cpp
libkwritepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkwritepart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS) ../main/libkant.la

INCLUDES= $(all_includes)

METASOURCES = AUTO
