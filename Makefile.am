SUBDIRS = document view part app console data filelist fileselector kwrite mainwindow pics project sidebar interfaces pluginmanager plugins
EXTRA_DIST = piper AUTHORS COPYING ChangeLog NEWS README TODO

bin_PROGRAMS = kant

kant_SOURCES = kantmain.cpp
kant_LDADD   = ./part/libkantpart.la ./app/libkantapp.la ./view/libkantviewinternal.la \
		./mainwindow/libkantmainwindow.la \
		./sidebar/libkantsidebar.la ./project/libkantproject.la \
		./fileselector/libkantfileselector.la ./document/libkantdocumentinternal.la \
		./console/libkantconsole.la ./filelist/libkantfilelist.la \
		./pluginmanager/libkantpluginmanager.la ./interfaces/libkantpluginIface.la
kant_LDFLAGS = $(all_libraries) $(KDE_RPATH)

INCLUDES= $(all_includes)

METASOURCES = AUTO


messages: rc.cpp
	$(XGETTEXT) -C -ki18n -kI18N_NOOP -ktranslate -x $(kde_includes)/kde.pot *.cpp -o $(podir)/kant.pot

rc.cpp:
	$(EXTRACTRC) *.rc *.ui */*.rc */*.ui */*/*.rc */*/*.ui > rc.cpp
