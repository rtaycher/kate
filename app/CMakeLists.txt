kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### next target ###############

set(kateinterfaces_LIB_SRCS  ${kateinterfacesprivate_STAT_SRCS}
   kateapp.cpp 
   kateconfigdialog.cpp 
   kateconfigplugindialogpage.cpp 
   katedocmanager.cpp 
   katefilelist.cpp 
   katemainwindow.cpp 
   katepluginmanager.cpp 
   kateviewmanager.cpp 
   kateviewspace.cpp 
   katemainwindowiface.cpp 
   kategrepdialog.cpp 
   katemailfilesdialog.cpp 
   katetooltipmenu.cpp 
   kateappIface.cpp 
   katedocmanageriface.cpp 
   kateexternaltools.cpp 
   katesavemodifieddialog.cpp 
   kateviewspacecontainer.cpp 
   katemwmodonhddialog.cpp 
   katesession.cpp 
   katemdi.cpp 
   katetabwidget.cpp 
   katepartmanager.cpp )

kde4_automoc(${kateinterfaces_LIB_SRCS})

kde4_add_dcop_skels(kateinterfaces_LIB_SRCS katemainwindowiface.h katedocmanageriface.h kateappIface.h )

kde4_add_library(kateinterfaces SHARED ${kateinterfaces_LIB_SRCS})

target_link_libraries(kateinterfaces  ${LIB_KDE3SUPPORT} DCOP ${QT_QTXML_LIBRARY} ktexteditor  )

set_target_properties(kateinterfaces PROPERTIES VERSION 1.0.0 SOVERSION 1 )
install_targets(${LIB_INSTALL_DIR} kateinterfaces )


########### next target ###############

set(kate_KDEINIT_SRCS katemain.cpp ${kateinterfaces_LIB_SRCS})

kde4_automoc(${kate_KDEINIT_SRCS})

kde4_add_kdeinit_executable( kate ${kate_KDEINIT_SRCS})

target_link_libraries(kdeinit_kate  ${QT_AND_KDECORE_LIBS} kateinterfaces )

install_targets(${LIB_INSTALL_DIR} kdeinit_kate )

target_link_libraries( kate kdeinit_kate )
install_targets(/bin kate )

########### install files ###############


kde4_footer()



#original Makefile.am contents follow:

#lib_LTLIBRARIES = libkateinterfaces.la
#bin_PROGRAMS =
#kdeinit_LTLIBRARIES = kate.la
#
#libkateinterfaces_la_SOURCES = kateapp.cpp kateconfigdialog.cpp kateconfigplugindialogpage.cpp \
#  katedocmanager.cpp katefilelist.cpp \
#  katemainwindow.cpp katepluginmanager.cpp \
#  kateviewmanager.cpp kateviewspace.cpp \
#  katemainwindowiface.skel katemainwindowiface.cpp kategrepdialog.cpp \
#  katemailfilesdialog.cpp katetooltipmenu.cpp\
#  katedocmanageriface.skel kateappIface.cpp kateappIface.skel katedocmanageriface.cpp \
#  kateexternaltools.cpp katesavemodifieddialog.cpp kateviewspacecontainer.cpp \
#  katemwmodonhddialog.cpp katesession.cpp katemdi.cpp katetabwidget.cpp katepartmanager.cpp
#
#libkateinterfaces_la_LIBADD   = ../interfaces/libkateinterfacesprivate.la $(LIB_KUTILS)
#
#libkateinterfaces_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -no-undefined
#
#kate_la_SOURCES = katemain.cpp
#kate_la_LIBADD = libkateinterfaces.la
#kate_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version
#
#METASOURCES = AUTO
#
#INCLUDES= -I../interfaces -I$(srcdir)/../lib $(all_includes)
