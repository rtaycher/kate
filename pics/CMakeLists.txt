
add_subdirectory( actions ) 



########### install files ###############

install( FILES sessionchooser.png  DESTINATION  ${DATA_INSTALL_DIR}/kate/pics )

kde4_install_icons( ${ICON_INSTALL_DIR}  )




#original Makefile.am contents follow:

#KDE_ICON = AUTO
#
#SUBDIRS = actions
#
#kate_pics_data_DATA = sessionchooser.png
#kate_pics_datadir = $(kde_datadir)/kate/pics
