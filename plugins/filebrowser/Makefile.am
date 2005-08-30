INCLUDES    =  -I$(top_srcdir) $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = katefilebrowserplugin.la

katefilebrowserplugin_la_SOURCES = katefileselector.cpp kbookmarkhandler.cpp
katefilebrowserplugin_la_LIBADD = -lkateinterfaces
katefilebrowserplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

#pluginsdir = $(kde_datadir)/kate/plugins/kfileselector
#plugins_DATA = ui.rc

kde_services_DATA = katefilebrowserplugin.desktop
