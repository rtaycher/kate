METASOURCES = AUTO
#application.moc documentmanager.moc mainwindow.moc plugin.moc viewmanager.moc pluginconfiginterfaceextension.moc

lib_LTLIBRARIES = libkateinterfaces.la

libkateinterfaces_la_SOURCES = interfaces.cpp application.cpp pluginconfiginterface.cpp pluginconfiginterfaceextension.cpp
libkateinterfaces_la_LIBADD   = -lkatepartinterfaces
libkateinterfaces_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)

kateinclude_HEADERS = application.h documentmanager.h mainwindow.h plugin.h viewmanager.h pluginconfiginterface.h pluginconfiginterfaceextension.h toolviewmanager.h pluginmanager.h
kateincludedir = $(includedir)/kate

INCLUDES= $(all_includes)
