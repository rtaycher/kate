
add_subdirectory( actions ) 



########### install files ###############

install_files( ${DATA_INSTALL_DIR}/kate/pics FILES sessionchooser.png )

kde4_install_icons( ${ICON_INSTALL_DIR} hicolor )




#original Makefile.am contents follow:

#KDE_ICON = AUTO
#
#SUBDIRS = actions
#
#kate_pics_data_DATA = sessionchooser.png
#kate_pics_datadir = $(kde_datadir)/kate/pics
