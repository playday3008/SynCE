cmake_minimum_required(VERSION 2.0)

project(kde4-kcemirror)

set(PACKAGE_VERSION "0.1.1")

# configuration to build source packages
set (CPACK_PACKAGE_NAME ${PACKAGE_NAME})
set (CPACK_PACKAGE_VERSION ${PACKAGE_VERSION})
set (CPACK_PACKAGE_VENDOR "The SynCE Project")
set (CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README )
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "KDE application to remotely view and control a Windows Mobile PDA" )
set (CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/COPYING )
set (CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
set (CPACK_SOURCE_PACKAGE_FILE_NAME kde4-kcemirror-${CPACK_PACKAGE_VERSION} )
set (CPACK_SOURCE_IGNORE_FILES
"/\\\\.svn/"
"/build/"
".*~$"
".*kdev4$" )
set (CPACK_CMAKE_GENERATOR "Unix Makefiles")
include (CPack)


find_package(KDE4 REQUIRED)
include(FindPkgConfig)
include(KDE4Defaults)
include(MacroLibrary)

pkg_check_modules( SYNCE REQUIRED libsynce )
add_definitions(${SYNCE_CFLAGS} ${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

include_directories(${CMAKE_BINARY_DIR} ${SYNCE_INCLUDE_DIRS} ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


add_subdirectory(doc)
add_subdirectory(icons)
add_subdirectory(kcemirror)
add_subdirectory(po)
add_subdirectory(screensnap)


configure_file(config.h.in config.h @ONLY)
