## useful flags
SRC = ../../src
CXXFLAGS = -ggdb -Wall -I$(SRC) -I$(SRC)/support
LDADD = $(SRC)/librapi.la

noinst_PROGRAMS = \
	CeCreateFile \
	CeGetVersionEx \
	CeGetSpecialFolderPath \
	CeRapiInit

CeRapiInit_SOURCES = test.h CeRapiInit.cpp
CeGetVersionEx_SOURCES = test.h CeGetVersionEx.cpp
CeGetSpecialFolderPath_SOURCES = test.h CeGetSpecialFolderPath.cpp
CeCreateFile_SOURCES = test.h CeCreateFile.cpp

