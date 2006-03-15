lib_LTLIBRARIES = libkateinterfaces.la
bin_PROGRAMS =
kdeinit_LTLIBRARIES = kate.la

libkateinterfaces_la_SOURCES = kateapp.cpp kateconfigdialog.cpp kateconfigplugindialogpage.cpp \
  katedocmanager.cpp katefilelist.cpp \
  katemainwindow.cpp katepluginmanager.cpp \
  kateviewmanager.cpp kateviewspace.cpp \
  katemainwindowiface.skel katemainwindowiface.cpp kategrepdialog.cpp \
  katemailfilesdialog.cpp katetooltipmenu.cpp\
  katedocmanageriface.skel kateappIface.cpp kateappIface.skel katedocmanageriface.cpp \
  kateexternaltools.cpp katesavemodifieddialog.cpp kateviewspacecontainer.cpp \
  katemwmodonhddialog.cpp katesession.cpp katemdi.cpp katetabwidget.cpp katepartmanager.cpp

libkateinterfaces_la_LIBADD   = ../interfaces/libkateinterfacesprivate.la $(LIB_KUTILS)

libkateinterfaces_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -no-undefined

kate_la_SOURCES = katemain.cpp
kate_la_LIBADD = libkateinterfaces.la
kate_la_LDFLAGS = $(all_libraries) $(KDE_RPATH) -module -avoid-version

METASOURCES = AUTO

INCLUDES= -I../interfaces -I$(srcdir)/../lib -I$(top_srcdir) $(all_includes)
