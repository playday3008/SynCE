## Makefile.am for raki

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = raki 

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/raki/interfaces $(SYNCE_INCLUDES) $(RAPI_INCLUDES) $(RRA_INCLUDES) $(all_includes)

# the library search path. 
raki_LDFLAGS = $(KDE_RPATH) $(SYNCE_LDFLAGS) $(RAPI_LDFLAGS) $(RRA_LDFLAGS) $(all_libraries) -rdynamic --whole-archive

# the libraries to link against.
raki_LDADD = $(top_builddir)/raki/interfaces/libinterfaces.a $(LIB_KDEUI) $(LIB_KABC) $(RRA_LIB) $(RAPI_LIB) $(SYNCE_LIB) $(ICAL_LIB)

# which sources should be compiled for raki
raki_SOURCES = management.ui raki.cpp rakiworkerthread.cpp runwindow.ui configdialog.ui runwindowimpl.cpp managerimpl.cpp configdialogimpl.cpp workerthreadinterface.cpp installer.cpp pda.cpp installdialog.ui passworddialog.ui passworddialogimpl.cpp pdaconfigdialog.ui pdaconfigdialogimpl.cpp rra.cpp syncdialog.ui syncdialogimpl.cpp synctasklistitem.cpp dmalloc.cpp wince_ids.cpp removepartnershipdialog.ui removepartnershipdialogimpl.cpp welcomedialogimpl.cpp welcomedialog.ui threadevent.cpp syncthread.cpp initprogress.ui rapiwrapper.cpp matchmaker.cpp

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/raki.pot

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
AM_CXXFLAGS = 
AM_FFLAGS = 
SUBDIRS = plugins interfaces
include_HEADERS = rra.h rapiwrapper.h
rc_SCRIPTS =  dccm.sh vdccm.sh
noinst_HEADERS = configdialog.ui.h configdialogimpl.h installer.h management.ui.h managerimpl.h passworddialogimpl.h pda.h pdaconfigdialog.ui.h pdaconfigdialogimpl.h raki.h rakiworkerthread.h rapiwrapper.h removepartnershipdialogimpl.h runwindow.ui.h runwindowimpl.h syncdialogimpl.h synctasklistitem.h syncthread.h threadevent.h welcomedialogimpl.h wince_ids.h workerthreadinterface.h matchmaker.h
