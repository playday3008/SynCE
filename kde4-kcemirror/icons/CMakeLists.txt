
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

message(STATUS "${CMAKE_CURRENT_SOURCE_DIR}: skipped subdir $(AUTODIRS)")

########### install files ###############


kde4_install_icons(${ICON_INSTALL_DIR})



#original Makefile.am contents follow:

#SUBDIRS = $(AUTODIRS)
#KDE_ICON = kcemirror
