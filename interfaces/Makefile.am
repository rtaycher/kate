METASOURCES =  kateappIface.moc  katedocmanagerIface.moc  katedocumentIface.moc katepluginIface.moc kateviewIface.moc kateviewmanagerIface.moc

lib_LTLIBRARIES = libkateIface.la

libkateIface_la_SOURCES = kateIface.cpp
libkateIface_la_LIBADD   = -lktexteditor
libkateIface_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)  $(KDE_PLUGIN)  -module -avoid-version

INCLUDES= $(all_includes)
