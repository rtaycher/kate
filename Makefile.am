SUBDIRS = interfaces part app data pics
EXTRA_DIST = AUTHORS COPYING COPYING.LIB ChangeLog NEWS README TODO

messages:
	$(EXTRACTRC) ./*/*.rc >> ./rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kate.pot

install-data-local: uninstall.desktop
	$(mkinstalldirs) $(kde_servicesdir)
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(kde_servicesdir)/kate_component.desktop
	$(INSTALL_DATA) $(srcdir)/uninstall.desktop $(kde_servicesdir)/kwrite_component.desktop
