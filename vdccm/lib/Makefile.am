# set the include path found by configure
INCLUDES = $(all_includes)

# the library search path.
noinst_LTLIBRARIES = libdescriptor.la
libdescriptor_la_SOURCES = continousnode.cpp descriptor.cpp \
			descriptormanager.cpp multiplexer.cpp singleshotnode.cpp timernode.cpp timernodemanager.cpp \
			tcpsocket.cpp tcpserversocket.cpp tcpconnectedsocket.cpp tcpclientsocket.cpp \
			localsocket.cpp localserversocket.cpp localconnectedsocket.cpp localclientsocket.cpp \
			tcpacceptedsocket.cpp localacceptedsocket.cpp netsocket.cpp udpsocket.cpp

noinst_HEADERS = continousnode.h descriptor.h \
			descriptormanager.h	localacceptedsocket.h localclientsocket.h localconnectedsocket.h	localserversocket.h \
			localsocket.h multiplexer.h singleshotnode.h	tcpacceptedsocket.h tcpclientsocket.h \
			tcpconnectedsocket.h	tcpserversocket.h tcpsocket.h timernode.h timernodemanager.h tcpacceptedsocketfactory.h \
			localacceptedsocketfactory.h netsocket.h udpsocket.h
