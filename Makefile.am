SUBDIRS = document view part app console data filelist fileselector mainwindow pics project sidebar interfaces pluginmanager plugins
EXTRA_DIST = piper AUTHORS COPYING ChangeLog NEWS README TODO

bin_PROGRAMS = kant kwrite

kant_SOURCES = kantmain.cpp
kant_LDADD   = ./part/libkantpart.la ./app/libkantapp.la ./view/libkantviewinternal.la \
		./mainwindow/libkantmainwindow.la \
		./sidebar/libkantsidebar.la ./project/libkantproject.la \
		./fileselector/libkantfileselector.la ./document/libkantdocumentinternal.la \
		./console/libkantconsole.la ./filelist/libkantfilelist.la \
		./pluginmanager/libkantpluginmanager.la ./interfaces/libkantpluginIface.la
kant_LDFLAGS = $(all_libraries) $(KDE_RPATH)

kwrite_SOURCES = kwritemain.cpp
kwrite_LDADD   = ./part/libkantpart.la
kwrite_LDFLAGS = $(all_libraries) $(KDE_RPATH)

INCLUDES= $(all_includes)

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kant.pot

rc.cpp:
	$(EXTRACTRC) *.rc *.ui */*.rc */*.ui */*/*.rc */*/*.ui > rc.cpp
