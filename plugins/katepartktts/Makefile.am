# Allow build from within kdebase or not.
INCLUDES    = \
	-I$(top_srcdir)/kate/interfaces \
	-I$(kde_includes)/kate \
	$(all_includes)

METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = libkatepartkttsdplugin.la

libkatepartkttsdplugin_la_SOURCES = katepartkttsd.cpp
libkatepartkttsdplugin_la_LIBADD  = $(LIB_KPARTS) -lkateinterfaces
libkatepartkttsdplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

pluginsdir = $(kde_datadir)/katepart/kpartplugins
plugins_DATA = katepartkttsd.rc

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/katepartkttsd.pot
