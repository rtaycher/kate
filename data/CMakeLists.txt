


########### install files ###############

install( FILES kate.desktop  DESTINATION  ${XDG_APPS_DIR} )
install( FILES katerc  DESTINATION  ${CONFIG_INSTALL_DIR} )
install( FILES tips  DESTINATION  ${DATA_INSTALL_DIR}/kate )
install( FILES kateui.rc externaltools  DESTINATION  ${DATA_INSTALL_DIR}/kate )
install( FILES kateplugin.desktop  DESTINATION  ${SERVICETYPES_INSTALL_DIR} )
install( FILES kate-2.4.upd  DESTINATION  ${KCONF_UPDATE_INSTALL_DIR} )




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
