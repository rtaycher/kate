KDE_CXXFLAGS=-UQT_NO_COMPAT
INCLUDES=-I$(srcdir)/.. $(all_includes)
check_PROGRAMS = textedit
textedit_SOURCES = main.cpp textedit.cpp
textedit_LDADD = ../libqt3stuff.la $(LIB_QT)
textedit_LDFLAGS = $(all_libraries)
METASOURCES = AUTO
KDE_OPTIONS = qtonly
