## Makefile.am for raki

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = raki 

# set the include path for X, qt and KDE
INCLUDES = -I$(top_srcdir)/raki/interfaces $(all_includes)

# the library search path. 
raki_LDFLAGS = $(KDE_RPATH) $(all_libraries) -rdynamic --whole-archive

# the libraries to link against.
raki_LDADD = $(top_builddir)/raki/interfaces/libinterfaces.a $(LIB_KCAL) $(LIB_KABC) $(LIB_KFILE) $(LIB_KDEPRINT)

# which sources should be compiled for raki
raki_SOURCES = management.ui management.ui.h raki.cpp raki.h rakiworkerthread.cpp rakiworkerthread.h rapiwrapper.cpp rapiwrapper.h runwindow.ui runwindow.ui.h configdialog.ui configdialog.ui.h runwindowimpl.h runwindowimpl.cpp managerimpl.h managerimpl.cpp configdialogimpl.h configdialogimpl.cpp workerthreadinterface.h workerthreadinterface.cpp installer.h installer.cpp pda.h pda.cpp installdialog.ui passworddialog.ui passworddialogimpl.h passworddialogimpl.cpp pdaconfigdialog.ui pdaconfigdialog.ui.h pdaconfigdialogimpl.cpp pdaconfigdialogimpl.h rra.h rra.cpp syncdialog.ui syncdialogimpl.h syncdialogimpl.cpp synctasklistitem.cpp synctasklistitem.h dmalloc.cpp wince_ids.h wince_ids.cpp removepartnershipdialog.ui removepartnershipdialogimpl.h removepartnershipdialogimpl.cpp welcomedialogimpl.h welcomedialogimpl.cpp welcomedialog.ui threadevent.h threadevent.cpp syncthread.h syncthread.cpp

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
rc_DATA = vdccm.sh dccm.sh

konqdir = $(kde_datadir)/konqueror/servicemenus
konq_DATA= cab_install.desktop

tipsdir = $(kde_datadir)/raki
tips_DATA = tips

AM_CFLAGS = 
AM_CXXFLAGS = 
AM_FFLAGS = 
SUBDIRS = plugins interfaces
include_HEADERS =  rra.h rapiwrapper.h syncthread.h
