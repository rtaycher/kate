SUBDIRS = document view app console data filelist fileselector mainwindow pics sidebar interfaces pluginmanager factory main part plugins
EXTRA_DIST = piper AUTHORS COPYING ChangeLog NEWS README TODO

messages: rc.cpp
	$(EXTRACTRC) */*.rc */*.ui */*/*.rc */*/*.ui >> rc.cpp
#	perl ./preparesyntax >kantsyntax.cc
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kant.pot
#	rm -f kantsyntax.cc

