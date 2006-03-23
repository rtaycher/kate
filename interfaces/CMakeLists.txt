kde4_header()
include_directories(  ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### next target ###############


########### install files ###############

install_files( /include/kate FILES application.h documentmanager.h mainwindow.h plugin.h pluginconfigpageinterface.h pluginmanager.h )

kde4_footer()



#original Makefile.am contents follow:

#METASOURCES = AUTO
#
#noinst_LTLIBRARIES = libkateinterfacesprivate.la
#
#libkateinterfacesprivate_la_SOURCES = application.cpp mainwindow.cpp documentmanager.cpp \
#                                        pluginmanager.cpp plugin.cpp pluginconfigpageinterface.cpp
#
#libkateinterfacesprivate_la_LIBADD   = -lktexteditor
#libkateinterfacesprivate_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)
#
#kateinclude_HEADERS = application.h documentmanager.h mainwindow.h plugin.h pluginconfigpageinterface.h pluginmanager.h
#kateincludedir = $(includedir)/kate
#
#INCLUDES= $(all_includes)
