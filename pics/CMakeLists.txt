kde4_header()

add_subdirectory( actions ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kate/pics FILES sessionchooser.png )

kde4_install_icons( hicolor crystalsvg )

kde4_footer()



#original Makefile.am contents follow:

#KDE_ICON = AUTO
#
#SUBDIRS = actions
#
#kate_pics_data_DATA = sessionchooser.png
#kate_pics_datadir = $(kde_datadir)/kate/pics