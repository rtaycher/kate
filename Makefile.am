SUBDIRS = document view app console data filelist fileselector mainwindow pics project sidebar interfaces pluginmanager factory main part plugins
EXTRA_DIST = piper AUTHORS COPYING ChangeLog NEWS README TODO

messages: rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kant.pot

rc.cpp:
	$(EXTRACTRC) *.rc *.ui */*.rc */*.ui */*/*.rc */*/*.ui > rc.cpp
