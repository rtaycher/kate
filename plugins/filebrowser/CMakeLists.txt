kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(katefilebrowserplugin_PART_SRCS katefileselector.cpp kbookmarkhandler.cpp )

kde4_automoc(${katefilebrowserplugin_PART_SRCS})

kde4_add_plugin(katefilebrowserplugin ${katefilebrowserplugin_PART_SRCS})

kde4_install_libtool_file( ${PLUGIN_INSTALL_DIR} katefilebrowserplugin )

target_link_libraries(katefilebrowserplugin  ${KDE4_KDE3SUPPORT_LIBS} kateinterfaces )

install_targets(${LIB_INSTALL_DIR}/kde4 katefilebrowserplugin )


########### install files ###############

install_files( ${SERVICES_INSTALL_DIR} FILES katefilebrowserplugin.desktop )

kde4_footer()



#original Makefile.am contents follow:

#INCLUDES    =  -I$(top_srcdir) $(all_includes)
#METASOURCES = AUTO
#
## Install this plugin in the KDE modules directory
#kde_module_LTLIBRARIES = katefilebrowserplugin.la
#
#katefilebrowserplugin_la_SOURCES = katefileselector.cpp kbookmarkhandler.cpp
#katefilebrowserplugin_la_LIBADD = ../../app/libkateinterfaces.la
#katefilebrowserplugin_la_LDFLAGS = -module $(KDE_PLUGIN) $(all_libraries)
#
##pluginsdir = $(kde_datadir)/kate/plugins/kfileselector
##plugins_DATA = ui.rc
#
#kde_services_DATA = katefilebrowserplugin.desktop
