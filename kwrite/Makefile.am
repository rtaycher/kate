# $Id$

INCLUDES = $(all_includes)

KDE_CXXFLAGS = $(USE_RTTI) -DNO_NEW_CODE

bin_PROGRAMS = kwrite

lib_LTLIBRARIES = libkwritepart.la kwrite.la
# libkwritepart is a part, but kwrite.la links directly to it, so we can't
# install it under kde_module

libkwritepart_la_SOURCES = highlight.cpp  \
		           kwdoc.cpp kwview.cpp undohistory.cpp  \
		           kwdialog.cpp kwrite_factory.cpp \
                           kwbuffer.cpp kwtextline.cpp kvmallocator.cpp\
                           kwattribute.cpp kwriteIface.skel syntaxdocument.cpp

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

kde_services_DATA = kwrite_component.desktop

rcdir = $(kde_datadir)/kwrite
rc_DATA = kwrite_browser.rc kwriteui.rc kwrite_shell.rc syntax.xml

KDE_ICON = kwrite

EXTRA_DIST = $(apps_DATA)

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/kwrite.pot
