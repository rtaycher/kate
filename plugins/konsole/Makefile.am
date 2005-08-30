INCLUDES    =  -I$(top_srcdir) $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = katekonsoleplugin.la

katekonsoleplugin_la_SOURCES = kateconsole.cpp
katekonsoleplugin_la_LIBADD = ../../app/libkateinterfaces.la
katekonsoleplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir = $(kde_datadir)/kate/plugins/katekonsole
plugins_DATA = ui.rc

kde_services_DATA = katekonsoleplugin.desktop
