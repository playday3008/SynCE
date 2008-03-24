from PyQt4.QtCore import *
from PyQt4.QtGui import *
from PyQt4 import *
from dbus.mainloop.qt import DBusQtMainLoop

from synceKPM.gui.mainwindow import *
import synceKPM.constants
from pkg_resources import resource_filename


import sys

def main(startIconified):
    print "running the GUI Part"

    app = QtGui.QApplication(sys.argv)

    DBusQtMainLoop(set_as_default = True)
	#dbus.mainloop.qt.DBusQtMainLoop
   
    abortStart=False
	
    try:
        _gui = dbus.SessionBus().get_object("org.synce.kpm.gui",synceKPM.constants.DBUS_SYNCEKPM_GUI_OBJPATH)
        _gui.ShowScreen(dbus_interface=synceKPM.constants.DBUS_SYNCEKPM_GUI_IFACE)
        abortStart = True
    except Exception,e :
        pass

    if abortStart:
        sys.exit(0) 


    pixmap = QPixmap("%s/splashscreen.png"%resource_filename('synceKPM','data'));
    splash = QSplashScreen(pixmap);
    splash.show();
    app.processEvents();

    QTimer.singleShot(2000, splash.hide)

    mainWindow = mainwindow()
    app.processEvents();
    
    if not startIconified:
        mainWindow.show()
    
    app.setQuitOnLastWindowClosed(False)	
    app.exec_()
