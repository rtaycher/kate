project(kate)

add_subdirectory( interfaces )
add_subdirectory( app )
add_subdirectory( plugins )
add_subdirectory( data )
add_subdirectory( pics )
if (Q_WX_X11)
  add_subdirectory( sessionmenu )
endif (Q_WX_X11)

