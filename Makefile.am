# $Id$

INCLUDES = $(all_includes)

KDE_CXXFLAGS = -frtti

bin_PROGRAMS = kwrite

lib_LTLIBRARIES = libktexteditor.la libkwrite.la

libktexteditor_la_SOURCES = ktexteditor.cpp
libktexteditor_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)
libktexteditor_la_LIBADD = $(LIB_KPARTS)

include_HEADERS = ktexteditor.h

libkwrite_la_SOURCES = highlight.cpp ktextprint.cpp  \
		       kguicommand.cpp kwdoc.cpp \
		       kwview.cpp undohistory.cpp kwdialog.cpp \
		       kwrite_factory.cpp

libkwrite_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version -no-undefined
libkwrite_la_LIBADD = libktexteditor.la -lkspell $(LIB_KPARTS)

kwrite_SOURCES = toplevel.cpp

kwrite_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kwrite_LDADD = libkwrite.la $(LIB_KFILE) $(LIB_KDEUI) $(LIB_KDECORE)

METASOURCES = AUTO

servicetypedir = $(kde_servicetypesdir)
servicetype_DATA = ktexteditor.desktop

appsdir = $(kde_appsdir)/Editors
apps_DATA = kwrite.desktop

KDE_ICON = kwrite

EXTRA_DIST = $(apps_DATA)

messages: rc.cpp
	$(XGETTEXT) -ki18nop *.cpp -o $(podir)/kwrite.pot
