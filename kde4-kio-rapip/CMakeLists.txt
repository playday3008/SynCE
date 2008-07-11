project(kio_rapip)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include (UsePkgConfig)
include (KDE4Defaults)
include (MacroLibrary)
include(MacroOptionalAddSubdirectory)

PKGCONFIG(librapi2 SYNCE_INCLUDE_DIR SYNCE_LINK_DIR SYNCE_LINK_FLAGS SYNCE_CFLAGS)

add_definitions (${QT_DEFINITIONS} ${SYNCE_CFLAGS} ${KDE4_DEFINITIONS})
include_directories (${SYNCE_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

add_subdirectory(src)
add_subdirectory(icons)
