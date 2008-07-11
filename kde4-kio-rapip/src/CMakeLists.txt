set(kio_rapip_SRCS
   rapip.cpp
)

kde4_add_plugin(kio_rapip ${kio_rapip_SRCS})
target_link_libraries(kio_rapip ${SYNCE_LINK_FLAGS} ${KDE4_KIO_LIBS})
install(TARGETS kio_rapip DESTINATION ${PLUGIN_INSTALL_DIR})

install( FILES rapip.protocol DESTINATION ${SERVICES_INSTALL_DIR} )
