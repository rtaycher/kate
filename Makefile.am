SUBDIRS = kwrite app console data document fileselector mainwindow pics project sidebar view pluginmanager plugins
EXTRA_DIST = piper

KDE_CXXFLAGS = $(USE_RTTI)  -DQT_CLEAN_NAMESPACE -DQT_NO_COMPAT -DQT_NO_ASCII_CAST

bin_PROGRAMS = kant

kant_SOURCES = kantmain.cpp
kant_LDADD   = -lktexteditor -lDCOP -lkparts -lkspell -lkfile -lkdeui -lkdecore \
		$(LIB_QT) $(LIBSOCKET)  ./kwrite/libkwritepart.la\
		-L./view -L./sidebar -L./project -L./mainwindow -L./fileselector\
		-L./app -L./pluginmanager  -L./document -L./console\
		-lkantview -lkantsidebar ./project/kantprojectmanager.o\
		./project/kantprojectdialog.o -lkantmainwindow	-lkantfileselector\
		-lkantdocument -lkantconsole\
		./app/kantapp.o  -lkantpluginmanager\
		-lkantpluginiface

kant_DEPENDENCIES = ./*/*.a ./*/*.la


EXTRA_DIST = kantmain.cpp kantmain.h

INCLUDES= $(all_includes)

METASOURCES = AUTO

kant_LDFLAGS = $(all_libraries) $(KDE_RPATH)

messages: rc.cpp
	$(XGETTEXT) -C -ki18n -kI18N_NOOP -ktranslate -x $(kde_includes)/kde.pot *.cpp -o $(podir)/kant.pot

rc.cpp:
	$(EXTRACTRC) *.rc *.ui */*.rc */*.ui */*/*.rc */*/*.ui > rc.cpp
