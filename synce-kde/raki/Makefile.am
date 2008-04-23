## Makefile.am for raki

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = raki

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/raki/interfaces $(all_includes)

# the library search path.
raki_LDFLAGS = $(KDE_RPATH) $(ORANGE_LDFLAGS) $(LIBRRA_LDFLAGS) \
	$(UNSHIELD_LDFLAGS) $(DYNAMITE_LDFLAGS) $(all_libraries) \
	-Wl,--export-dynamic --whole-archive -Wno-non-virtual-dtor

# the libraries to link against.
raki_LDADD = $(top_builddir)/raki/interfaces/libinterfaces.a $(LIB_KDEUI) $(LIB_KABC) $(LIBORANGE_LIBS) $(LIBRRA_LIBS) $(UNSHIELD_LIB) $(DYNAMITE_LIB)

# which sources should be compiled for raki
raki_SOURCES = management.ui raki.cpp rakiworkerthread.cpp runwindow.ui \
	configdialog.ui runwindowimpl.cpp managerimpl.cpp configdialogimpl.cpp \
	workerthreadinterface.cpp installer.cpp pda.cpp installdialog.ui passworddialog.ui \
	passworddialogimpl.cpp pdaconfigdialog.ui pdaconfigdialogimpl.cpp rra.cpp syncdialog.ui \
	syncdialogimpl.cpp synctasklistitem.cpp removepartnershipdialog.ui \
	removepartnershipdialogimpl.cpp welcomedialogimpl.cpp welcomedialog.ui threadevent.cpp initprogress.ui \
	rapiwrapper.cpp matchmaker.cpp rakiapi.cpp

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) -j *.cpp -o $(podir)/synce-kde.pot

# this is where the kdelnk file will go 
kdelnkdir = $(kde_appsdir)/Utilities
kdelnk_DATA = raki.desktop

kdemimedir = $(kde_mimedir)/application
kdemime_DATA = x-raki.desktop

rcdir = $(kde_datadir)/raki/scripts


konqdir = $(kde_datadir)/konqueror/servicemenus
konq_DATA= cab_install.desktop

tipsdir = $(kde_datadir)/raki
tips_DATA = tips

AM_CFLAGS =  
AM_CXXFLAGS = @LIBRRA_CFLAGS@ @LIBORANGE_CFLAGS@
AM_LDFLAGS = @LIBRRA_LIBS@ @LIBORANGE_LIBS@
AM_FFLAGS = 
SUBDIRS = plugins interfaces
include_HEADERS = rra.h rapiwrapper.h rakiapi.h
rc_SCRIPTS =  dccm.sh vdccm.sh
noinst_HEADERS = configdialog.ui.h configdialogimpl.h installer.h \
	management.ui.h managerimpl.h passworddialogimpl.h pda.h pdaconfigdialog.ui.h \
	pdaconfigdialogimpl.h raki.h rakiworkerthread.h rapiwrapper.h removepartnershipdialogimpl.h \
	runwindow.ui.h runwindowimpl.h syncdialogimpl.h synctasklistitem.h threadevent.h \
	welcomedialogimpl.h workerthreadinterface.h matchmaker.h
