#!/usr/bin/python
"""Download the BBC headlines to the PDA


THIS IS CURRENTLY VERY BROKEN  :-)

""" 

from pyrapi import file
import urllib2
import re


counter = 0
links_to_fetch = {}
links_done     = {}


def nextLinkNo():
    global counter
    counter = counter + 1
    return counter

def fetchAndFix(url):
    global links_to_fetch
    global links_done
    web_page = urllib2.urlopen(url).read()

    links_done[links_to_fetch[url]] = url
    del links_to_fetch[url]
    
    print "****************** Fetching ***************************", url

    print web_page
    m = re.findall('<[aA]\s*[hH][rR][eE][fF]="[^#](\S*)"\s*>', web_page)
    print m
    for link in m:
        if (not (link in links_to_fetch.keys() or (link in links_done.keys())):
            no = nextLinkNo()
            links_to_fetch[link] = no
        else:

        if link_to_fetch.has_key(link):
            no = links_to_fetch[link]
        else:
            no = links_done[link]
            
        web_page = web_page.replace(link,str(no))
    #print m

    print web_page

    print links_to_fetch

    for link in links_to_fetch.values():
        print "****************** Fetching **************************", "http://news.bbc.co.uk/"+link
        fetchAndFix("http://news.bbc.co.uk/"+link)



fetchAndFix("http://news.bbc.co.uk/low/english/pda/")

### Fetch the web page
##web_page = urllib2.urlopen("http://news.bbc.co.uk/low/english/pda/")

### Write it to the PDA
##outfilename = r'\my documents\bbc.htm'
##outfile = file.openCeFile(outfilename,'w')
##outfile.write(web_page.read())
##outfile.close()

### Makeup the favorites entry.

##outfile = file.openCeFile(r'\windows\favorites\bbc.url','w')
##outfile.write("""[InternetShortcut]
##URL=file://%s
##Offline=1""" % outfilename)
##outfile.close()
