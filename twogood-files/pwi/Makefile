
SYNCE_LIB=/var/tmp/synce/lib

LDFLAGS = -L$(SYNCE_LIB) -Wl,-rpath,$(SYNCE_LIB) -lsynce
CXXFLAGS = -ggdb -Wall -ansi -pedantic

APPS = pwi

all: $(APPS)

pwi: pwi.o
	g++ -o $@ $(LDFLAGS) $< $(LIBS)

clean:
	-rm -rf $(APPS) *.o

