## useful flags
SRC = ../../src
CXXFLAGS = -ggdb -Wall -I$(SRC) -I$(SRC)/support
LDADD = $(SRC)/librapi.la

noinst_PROGRAMS = \
	CeCreateDirectory \
	CeCreateFile \
	CeFindAllDatabases \
	CeFindAllFiles \
	CeFindFirstFile \
	CeGetVersionEx \
	CeGetSpecialFolderPath \
	CeMoveFile \
	CeOpenDatabase \
	CeRapiInit

CeRapiInit_SOURCES = test.h CeRapiInit.cpp
CeGetVersionEx_SOURCES = test.h CeGetVersionEx.cpp
CeGetSpecialFolderPath_SOURCES = test.h CeGetSpecialFolderPath.cpp
CeCreateFile_SOURCES = test.h CeCreateFile.cpp
CeFindFirstFile_SOURCES = test.h CeFindFirstFile.cpp
CeFindAllFiles_SOURCES = test.h CeFindAllFiles.cpp
CeCreateDirectory_SOURCES = test.h CeCreateDirectory.cpp
CeMoveFile_SOURCES = test.h CeMoveFile.cpp
CeOpenDatabase_SOURCES = test.h CeOpenDatabase.cpp
CeFindAllDatabases_SOURCES = test.h CeFindAllDatabases.cpp

