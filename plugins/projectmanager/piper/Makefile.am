EXTRA_DIST = builder.sh TODO

noinst_LTLIBRARIES = libpiper.la
bin_PROGRAMS = testor dcop_kant

libpiper_la_SOURCES = piper.cpp

testor_SOURCES = test.cpp
testor_LDADD = libpiper.la -lkdecore
testor_LDFLAGS = $(KDE_RPATH) $(all_libraries)

dcop_kant_SOURCES = dcop_kant.cpp
dcop_kant_LDADD = libpiper.la -lkdecore -lDCOP
dcop_kant_LDFLAGS = $(KDE_RPATH) $(all_libraries)

INCLUDES= $(all_includes)

METASOURCES = AUTO

test:  testor
	./testor