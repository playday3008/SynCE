cmake_minimum_required(VERSION 2.0)

project(kde4-kio-rapip)

include (InstallRequiredSystemLibraries)

# configuration to build source packages
set (CPACK_PACKAGE_NAME "kde4-kio-rapip")
set (CPACK_PACKAGE_VERSION "0.3")
set (CPACK_PACKAGE_VENDOR "The SynCE Project")
set (CPACK_PACKAGE_DESCRIPTION_FILE ${CMAKE_CURRENT_SOURCE_DIR}/README )
set (CPACK_PACKAGE_DESCRIPTION_SUMMARY "KIOSlave used to browse through a Windows Mobile PDA filesystem" )
set (CPACK_RESOURCE_FILE_LICENSE ${CMAKE_CURRENT_SOURCE_DIR}/LICENSE )
set (CPACK_SOURCE_GENERATOR "TGZ;TBZ2")
set (CPACK_SOURCE_PACKAGE_FILE_NAME kde4-kio-rapip-${CPACK_PACKAGE_VERSION} )
set (CPACK_CMAKE_GENERATOR "Unix Makefiles")
include (CPack)

# search packages used by KDE
find_package(KDE4 REQUIRED)
include (FindPkgConfig)
include (KDE4Defaults)
include (MacroLibrary)
include(MacroOptionalAddSubdirectory)

pkg_check_modules(SYNCE REQUIRED libsynce>=0.17)

add_definitions (${SYNCE_CFLAGS} ${QT_DEFINITIONS} ${KDE4_DEFINITIONS})
include_directories (${SYNCE_INCLUDE_DIRS} ${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${KDE4_INCLUDES})

add_subdirectory(src)
add_subdirectory(icons)
add_subdirectory(data)
