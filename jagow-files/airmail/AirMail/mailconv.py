###############################################################################
# MAILCONV.py
#
# Classes to handle mail conversions and synchronization
#
# Dr J A Gow 2007
#
###############################################################################

import xml2util
import libxml2
import time
import email
import util

#
#
# Options
#
# Simple class containing a group of options sent from the device, usually
# on a per-collection basis
#

class Options:
	
	def __init__(self,optnode):
		
		if optnode != None:
		
			nFilterType = xml2util.FindChildNode(optnode,"FilterType")
			if nFilterType == None:
				self.FilterType = 2
			else:
				self.FilterType = int(xml2util.GetNodeValue(nFilterType))
			
			nTruncation = xml2util.FindChildNode(optnode,"Truncation")
			if nTruncation == None:
				self.Truncation = 4
			else:
				self.Truncation = int(xml2util.GetNodeValue(nTruncation))
			
			nConflict = xml2util.FindChildNode(optnode,"Conflict")
			if nConflict == None:
				self.Conflict = 1
			else:
				self.Conflict = int(xml2util.GetNodeValue(nConflict))
		else:
			self.FilterType = 2
			self.Truncation = 4
			self.Conflict = 1


#
# class MailConvert
#
# Converter class handling mail messages.

class MailConvert:
	
	def __init__(self,folderName):
		self.mailmessage = None
		self.folderName = folderName

	#
	# newNode
	#
	# Adds an XML node with the correct namespace
	
	def NewNode(self,parent,node,value):
		n=parent.newChild(None,node,value)
		n.setProp("xmlns","http://synce.org/formats/airsync_wm5/mail")
		return n

	#
	# DecodeHeader
	#
	# Decode a mail header and convert each bloody part of it
	
	def DecodeHeader(self,hdr,defaultval):
		
		if hdr!=None:
			headerparts = email.Header.decode_header(hdr)
			
			# Now we get to convert each little bit of it
			# and rebuild it
			
			rebuilt = ""
			
			for string,chrset in headerparts:
				if chrset !=None:
					code = chrset
				else:
					code = "cp1252" # std ASCII
					
				rebuilt+=string.decode(code)
			
			return rebuilt
		else:
			return defaultval

	#
	# DateToAirsync
	#
	# Convert an email message date string to an Airsync
	# date string
	
	def DateToAirsync(self,date):
		if date != None:
			date = email.Utils.parsedate_tz(date)
			if date:
				return time.strftime("%Y-%m-%dT%H:%M:%SZ", time.gmtime(email.Utils.mktime_tz(date)))
			else:
				return None
		else:
			return None

	#
	# MailToApplicationNode
	
	def MailToApplicationNode(self,itemID,mailmessage):
		
		msgnode=libxml2.newNode("ApplicationData")
				
		# We need to munge the mail as, typically, the data used in the sync does not
		# completely match the data in the headers.
		
		# The standard headers we should make sure we encode.
		
		hdrTo = self.DecodeHeader(mailmessage["To"],"<unspecified>")
		hdrSubject = self.DecodeHeader(mailmessage["Subject"],"<unspecified>")
		self.NewNode(msgnode,"To",hdrTo)
		self.NewNode(msgnode,"From",self.DecodeHeader(mailmessage["From"],"<unspecified>"))
		self.NewNode(msgnode,"Subject",hdrSubject)
		self.NewNode(msgnode,"DisplayTo",hdrTo)

		# Deal with Cc and Bcc only if we have them
		
		h=mailmessage["Cc"]
		if h!=None:
			self.NewNode(msgnode,"Cc",self.DecodeHeader(h,""))
		h=mailmessage["Bcc"]
		if h!=None:
			self.NewNode(msgnode,"Bcc",self.DecodeHeader(h,""))
		
		# If we have a Thread-Topic header, use it. Otherwise use the subject
		
		h=mailmessage["Thread-Topic"]
		if h!=None:
			n=self.DecodeHeader(h,"")
		else:
			n=hdrSubject
		self.NewNode(msgnode,"ThreadTopic",n)
		
		# Now deal with the timestamp.
				
		h=mailmessage["Date"]
		if h!=None:
			self.NewNode(msgnode,"DateReceived",self.DateToAirsync(h))
				
		##
		## TODO - 'Read' flag?
				
		# Check for a body and attachments. If we have no explicit body, then get this from the
		# message. We have to walk the MIME-compliant message to do this. Make sure we only grab
		# one body and add this last
	
		haveBody = False
		attidx = 0
		attNode = None
	
		for msgpart in mailmessage.walk():
			
			msgtype = msgpart.get_content_type()
			msgdata = msgpart.get_payload(None,True)
			
			# First the body-search code
			
			if not haveBody:
				
				if msgtype == "text/plain" or msgtype == "text/html":
				
					msgencoding = msgpart.get_content_charset()
				
					# these types are standard body types (decode if we can, if encoded 
					# according to the specified content charset
				
					if not msgencoding:
						msgencoding = "cp1252"	# sounds like a good default
				
					try:
						ucbodydata = msgdata.decode(msgencoding)
					except:
						ucbodydata = u"---- UNABLE TO DECODE DATA ----"
						
					haveBody = True
			
			# Now process attachments. Attachments will always have a filename so we
			# can distinguish from text blocks
			
			fn = msgpart.get_filename()
			if fn:
				if attNode == None:
					attNode = self.NewNode(msgnode,"Attachments",None)
					
				# we have an attachment. Get its details (but we don't actually
				# want the data here
				
				attsize = len(msgpart.get_payload(None,True))
				atttype = msgpart.get_content_type()
				attname = util.GenerateCombinedID(self.folderName,itemID) + "/" + fn + "/" + str(attidx) # close to Airsync

				thisattnode = self.NewNode(attNode,"Attachment",None)
				self.NewNode(thisattnode,"AttMethod","1")
				self.NewNode(thisattnode,"AttSize",str(attsize))
				self.NewNode(thisattnode,"DisplayName",fn)
				self.NewNode(thisattnode,"AttName",attname)
			attidx += 1

		# set the body node

		self.NewNode(msgnode,"Body",ucbodydata)
			
		# Some other AirSync stuff that we seem to need. The MessageClass is unchanged
		# (except for a delivery report) and we are sending the message out in Unicode.
		
		self.NewNode(msgnode,"MessageClass","IPM.Note")
		self.NewNode(msgnode,"InternetCPID","65001")

		
		return msgnode
	
#
# ParseAttachmentName
#
# Get the folder ID, the message ID, the attachment filename and the index from the attachment name

def ParseAttachmentName(attname):
	
	strs = attname.split("/")
	combinedID = strs[0]
	filename = strs[1]
	index = strs[2]
	folderID,itemID=util.SplitCombinedID(combinedID)
	return folderID,itemID,filename,index
	