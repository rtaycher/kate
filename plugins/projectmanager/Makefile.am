EXTRA_DIST = piper

INCLUDES    =  -I$(srcdir)/../../interfaces $(all_includes)
METASOURCES = AUTO

KDE_ICON = textfilter

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = libkantprojectmanagerplugin.la

# This is all standard.  Remove the LIB_KHTML reference if you are not
# using the KHTML Part
libkantprojectmanagerplugin_la_SOURCES =  kantprojectmanager.cpp kantprojectdialog.cpp kantproject.cpp
libkantprojectmanagerplugin_la_LIBADD = ../../interfaces/libkantIface.la
libkantprojectmanagerplugin_la_LDFLAGS = $(KDE_PLUGIN) $(all_libraries)

# Install the .rc file in the Part's directory (in this case, the part
# is KHTMLPart)
pluginsdir = $(kde_datadir)/kant/plugins/kantprojectmanager
plugins_DATA = ui.rc

deskdir=$(kde_datadir)/kant/plugins
desk_DATA=kantprojectmanager.desktop
