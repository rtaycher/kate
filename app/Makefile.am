lib_LTLIBRARIES = kate.la kwrite.la
bin_PROGRAMS = kate kwrite

kate_la_SOURCES = kateapp.cpp kateconfigdialog.cpp kateconfigplugindialogpage.cpp \
	kateconsole.cpp katedocmanager.cpp katefilelist.cpp katefileselector.cpp \
	katemainwindow.cpp katepluginmanager.cpp katesplitter.cpp \
	kateviewmanager.cpp kateviewspace.cpp katemain.cpp \
	kateIface.skel kategrepdialog.cpp katefiledialog.cpp katemailfilesdialog.cpp \
        kbookmarkhandler.cpp kactionselector.cpp

kate_la_LIBADD   = ../interfaces/libkateinterfaces.la -lkscript
kate_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version

kwrite_la_SOURCES = kwritemain.cpp katefiledialog.cpp
kwrite_la_LIBADD   = -lktexteditor
kwrite_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version

kate_SOURCES = dummy.cpp
kate_LDADD = kate.la
kate_LDFLAGS = $(KDE_RPATH) $(all_libraries)

kwrite_SOURCES = dummy.cpp
kwrite_LDADD = kwrite.la
kwrite_LDFLAGS = $(KDE_RPATH) $(all_libraries)

dummy.cpp:
	echo > dummy.cpp

INCLUDES= -DKATEPART_NOT_INSTALLED -I../interfaces $(all_includes)

METASOURCES = AUTO
