METASOURCES = application.moc docmanager.moc document.moc plugin.moc view.moc viewmanager.moc

lib_LTLIBRARIES = libkate.la

libkate_la_SOURCES = interfaces.cpp
libkate_la_LIBADD   = -lktexteditor
libkate_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)  $(KDE_PLUGIN)  -module -avoid-version

kateinclude_HEADERS = application.h docmanager.h document.h plugin.h view.h viewmanager.h
kateincludedir = $(includedir)/kate

INCLUDES= $(all_includes)
