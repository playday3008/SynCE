cd src
swig -I/usr/include -I/usr/local/include $(pkg-config --cflags librapi2) -python -shadow -o pyrapi_wrap.c pyrapi_wrap.i

