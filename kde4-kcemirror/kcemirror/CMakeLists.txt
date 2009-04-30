
include_directories(${SYNCE_INCLUDE_DIRS} ${KDE4_INCLUDES} ${KDE4_INCLUDE_DIR} ${QT_INCLUDES} )


set(kcemirror_SRCS
    imageviewer.cpp
    cescreen.cpp
    decoder.cpp
    rledecoder.cpp
    huffmandecoder.cpp
    xordecoder.cpp
    rapiwrapper.cpp
    kcemirror.cpp)

kde4_add_executable(kcemirror ${kcemirror_SRCS})

target_link_libraries(kcemirror ${SYNCE_LINK_FLAGS} ${KDE4_KDECORE_LIBS} ${KDE4_KIO_LIBS} ${KDE4_KDEUI_LIBS})

install(TARGETS kcemirror ${INSTALL_TARGETS_DEFAULT_ARGS})


install(FILES  DESTINATION /share/applnk/Utilities)
