kde4_header()

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############

install_files( ${XDG_APPS_DIR} FILES kate.desktop )
install_files( ${CONFIG_INSTALL_DIR} FILES katerc )
install_files( ${DATA_INSTALL_DIR}/kate FILES tips )
install_files( ${DATA_INSTALL_DIR}/kate FILES kateui.rc externaltools )
install_files( ${SERVICETYPES_INSTALL_DIR} FILES kateplugin.desktop )
install_files( ${DATA_INSTALL_DIR}/kconf_update FILES kate-2.4.upd )

kde4_footer()



#original Makefile.am contents follow:

#servicetypedir = $(kde_servicetypesdir)
#servicetype_DATA = kateplugin.desktop
#
#xdg_apps_DATA = kate.desktop
#
#rckatedir = $(kde_datadir)/kate
#rckate_DATA = kateui.rc externaltools
#
#tipdir = $(kde_datadir)/kate
#tip_DATA = tips
#
#updatedir = $(kde_datadir)/kconf_update
#update_DATA = kate-2.4.upd
#
#kde_conf_DATA = katerc
