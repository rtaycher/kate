INCLUDES = -I$(top_srcdir)/kate/lib/utils $(all_includes)

lib_LTLIBRARIES = libkateutils.la

libkateutils_la_SOURCES = katedockviewbase.cpp katemessageview.cpp

libkateutils_la_LDFLAGS = $(all_libraries) -no-undefined

                            
libkateutils_la_LIBADD = $(LIB_KDEUI) $(LIB_KDECORE)

libkateutils_la_METASOURCES = AUTO

kateutilsinclude_HEADERS = katedockviewbase.h katemessageview.h

kateutilsincludedir = $(includedir)/kate/utils
