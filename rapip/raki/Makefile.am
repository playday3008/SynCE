## Makefile.am for raki

# this is the program that gets installed.  it's name is used for all
# of the other Makefile.am variables
bin_PROGRAMS = raki 

# set the include path for X, qt and KDE
INCLUDES         = $(all_includes)

# the library search path. 
raki_LDFLAGS = $(KDE_RPATH) $(all_libraries)

# the libraries to link against.
raki_LDADD = $(LIB_KFILE) $(LIB_KDEPRINT)

# which sources should be compiled for raki
raki_SOURCES = errorevent.cpp errorevent.h management.ui management.ui.h raki.cpp raki.h rakievent.cpp rakievent.h rakiworkerthread.cpp rakiworkerthread.h rapiwrapper.cpp rapiwrapper.h runwindow.ui runwindow.ui.h configdialog.ui configdialog.ui.h runwindowimpl.h runwindowimpl.cpp managerimpl.h managerimpl.cpp configdialogimpl.h configdialogimpl.cpp workerthreadinterface.h workerthreadinterface.cpp installer.h installer.cpp

# let automoc handle all of the meta source files (moc)
METASOURCES = AUTO

messages: rc.cpp
	$(XGETTEXT) *.cpp -o $(podir)/raki.pot

noinst_SCRIPTS =  raki.sh

KDE_ICON = raki raki_bw

# this is where the kdelnk file will go 
kdelnkdir   = $(kde_appsdir)/Utilities
kdelnk_DATA = raki.desktop

# this is where the XML-GUI resource file goes
rcdir = $(kde_datadir)/raki
