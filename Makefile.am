SUBDIRS = app console data document filelist fileselector kwrite mainwindow pics project sidebar view pluginmanager plugins
EXTRA_DIST = interfaces piper AUTHORS COPYING ChangeLog NEWS README TODO 

bin_PROGRAMS = kant

kant_SOURCES = kantmain.cpp
kant_LDADD   =  ./kwrite/libkwritepart.la ./app/libkantapp.la \
		./view/libkantview.la ./mainwindow/libkantmainwindow.la \
		./sidebar/libkantsidebar.la ./project/libkantproject.la \
		./fileselector/libkantfileselector.la ./document/libkantdocument.la \
		./console/libkantconsole.la ./filelist/libkantfilelist.la \
		./pluginmanager/libkantpluginmanager.la ./pluginmanager/libkantpluginiface.la
kant_LDFLAGS = $(all_libraries) $(KDE_RPATH)

INCLUDES= $(all_includes)

METASOURCES = AUTO


messages: rc.cpp
	$(XGETTEXT) -C -ki18n -kI18N_NOOP -ktranslate -x $(kde_includes)/kde.pot *.cpp -o $(podir)/kant.pot

rc.cpp:
	$(EXTRACTRC) *.rc *.ui */*.rc */*.ui */*/*.rc */*/*.ui > rc.cpp
