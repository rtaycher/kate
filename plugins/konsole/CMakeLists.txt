


########### next target ###############

set(katekonsoleplugin_PART_SRCS kateconsole.cpp )

kde4_automoc(${katekonsoleplugin_PART_SRCS})

kde4_add_plugin(katekonsoleplugin ${katekonsoleplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} katekonsoleplugin )

target_link_libraries(katekonsoleplugin  ${KDE4_KDECORE_LIBS} kateinterfaces )

install(TARGETS katekonsoleplugin  DESTINATION ${PLUGIN_INSTALL_DIR} )


########### install files ###############

install( FILES ui.rc  DESTINATION  ${DATA_INSTALL_DIR}/kate/plugins/katekonsole )
install( FILES katekonsoleplugin.desktop  DESTINATION  ${SERVICES_INSTALL_DIR} )




#original Makefile.am contents follow:

#INCLUDES    =  -I$(top_srcdir) $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = katekonsoleplugin.la
#
#katekonsoleplugin_la_SOURCES = kateconsole.cpp
#katekonsoleplugin_la_LIBADD = ../../app/libkateinterfaces.la
#katekonsoleplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
#pluginsdir = $(kde_datadir)/kate/plugins/katekonsole
#plugins_DATA = ui.rc
#
#kde_services_DATA = katekonsoleplugin.desktop
