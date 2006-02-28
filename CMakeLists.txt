kde4_header()

add_subdirectory( interfaces ) 
add_subdirectory( app ) 
add_subdirectory( plugins ) 
add_subdirectory( data ) 
add_subdirectory( pics ) 

include_directories( ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}  )


########### install files ###############


kde4_create_doxygen_docs( RECURSIVE REFERENCES kdecore kdeui EXCLUDE app data pics plugins  )

kde4_footer()



#original Makefile.am contents follow:

#SUBDIRS = interfaces app plugins data pics
#EXTRA_DIST = AUTHORS COPYING.LIB ChangeLog DESIGN NEWS README TODO
#
#messages:
#	rm -f tips.cpp
#	(cd data && $(PREPARETIPS) > ../tips.cpp)
#	$(EXTRACTRC) ./*/*.rc >> ./rc.cpp
#	$(XGETTEXT) `find . -name "*.cpp"` -o $(podir)/kate.pot
#	rm -f tips.cpp
#
#DOXYGEN_REFERENCES = kdecore kdeui
#DOXYGEN_EXCLUDE = app data pics plugins
#DOXYGEN_SET_RECURSIVE = YES
#include ../admin/Doxyfile.am
#
