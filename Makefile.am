SUBDIRS = qt3back document view app console data filelist fileselector mainwindow pics sidebar interfaces pluginmanager factory main part plugins
EXTRA_DIST = AUTHORS COPYING ChangeLog NEWS README TODO

messages: rc.cpp
	$(EXTRACTRC) */*.rc */*.ui */*/*.rc */*/*.ui >> rc.cpp
	perl ./data/preparedata.pl ./data/*.xml
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kate.pot
	rm -f data.cpp
