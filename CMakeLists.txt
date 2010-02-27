# Kate project
project (kate)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include (KDE4Defaults)
include (MacroLibrary)

# use our own copies of some stuff
set (KDE4_KTEXTEDITOR_LIBS "ktexteditor")

# enforce build of tests
set (KDE4_BUILD_TESTS "ON" CACHE "BOOL" "Enable building of tests" FORCE)

# add some definitions needed
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

# ktexteditor interfaces, copy from kdelibs/interfaces/ktexteditor
macro_optional_add_subdirectory (ktexteditor)

# kate, copy from kdesdk/kate
macro_optional_add_subdirectory (kate)

# kwrite, copy from kdebase/apps/kwrite
macro_optional_add_subdirectory (kwrite)

# testing area, just for experimenting with new stuff
macro_optional_add_subdirectory (playground)

# global unit tests
macro_optional_add_subdirectory (tests)
