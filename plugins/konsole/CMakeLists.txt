kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(katekonsoleplugin_PART_SRCS kateconsole.cpp )

kde4_automoc(${katekonsoleplugin_PART_SRCS})

kde4_add_plugin(katekonsoleplugin ${katekonsoleplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} katekonsoleplugin )

target_link_libraries(katekonsoleplugin  ${QT_AND_KDECORE_LIBS} kateinterfaces )

install_targets(${LIB_INSTALL_DIR}/kde4 katekonsoleplugin )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kate/plugins/katekonsole FILES ui.rc )
install_files( ${SERVICES_INSTALL_DIR} FILES katekonsoleplugin.desktop )

kde4_footer()



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
