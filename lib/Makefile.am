SUBDIRS = .

INCLUDES = -I$(top_srcdir)/libkonq -I$(top_srcdir)/kate/lib $(all_includes)

lib_LTLIBRARIES = libkmultiverttabbar.la 

libkmultiverttabbar_la_SOURCES=kmultiverttabbar.cpp
libkmultiverttabbar_la_LDFLAGS = $(all_libraries) -no-undefined
libkmultiverttabbar_la_LIBADD = $(LIB_KPARTS)

METASOURCES = AUTO

