# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/src/lib -I$(top_srcdir)/src/libPIM $(all_includes)

# these are the headers for your project
noinst_HEADERS = pocketpccommunication.h PocketPCMainWindow.h PdaNameDialog.h Preferences.h KSyncTest.h

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/pocketpccommunication.pot

KDE_ICON = pocketpccommunication

#########################################################################
# APPLICATION SECTION
#########################################################################
# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = pocketpccommunication

# the application source, library search path, and link libraries
pocketpccommunication_SOURCES = main.cpp pocketpccommunication.cpp PocketPCMainWindowUI.ui PocketPCMainWindow.cpp PdaNameDialogUI.ui PdaNameDialog.cpp PreferencesUI.ui Preferences.cpp KSyncTest.cpp
pocketpccommunication_LDFLAGS = $(KDE_RPATH) $(all_libraries)
pocketpccommunication_LDADD = $(top_builddir)/src/libPIM/libPocketPCPIM.la \
	$(top_builddir)/src/lib/libkpocketpccomm.la $(top_builddir)/src/libPocketPCKonnector/libpocketpckonnector.la -lsynce -lrapi -lrra -lkcal -lkonnector -lksync2 -lkabc_file -lkabc \
	$(LIB_KDEUI)

# this is where the desktop file will go 
shelldesktopdir = $(kde_appsdir)/Utilities
shelldesktop_DATA = pocketpccommunication.desktop

# this is where the shell's XML-GUI resource file goes
shellrcdir = $(kde_datadir)/pocketpccommunication
shellrc_DATA = pocketpccommunicationui.rc

SUBDIRS = libPIM lib libPocketPCKonnector
pocketpccommunication_DEPENDENCIES = 
