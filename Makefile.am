SUBDIRS = lib interfaces app data pics
EXTRA_DIST = AUTHORS COPYING COPYING.LIB ChangeLog DESIGN NEWS README TODO

messages:
	$(EXTRACTRC) ./*/*.rc >> ./rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kate.pot
