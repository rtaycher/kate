EXTRA_DIST = builder.sh TODO

noinst_LTLIBRARIES = libpiper.la
bin_PROGRAMS = testor dcop_kate

libpiper_la_SOURCES = piper.cpp

testor_SOURCES = test.cpp
testor_LDADD = libpiper.la -lkdecore
testor_LDFLAGS = $(KDE_RPATH) $(all_libraries)

dcop_kate_SOURCES = dcop_kate.cpp
dcop_kate_LDADD = libpiper.la -lkdecore -lDCOP
dcop_kate_LDFLAGS = $(KDE_RPATH) $(all_libraries)

INCLUDES= $(all_includes)

METASOURCES = AUTO

test:  testor
	./testor