INCLUDES    =  -I$(srcdir)/../../interfaces $(all_includes)
METASOURCES = AUTO

KDE_ICON = textfilter

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = libkatetextfilterplugin.la

# This is all standard.  Remove the LIB_KHTML reference if you are not
# using the KHTML Part
libkatetextfilterplugin_la_SOURCES = plugin_katetextfilter.cpp
libkatetextfilterplugin_la_LIBADD = ../../interfaces/libkate.la
libkatetextfilterplugin_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)

# Install the .rc file in the Part's directory (in this case, the part
# is KHTMLPart)
pluginsdir = $(kde_datadir)/kate/plugins/katetextfilter
plugins_DATA = ui.rc

deskdir=$(kde_datadir)/kate/plugins
desk_DATA=katetextfilter.desktop
