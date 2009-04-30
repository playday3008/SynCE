
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


########### install files ###############

install(FILES  screensnap.exe.mips DESTINATION ${DATA_INSTALL_DIR}/kcemirror/exe)



#original Makefile.am contents follow:

#rcdir = $(kde_datadir)/kcemirror/exe
#
#rc_DATA = screensnap.exe.mips
