KDE_CXXFLAGS = $(USE_RTTI)  -DQT_CLEAN_NAMESPACE -DQT_NO_COMPAT -DQT_NO_ASCII_CAST

noinst_LIBRARIES = libkantpluginmanager.a
lib_LTLIBRARIES = libkantpluginiface.la

libkantpluginmanager_a_SOURCES = kantpluginmanager.cpp

libkantpluginiface_la_SOURCES = kantpluginiface.cpp
libkantpluginiface_la_LDFLAGS = $(all_libraries) $(KDE_PLUGIN)
libkantpluginiface_la_LIBADD = -L../kwrite/ -lkwritepart $(LIB_KPARTS)


EXTRA_DIST = kantpluginmanager.cpp kantpluginmanager.h kantpluginiface.cpp kantpluginiface.h

INCLUDES= $(all_includes)

METASOURCES = AUTO
