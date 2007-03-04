# PYRTFCOMP.PYX
#
# Python wrappers for librtfcomp
#
# Copyright 28/2/2007 Dr J A Gow
#
# This file is released under the terms and conditions of the LGPL - please see the LICENCE
# file in the package root directory.

cdef extern from "rtfcomp/rtfcomp.h":

	ctypedef struct RTFOPTS:
		int		lenOpts
		unsigned int 	isCompressed

	int LZRTFCompress(unsigned char ** dest, unsigned int * outlen, unsigned char * src, int len)
	int LZRTFDecompress(unsigned char ** dest, unsigned int * outlen, unsigned char * src, unsigned int len)
	int LZRTFConvertRTFToUTF8(unsigned char ** utfout, unsigned int * utflen, unsigned char * rtfin, unsigned int rtflen, RTFOPTS * options)
	int LZRTFConvertUTF8ToRTF(unsigned char ** rtfout, unsigned int * lenOut, unsigned char * utfin, unsigned int len, unsigned char * rtfhdr, unsigned int hdrlen, RTFOPTS * options)
	char * LZRTFGetStringErrorCode(int ec)

cdef extern from "Python.h":
	char *PyString_AsString(object string)
	object PyString_FromStringAndSize(char *s, int len)
	int PyString_AsStringAndSize(object obj, char **buffer, int *length)

cdef extern from "stdlib.h":
	void free(void * ptr)

class RTFException:
	def __init__(self,ec):
		self.errorcode = ec;
		self.strdesc   = <char *>LZRTFGetStringErrorCode(ec)

	def dump(self):
		print "Failed to convert: %s" % self.strdesc

def RTFCompress(src):
	cdef unsigned char * result
	cdef unsigned int reslen
	cdef int rc

	rc=LZRTFCompress(&result,&reslen,<unsigned char *>PyString_AsString(src),len(src))
	
	if rc != 0:
		raise RTFException(rc)

	rstr = PyString_FromStringAndSize(<char *>result,reslen)
	free(result)
	return rstr

def RTFDecompress(src):
	cdef unsigned char * result
	cdef unsigned int reslen
	cdef int rc

	rc=LZRTFDecompress(&result,&reslen,<unsigned char *>PyString_AsString(src),len(src))

	if rc != 0:
		raise RTFException(rc)

	rstr = PyString_FromStringAndSize(<char *>result, reslen)
	free(result)
	return rstr

def RTFConvertToUTF8(src, isCompressed):
	cdef RTFOPTS opts
	cdef unsigned char * result
	cdef unsigned int    reslen
	cdef int rc

	opts.lenOpts = sizeof(RTFOPTS)
	opts.isCompressed = isCompressed

	rc=LZRTFConvertRTFToUTF8(&result, &reslen, <unsigned char *>PyString_AsString(src),len(src),&opts)

	if rc != 0:
		raise RTFException(rc)

	rstr = PyString_FromStringAndSize(<char *>result, reslen)
	free(result)
	return rstr

def RTFConvertFromUTF8(src, header, isCompressed):
	cdef RTFOPTS opts
	cdef unsigned char * result
	cdef unsigned int    reslen
	cdef int rc

	opts.lenOpts = sizeof(RTFOPTS)
	opts.isCompressed = isCompressed

	rc=LZRTFConvertUTF8ToRTF(&result, &reslen, <unsigned char *>PyString_AsString(src),len(src),<unsigned char *>PyString_AsString(header), len(header), &opts)

	if rc != 0:
		raise RTFException(rc)

	rstr = PyString_FromStringAndSize(<char *>result, reslen)
	free(result)
	return rstr

