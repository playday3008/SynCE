unix {
  UI_DIR = .ui
  MOC_DIR = .moc
  OBJECTS_DIR = .obj
}
FORMS	= configdialog.ui \
	management.ui \
	progressbar.ui \
	runwindow.ui
IMAGES	= hi16-app-raki.png \
	hi16-app-raki_bw.png \
	hi22-app-raki.png \
	hi22-app-raki_bw.png \
	hi32-app-raki.png \
	hi32-app-raki_bw.png \
	hi48-app-raki.png \
	hi48-app-raki_bw.png \
	hi64-app-raki.png \
	hi64-app-raki_bw.png
TEMPLATE	=app
CONFIG	+= qt warn_on release
LANGUAGE	= C++
