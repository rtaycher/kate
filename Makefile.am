# $Id$

AM_CPPFLAGS = -DQT_NO_ASCII_CAST

INCLUDES = $(all_includes)

KDE_CXXFLAGS = $(USE_RTTI)

bin_PROGRAMS = kwrite

lib_LTLIBRARIES = libkwritepart.la kwrite.la

libkwritepart_la_SOURCES = highlight.cpp ktextprint.cpp  \
		           kwdoc.cpp kwview.cpp undohistory.cpp  \
		           kwdialog.cpp kwrite_factory.cpp

libkwritepart_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkwritepart_la_LIBADD = -lktexteditor -lkspell $(LIB_KPARTS)

kwrite_la_SOURCES = toplevel.cpp

kwrite_la_LDFLAGS = $(all_libraries) -module -avoid-version
kwrite_la_LIBADD = libkwritepart.la $(LIB_KFILE)

kwrite_SOURCES = dummy.cpp
kwrite_LDADD = kwrite.la
kwrite_LDFLAGS = $(KDE_RPATH) $(all_libraries)

dummy.cpp:
	echo > dummy.cpp

METASOURCES = AUTO

appsdir = $(kde_appsdir)/Editors
apps_DATA = kwrite.desktop

rcdir = $(kde_datadir)/kwrite
rc_DATA = kwrite_browser.rc kwriteui.rc

KDE_ICON = kwrite

EXTRA_DIST = $(apps_DATA)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kwrite.pot
