
# Style sheet locations
STYLE_SHEET_ROOT      = /usr/share/sgml/docbook/xsl-stylesheets-1.50.0
HTML_ONE_STYLESHEET   = ${STYLE_SHEET_ROOT}/html/docbook.xsl
HTML_CHUNK_STYLESHEET = ${STYLE_SHEET_ROOT}/html/chunk.xsl
FO_STYLESHEET         = ${STYLE_SHEET_ROOT}/fo/docbook.xsl

# Document files
DOC_DIR      = docbook
ROOT_DOC     = synce.xml  # Main root node file

# Output locations
ONE_FILE_OUT = synce.html # Output file name for one-in-one html
HTML_DIR     = html
ONE_FILE_DIR = ${HTML_DIR}/all-in-one
CHUNK_DIR    = ${HTML_DIR}/chunk
PDF_DIR      = pdf

# Location of xsl processor
xsltproc = xsltproc

help:
	@echo 
	@echo "-----------------------------------------------------"
	@echo "This makefile contains the following targets."
	@echo 
	@echo "setup       - create output directories."
	@echo "html-one    - make document in one html file"
	@echo "html-chunk  - make document split into seperate files."
	@echo "all         - make all document types."
	@echo "clean-one   - remove all-in-one html file."
	@echo "clean-chunk - remove split html files."
	@echo "clean-all   - remove all generated files."
	@echo "-----------------------------------------------------"
	@echo

setup: force
	mkdir -p ${ONE_FILE_DIR}
	mkdir -p ${CHUNK_DIR}
	mkdir -p ${PDF_DIR}

html-one: setup
	PWD=`pwd`
	(cd ${DOC_DIR}; ${xsltproc}  -o ${PWD}/${ONE_FILE_DIR}/${ONE_FILE_OUT} ${HTML_ONE_STYLESHEET} ${ROOT_DOC})

html-chunk: setup
	(cd ${DOC_DIR}; ${xsltproc} -o ${PWD}/${CHUNK_DIR}/ ${HTML_CHUNK_STYLESHEET} ${ROOT_DOC})

pdf: setup
	(cd ${DOC_DIR}; ${xsltproc} -o ${PWD}/${PDF_DIR}/out.fo ${FO_STYLESHEET} ${ROOT_DOC})	

all: html-chunk html-one


clean-one:
	/bin/rm ${ONE_FILE_DIR}/*.html

clean-chunk:
	/bin/rm ${CHUNK_DIR}/*.html

clean-pdf:
	/bin/rm ${PDF_DIR}/*

clean-all:
	/bin/rm -rf ${HTML_DIR}
	/bin/rm -rf ${PDF_DIR}
force:



