
noinst_LTLIBRARIES = libkantpluginmanager.la
libkantpluginmanager_la_SOURCES = kantpluginmanager.cpp

lib_LTLIBRARIES = libkantpluginiface.la
libkantpluginiface_la_SOURCES = kantpluginiface.cpp
libkantpluginiface_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkantpluginiface_la_LIBADD =  ../kwrite/libkwritepart.la $(LIB_KPARTS)

INCLUDES= $(all_includes)

METASOURCES = AUTO
