lib_LTLIBRARIES = libkant.la kant.la kwrite.la
bin_PROGRAMS = kant kwrite

libkant_la_SOURCES = dummy.cpp
libkant_la_LIBADD   = ../factory/libkantfactory.la ../document/libkantdocument.la ../view/libkantview.la ../app/libkantapp.la \
		../mainwindow/libkantmainwindow.la \
		../sidebar/libkantsidebar.la \
		../fileselector/libkantfileselector.la  \
		../console/libkantconsole.la ../filelist/libkantfilelist.la \
		../pluginmanager/libkantpluginmanager.la ../interfaces/libkantpluginIface.la -lktexteditor -lkspell $(LIB_KPARTS)
libkant_la_LDFLAGS = $(all_libraries) $(KDE_RPATH)  $(KDE_PLUGIN)  -module -avoid-version

kant_la_SOURCES = kantmain.cpp
kant_la_LIBADD   = libkant.la
kant_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version

kwrite_la_SOURCES = kwritemain.cpp
kwrite_la_LIBADD   = libkant.la
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
