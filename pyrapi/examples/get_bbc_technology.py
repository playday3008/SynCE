#!/usr/bin/python
"""
    Simple script to pull the BCC Technology news and force it into a
    format that can be downloaded on the the PDA. This is then
    downloaded to a file in My Documents.

    The script is meant to be placed in the ~/.synce/scripts directory
    and will run when the PocketPC is attached.

"""
from pyrapi import pyrapi
from pyrapi import rapi

import urllib2, urlparse
import re,sys

def fetchAndFix(url):
    
    web_page = urllib2.urlopen(url).read()

    start = re.search('<h3>TECHNOLOGY</h3>', web_page)
    end = re.search('<form action="http://newssearch.bbc.co.uk/cgi-bin/results.pl">', web_page)

    midsection = web_page[start.start():end.start()]


    reference = 1
    previous = None
    sub_pages = {}

    for link in re.finditer('<a href="(/1/low/\S*)">', midsection):
        if previous == None:
            res = midsection[:link.start(1)] + "#n"+str(reference)
        else:
            res = res + midsection[previous.end(1):link.start(1)] + "#n"+str(reference)


        sub_url = urlparse.urlparse(url)[0]+"://"+urlparse.urlparse(url)[1]+midsection[link.start(1):link.end(1)]
        #print "Fetching ", sub_url
        
        new_page = urllib2.urlopen(sub_url).read()
        body_start = re.search('<body.*>',new_page)
        body_end   = re.search('<form',new_page)
        
        sub_pages[reference] = new_page[body_start.end():body_end.start()]

        previous = link
        reference = reference + 1
            
    res = res + midsection[previous.end(1):]

    for ref in range(1,reference):
        res = res + '<a name="n'+str(ref)+'"></a>' +sub_pages[ref]

    return  res

##t = open("/home/rjt/errorlog","w")
##t.write(repr(sys.argv))
##t.close()

if sys.argv[1] == "connect":

    page = fetchAndFix('http://news.bbc.co.uk/1/low/technology/default.stm')

    ##f = open("outfile.html",'w')
    ##f.write("<html><head><title>BBC Tech News</title></head><body>\n")
    ##f.write(page)
    ##f.write("</body></html>\n")
    ##f.close()


    f = rapi.openCeFile('\\My Documents\\bbc_tech.htm','w')
    f.write("<html><head><title>BBC Tech News</title></head><body>\n")
    f.write(page)
    f.write("</body></html>\n")
    f.close()
