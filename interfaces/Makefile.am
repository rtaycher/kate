METASOURCES = application.moc docmanager.moc mainwindow.moc plugin.moc viewmanager.moc

lib_LTLIBRARIES = libkateinterfaces.la

libkateinterfaces_la_SOURCES = interfaces.cpp
libkateinterfaces_la_LIBADD   = -lkatepartinterfaces
libkateinterfaces_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)

kateinclude_HEADERS = application.h docmanager.h mainwindow.h plugin.h viewmanager.h
kateincludedir = $(includedir)/kate

INCLUDES= $(all_includes)
