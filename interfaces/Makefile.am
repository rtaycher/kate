lib_LTLIBRARIES = libkateIface.la

libkateIface_la_SOURCES = katepluginIface.cpp katedocumentIface.cpp kateviewIface.cpp katedocmanagerIface.cpp kateviewmanagerIface.cpp kateappIface.cpp
libkateIface_la_LIBADD   = -lktexteditor
libkateIface_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)  $(KDE_PLUGIN)  -module -avoid-version

INCLUDES= $(all_includes)

METASOURCES = AUTO
