lib_LTLIBRARIES = libkantIface.la

libkantIface_la_SOURCES = kantpluginIface.cpp kantdocumentIface.cpp kantviewIface.cpp kantdocmanagerIface.cpp kantviewmanagerIface.cpp kantappIface.cpp
libkantIface_la_LIBADD   = -lktexteditor
libkantIface_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)  $(KDE_PLUGIN)  -module -avoid-version

INCLUDES= $(all_includes)

METASOURCES = AUTO
