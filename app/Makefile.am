lib_LTLIBRARIES = libkateinterfaces.la kate.la kwrite.la
bin_PROGRAMS = kate kwrite

libkateinterfaces_la_SOURCES = kateapp.cpp kateconfigdialog.cpp kateconfigplugindialogpage.cpp \
	kateconsole.cpp katedocmanager.cpp katefilelist.cpp katefileselector.cpp \
	katemainwindow.cpp katepluginmanager.cpp katesplitter.cpp \
	kateviewmanager.cpp kateviewspace.cpp kateprojectmanager.cpp kateproject.cpp \
	kateIface.skel kategrepdialog.cpp katefiledialog.cpp katemailfilesdialog.cpp \
        kbookmarkhandler.cpp kactionselector.cpp katedockcontainer.cpp

libkateinterfaces_la_LIBADD   = ../interfaces/libkateinterfacesprivate.la -lkscript ../lib/libkmultitabbar.la
libkateinterfaces_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version
kate_la_SOURCES = katemain.cpp
kate_la_LIBADD = libkateinterfaces.la
kate_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version 

kwrite_la_SOURCES = kwritemain.cpp katefiledialog.cpp kwritedialogs.cpp
kwrite_la_LIBADD   = -lktexteditor
kwrite_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version

kate_SOURCES = dummy_kate.cpp
kate_LDADD = kate.la
kate_LDFLAGS = $(KDE_RPATH) $(all_libraries)

kwrite_SOURCES = dummy_kwrite.cpp
kwrite_LDADD = kwrite.la
kwrite_LDFLAGS = $(KDE_RPATH) $(all_libraries)

dummy_kate_la.cpp:
	echo > dummy_kate_la.cpp

dummy_kate.cpp:
	echo > dummy_kate.cpp

dummy_kwrite.cpp:
	echo > dummy_kwrite.cpp
                         
METASOURCES = AUTO  
  
INCLUDES= -I../interfaces -I$(srcdir)/../lib $(all_includes)

