SUBDIRS = document view part app console data filelist fileselector mainwindow pics project sidebar interfaces pluginmanager plugins
EXTRA_DIST = piper AUTHORS COPYING ChangeLog NEWS README TODO

lib_LTLIBRARIES = kant.la kwrite.la
bin_PROGRAMS = kant kwrite

kant_la_SOURCES = kantmain.cpp
kant_la_LIBADD   = ./part/libkantpart.la ./app/libkantapp.la ./view/libkantviewinternal.la \
		./mainwindow/libkantmainwindow.la \
		./sidebar/libkantsidebar.la ./project/libkantproject.la \
		./fileselector/libkantfileselector.la ./document/libkantdocumentinternal.la \
		./console/libkantconsole.la ./filelist/libkantfilelist.la \
		./pluginmanager/libkantpluginmanager.la ./interfaces/libkantpluginIface.la
kant_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)  -module -avoid-version

kwrite_la_SOURCES = kwritemain.cpp
kwrite_la_LIBADD   = ./part/libkantpart.la
kwrite_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)  -module -avoid-version

kant_SOURCES = dummy.cpp
kant_LDADD = kant.la
kant_LDFLAGS = $(KDE_RPATH) $(all_libraries)

kwrite_SOURCES = dummy.cpp
kwrite_LDADD = kwrite.la
kwrite_LDFLAGS = $(KDE_RPATH) $(all_libraries)

dummy.cpp:
	echo > dummy.cpp

INCLUDES= $(all_includes)

METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kant.pot

rc.cpp:
	$(EXTRACTRC) *.rc *.ui */*.rc */*.ui */*/*.rc */*/*.ui > rc.cpp
