INCLUDES    =   -I../../interfaces $(all_includes)
METASOURCES = AUTO

# Install this plugin in the KDE modules directory
kde_module_LTLIBRARIES = katedefaultprojectplugin.la

katedefaultprojectplugin_la_SOURCES = katedefaultproject.cpp
katedefaultprojectplugin_la_LIBADD = ../../app/libkateinterfaces.la
katedefaultprojectplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)

# Install the .rc file in the Part's directory (in this case, the part
# is KHTMLPart)
pluginsdir = $(kde_datadir)/kate/plugins/katedefaultproject
plugins_DATA = ui.rc

kde_services_DATA = katedefaultproject.desktop

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/katedefaultproject.pot
