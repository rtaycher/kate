EXTRA_DIST = kantui.rc kant.desktop

install-data-local:
	$(mkinstalldirs) $(kde_appsdir)/Editors/
	$(INSTALL_DATA) kant.desktop $(kde_appsdir)/Editors/kant.desktop

uninstall-local:
	-rm -f $(kde_appsdir)/Editors/kant.desktop

rcdir = $(kde_datadir)/kant
rc_DATA = kantui.rc
