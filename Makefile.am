SUBDIRS = interfaces part app data pics
EXTRA_DIST = AUTHORS COPYING COPYING.LIB ChangeLog NEWS README TODO

messages:
	$(EXTRACTRC) ./*/*.rc >> ./rc.cpp
	perl ./data/preparedata.pl ./data/*.xml
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kate.pot
	rm -f data.cpp
