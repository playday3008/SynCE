
CFLAGS = -Wall -ggdb
INCLUDES = -I$(top_srcdir)/libwine -I$(top_srcdir)/client/librapi $(all_includes)
LDADD = $(top_builddir)/client/librapi/librapi.la $(top_builddir)/libchunk/libchunk.la $(top_builddir)/libwine/libwine.la

## pcp - PocketCoPy
pcp_SOURCES = test.h pcp.cpp

## sources for the tests
CeRapiInit_SOURCES = test.h CeRapiInit.cpp
CeGetSpecialFolderPath_SOURCES = test.h CeGetSpecialFolderPath.cpp
CeRegOpenKeyEx_SOURCES = test.h CeRegOpenKeyEx.cpp
CeFindFirstFile_SOURCES = test.h CeFindFirstFile.cpp
CeCreateFile_SOURCES = test.h CeCreateFile.cpp
CeFindAllFiles_SOURCES = test.h CeFindAllFiles.cpp
CeFindAllDatabases_SOURCES = test.h CeFindAllDatabases.cpp
CeOpenDatabase_SOURCES = test.h CeOpenDatabase.cpp
CeRegEnumKeyEx_SOURCES = test.h CeRegEnumKeyEx.cpp
CeRegEnumValue_SOURCES = test.h CeRegEnumValue.cpp
CeRegQueryInfoKey_SOURCES = test.h CeRegQueryInfoKey.cpp
CeCreateDirectory_SOURCES = test.h CeCreateDirectory.cpp
CeGetVersionEx_SOURCES = test.h CeGetVersionEx.cpp
CeMoveFile_SOURCES = test.h CeMoveFile.cpp

## all tests
noinst_PROGRAMS = pcp \
	CeRapiInit CeGetSpecialFolderPath CeRegOpenKeyEx \
	CeFindFirstFile CeCreateFile CeFindAllFiles CeFindAllDatabases \
	CeOpenDatabase CeRegEnumKeyEx CeRegEnumValue CeRegQueryInfoKey \
	CeCreateDirectory CeGetVersionEx CeMoveFile
	

## run "make test" to run the tests

test: $(noinst_PROGRAMS)
	./CeRapiInit
	./CeGetSpecialFolderPath
	./CeRegOpenKeyEx
	./CeFindFirstFile
	./CeCreateFile
	./CeFindAllFiles
	./CeFindAllDatabases
##	./CeOpenDatabase
	./CeRegEnumKeyEx
##	./CeRegEnumValue
	./CeRegQueryInfoKey
	./CeCreateDirectory
	./CeGetVersionEx

