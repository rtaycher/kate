SUBDIRS = defaultproject

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(DESTDIR)$(kde_datadir)/kate/plugins/katekttsd
	$(mkinstalldirs) $(DESTDIR)$(kde_datadir)/katepart/kpartplugins
	cp -f /dev/null $(DESTDIR)$(kde_datadir)/kate/plugins/katekttsd/ui.rc
	cp -f /dev/null $(DESTDIR)$(kde_datadir)/katepart/kpartplugins/katepartkttsd.rc
	$(mkinstalldirs) $(DESTDIR)$(kde_servicesdir)
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(DESTDIR)$(kde_servicesdir)/katekttsd.desktop

