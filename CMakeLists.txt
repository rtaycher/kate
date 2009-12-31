project(ArtisticComment)

find_package(KDE4 REQUIRED)
include (KDE4Defaults)
add_definitions (${QT_DEFINITIONS} ${KDE4_DEFINITIONS} -ffor-scope)
include_directories(${KDE4_INCLUDES})

set(acomment_SRCS artisticcomment.cpp)
set(kte_acomment_SRCS )

kde4_add_library(acomment SHARED ${acomment_SRCS})

target_link_libraries(acomment QtCore)

# kde4_add_plugin(kte_acomment ${kte_acomment_SRCS})

install(TARGETS acomment ${INSTALL_TARGETS_DEFAULT_ARGS})

install(FILES artisticcomment.h DESTINATION ${INCLUDE_INSTALL_DIR} COMPONENT Devel)
