lib_LTLIBRARIES = libkateinterfaces.la
bin_PROGRAMS =
kdeinit_LTLIBRARIES = kate.la kwrite.la

libkateinterfaces_la_SOURCES = kateapp.cpp kateconfigdialog.cpp kateconfigplugindialogpage.cpp \
	kateconsole.cpp katedocmanager.cpp katefilelist.cpp katefileselector.cpp \
	katemainwindow.cpp katepluginmanager.cpp katesplitter.cpp \
	kateviewmanager.cpp kateviewspace.cpp kateprojectmanager.cpp \
	kateIface.skel kategrepdialog.cpp katemailfilesdialog.cpp \
        kbookmarkhandler.cpp kactionselector.cpp katedockcontainer.cpp kateprojectlist.cpp \
        kateprojecttreeview.cpp kateprojectviews.cpp kateprojectdirview.cpp

libkateinterfaces_la_LIBADD   = ../interfaces/libkateinterfacesprivate.la -lkscript $(LIB_KUTILS) ../utils/libkateutils.la
libkateinterfaces_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -no-undefined

kate_la_SOURCES = katemain.cpp
kate_la_LIBADD = libkateinterfaces.la
kate_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version

kwrite_la_SOURCES = kwritemain.cpp kwritedialogs.cpp
kwrite_la_LIBADD   = -lkatepartinterfaces  ../utils/libkateutils.la
kwrite_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version

METASOURCES = AUTO

INCLUDES= -I../interfaces -I$(srcdir)/../lib $(all_includes)
