INCLUDES    =  -I$(srcdir)/../../interfaces $(all_includes)
METASOURCES = AUTO

KDE_ICON = textfilter

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = libkateopenheaderplugin.la

# This is all standard.  Remove the LIB_KHTML reference if you are not
# using the KHTML Part
libkateopenheaderplugin_la_SOURCES = plugin_kateopenheader.cpp
libkateopenheaderplugin_la_LIBADD = ../../interfaces/libkate.la
libkateopenheaderplugin_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)

# Install the .rc file in the Part's directory (in this case, the part
# is KHTMLPart)
pluginsdir = $(kde_datadir)/kate/plugins/kateopenheader
plugins_DATA = ui.rc

deskdir=$(kde_datadir)/kate/plugins
desk_DATA=kateopenheader.desktop
