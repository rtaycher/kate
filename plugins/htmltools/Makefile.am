INCLUDES    =  -I$(srcdir)/../../pluginmanager -I$(srcdir)/../../interfaces $(all_includes)
METASOURCES = AUTO

KDE_ICON = htmltools

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = libkanthtmltoolsplugin.la

# This is all standard.  Remove the LIB_KHTML reference if you are not
# using the KHTML Part
libkanthtmltoolsplugin_la_SOURCES = plugin_kanthtmltools.cpp
libkanthtmltoolsplugin_la_LIBADD = ../../main/libkant.la ../../interfaces/libkantIface.la
libkanthtmltoolsplugin_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)

# Install the .rc file in the Part's directory (in this case, the part
# is KHTMLPart)
pluginsdir = $(kde_datadir)/kant/plugins/kanthtmltools
plugins_DATA = ui.rc

deskdir=$(kde_datadir)/kant/plugins
desk_DATA=kanthtmltools.desktop
