## useful flags
SRC = ../../src
CXXFLAGS = -ggdb -Wall -I$(SRC) -I$(SRC)/support
LDADD = $(SRC)/librapi.la

noinst_PROGRAMS = \
	CeCreateDirectory \
	CeCreateFile \
	CeFindAllFiles \
	CeFindFirstFile \
	CeGetVersionEx \
	CeGetSpecialFolderPath \
	CeMoveFile \
	CeRapiInit

CeRapiInit_SOURCES = test.h CeRapiInit.cpp
CeGetVersionEx_SOURCES = test.h CeGetVersionEx.cpp
CeGetSpecialFolderPath_SOURCES = test.h CeGetSpecialFolderPath.cpp
CeCreateFile_SOURCES = test.h CeCreateFile.cpp
CeFindFirstFile_SOURCES = test.h CeFindFirstFile.cpp
CeFindAllFiles_SOURCES = test.h CeFindAllFiles.cpp
CeCreateDirectory_SOURCES = test.h CeCreateDirectory.cpp
CeMoveFile_SOURCES = test.h CeMoveFile.cpp

