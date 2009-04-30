
include_directories(${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )

add_subdirectory(ARMRel)
add_subdirectory(MIPSRel)
add_subdirectory(SH3Rel)


########### install files ###############




#original Makefile.am contents follow:

#SUBDIRS = ARMRel MIPSRel SH3Rel
#
#EXTRA_DIST = encoder.cpp huffmanencoder.cpp screensnap.cpp snap.cpp \
#xorencoder.cpp eventprocessor.cpp rleencoder.cpp serversocket.cpp \
#vnckeymap.cpp encoder.h huffmanencoder.h rleencoder.h snap.h xorencoder.h \
#eventprocessor.h keysymdef.h serversocket.h vnckeymap.h \
#screensnap.vcp screensnap.vcw
