#!/usr/bin/make -f
# test
# Stylesheet paths
xsl_prefix=http://docbook.sourceforge.net/release/xsl/current
html_docbook=${xsl_prefix}/html/docbook.xsl
html_chunk=${xsl_prefix}/html/chunk.xsl

# Commands
xsltproc=xsltproc

# Directories
html_dir=html
chunk_dirname=chunk
all_dirname=all
html_all_dir=${html_dir}/${all_dirname}
html_chunk_dir=${html_dir}/${chunk_dirname}
archive_dir=archive

# Filenames
html_all_file=synce-docs.html
xmlfile=synce-docs.xml
archive_html_all=synce-docs-all.tar.bz2
archive_html_chunk=synce-docs-chunk.tar.bz2

# XSL extra parameters
xsl_params=--stringparam formal.title.placement "table after" \
           --stringparam section.autolabel 1 \
           --stringparam section.label.includes.component.label 1 \
           --stringparam ulink.footnotes 1


###############################
# Start Makefile
###############################

all: html archives

html: html-all html-chunk

html-all: prepare-html-all
	${xsltproc} ${xsl_params} -o ${html_all_dir}/${html_all_file} ${html_docbook} ${xmlfile}

html-chunk: prepare-html-chunk
	${xsltproc} ${xsl_params} -o ${html_chunk_dir}/ ${html_chunk} ${xmlfile}

archives: prepare-archives html
	tar --bzip2 --file=${archive_html_chunk} --directory=${html_dir} --create ${chunk_dirname}
	tar --bzip2 --file=${archive_html_all} --directory=${html_dir} --create ${all_dirname}
	mv ${archive_html_chunk} ${archive_dir}
	mv ${archive_html_all} ${archive_dir}

prepare-archives:
	mkdir -p archive

prepare-html-all:
	mkdir -p html/all

prepare-html-chunk:
	mkdir -p html/chunk

clean:
	rm -rf html
	rm -rf archive
