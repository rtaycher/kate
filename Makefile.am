# $Id$

AM_CPPFLAGS = -DQT_NO_ASCII_CAST

INCLUDES = $(all_includes)

KDE_CXXFLAGS = $(USE_RTTI)

bin_PROGRAMS = kwrite

lib_LTLIBRARIES = libkwrite.la

libkwrite_la_SOURCES = highlight.cpp ktextprint.cpp  \
		       kwdoc.cpp kwview.cpp undohistory.cpp  \
		       kwdialog.cpp kwrite_factory.cpp

libkwrite_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) $(KDE_PLUGIN)
libkwrite_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS)

kwrite_SOURCES = toplevel.cpp

kwrite_LDFLAGS = $(all_libraries) $(KDE_RPATH)
kwrite_LDADD = libkwrite.la $(LIB_KFILE)

METASOURCES = AUTO

appsdir = $(kde_appsdir)/Editors
apps_DATA = kwrite.desktop

rcdir = $(kde_datadir)/kwrite
rc_DATA = kwrite_browser.rc kwriteui.rc

KDE_ICON = kwrite

EXTRA_DIST = $(apps_DATA)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kwrite.pot
