# Allow build from within kdebase or not.
INCLUDES    = \
	-I$(top_srcdir)/kate/interfaces \
	-I$(kde_includes)/kate \
	$(all_includes)

METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = katekttsdplugin.la

katekttsdplugin_la_SOURCES = katekttsd.cpp
katekttsdplugin_la_LIBADD  = ../../app/libkateinterfaces.la
katekttsdplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir = $(kde_datadir)/kate/plugins/katekttsd
plugins_DATA = ui.rc

kde_services_DATA = katekttsd.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/katekttsd.pot
