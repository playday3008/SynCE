<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               exclude-result-prefixes="convert"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               xmlns:C1="http://synce.org/formats/airsync_wm5/contacts"
               xmlns:C2="http://synce.org/formats/airsync_wm5/contacts2">

    <xsl:template match="/contact">
        <AS:ApplicationData>

            <xsl:if test="count(FileAs) &gt;= 1">
                <C1:FileAs><xsl:value-of select="FileAs/Content"/></C1:FileAs>
            </xsl:if>

            <xsl:if test="count(FileAs) = 0">
                <xsl:if test="count(FormattedName) &gt;= 1">
                    <C1:FileAs><xsl:value-of select="FormattedName/Content"/></C1:FileAs>
                </xsl:if>
            </xsl:if>

            <xsl:if test="count(FileAs) = 0">
                <xsl:if test="count(FormattedName) = 0">
                    <C1:FileAs><xsl:value-of select="normalize-space(concat(Name/Prefix, ' ', Name/FirstName, ' ', Name/Additional, ' ', Name/LastName, ' ', Name/Suffix))"/></C1:FileAs>
                </xsl:if>
            </xsl:if>

            <C1:FirstName><xsl:value-of select="Name/FirstName"/></C1:FirstName>
            <C1:LastName><xsl:value-of select="Name/LastName"/></C1:LastName>
            <C1:MiddleName><xsl:value-of select="Name/Additional"/></C1:MiddleName>
            <C1:Suffix><xsl:value-of select="Name/Suffix"/></C1:Suffix>
            <C1:Title><xsl:value-of select="Name/Prefix"/></C1:Title>

            <C2:NickName><xsl:value-of select="Nickname/Content"/></C2:NickName>

            <C1:Picture><xsl:value-of select="Photo/Content[position()=1]"/></C1:Picture>

            <xsl:for-each select="Address[Type='HOME'][position() = 1]">
                <C1:HomeCity><xsl:value-of select="City"/></C1:HomeCity>
                <C1:HomeCountry><xsl:value-of select="Country"/></C1:HomeCountry>
                <C1:HomePostalCode><xsl:value-of select="PostalCode"/></C1:HomePostalCode>
                <C1:HomeState><xsl:value-of select="Region"/></C1:HomeState>
                <C1:HomeStreet><xsl:value-of select="Street"/></C1:HomeStreet>
            </xsl:for-each>

            <xsl:if test="count(Address[Type='HOME']) = 0">
                <xsl:for-each select="Address[count(parent::node()/Type) = 0][position() = 1]">
                    <xsl:if test="1">
                        <C1:HomeCity><xsl:value-of select="City"/></C1:HomeCity>
                        <C1:HomeCountry><xsl:value-of select="Country"/></C1:HomeCountry>
                        <C1:HomePostalCode><xsl:value-of select="PostalCode"/></C1:HomePostalCode>
                        <C1:HomeState><xsl:value-of select="Region"/></C1:HomeState>
                        <C1:HomeStreet><xsl:value-of select="Street"/></C1:HomeStreet>
                    </xsl:if>
                </xsl:for-each>
            </xsl:if>

            <C1:BusinessCity><xsl:value-of select="Address[Type='WORK']/City"/></C1:BusinessCity>
            <C1:BusinessCountry><xsl:value-of select="Address[Type='WORK']/Country"/></C1:BusinessCountry>
            <C1:BusinessPostalCode><xsl:value-of select="Address[Type='WORK']/PostalCode"/></C1:BusinessPostalCode>
            <C1:BusinessState><xsl:value-of select="Address[Type='WORK']/Region"/></C1:BusinessState>
            <C1:BusinessStreet><xsl:value-of select="Address[Type='WORK']/Street"/></C1:BusinessStreet>

            <C1:OtherCity><xsl:value-of select="Address[Type='OTHER']/City"/></C1:OtherCity>
            <C1:OtherCountry><xsl:value-of select="Address[Type='OTHER']/Country"/></C1:OtherCountry>
            <C1:OtherPostalCode><xsl:value-of select="Address[Type='OTHER']/PostalCode"/></C1:OtherPostalCode>
            <C1:OtherState><xsl:value-of select="Address[Type='OTHER']/Region"/></C1:OtherState>
            <C1:OtherStreet><xsl:value-of select="Address[Type='OTHER']/Street"/></C1:OtherStreet>

            <C1:Categories>
                <xsl:for-each select="Categories">
                    <xsl:for-each select="Category">
                        <C1:Category><xsl:value-of select="."/></C1:Category>
                    </xsl:for-each>
                </xsl:for-each>
            </C1:Categories>

            <C1:AssistantName><xsl:value-of select="Assistant/Content"/></C1:AssistantName>

            <xsl:for-each select="EMail/Content">
                <xsl:if test="position() &lt;= 3">
                    <xsl:element name="{concat('C1:Email', position(), 'Address')}">
                        <xsl:value-of select="."/>
                    </xsl:element>
                </xsl:if>
            </xsl:for-each>

            <xsl:for-each select="IM-MSN/Content">
                <xsl:choose>
                    <xsl:when test="position() = 1">
                        <C2:IMAddress>
                            <xsl:value-of select="."/>
                        </C2:IMAddress>
                    </xsl:when>
                    <xsl:when test="position() &lt;= 3">
                        <xsl:element name="{concat('C2:IMAddress', position())}">
                            <xsl:value-of select="."/>
                        </xsl:element>
                    </xsl:when>
                </xsl:choose>
            </xsl:for-each>

            <C2:ManagerName><xsl:value-of select="Manager/Content"/></C2:ManagerName>

            <C1:CompanyName><xsl:value-of select="Organization/Name"/></C1:CompanyName>
            <C1:Department><xsl:value-of select="Organization/Department"/></C1:Department>
            <C1:OfficeLocation><xsl:value-of select="Organization/Unit"/></C1:OfficeLocation>

            <C1:Spouse><xsl:value-of select="Spouse/Content"/></C1:Spouse>

            <C1:HomeFaxNumber><xsl:value-of select="Telephone[Type='HOME'][Type='FAX']/Content"/></C1:HomeFaxNumber>
            <C1:BusinessFaxNumber><xsl:value-of select="Telephone[Type='WORK'][Type='FAX']/Content"/></C1:BusinessFaxNumber>

            <xsl:for-each select="Telephone[Type='HOME']/Content">
                <xsl:if test="count(parent::node()[Type='FAX']) = 0">
                    <xsl:choose>
                        <xsl:when test="position() = 1">
                            <C1:HomePhoneNumber>
                                <xsl:value-of select="."/>
                            </C1:HomePhoneNumber>
                        </xsl:when>
                        <xsl:when test="position() &lt;= 2">
                            <xsl:element name="{concat('C1:Home', position(), 'PhoneNumber')}">
                                <xsl:value-of select="."/>
                            </xsl:element>
                        </xsl:when>
                    </xsl:choose>
                </xsl:if>
            </xsl:for-each>

            <xsl:for-each select="Telephone[Type='WORK']/Content">
                <xsl:if test="count(parent::node()[Type='FAX']) = 0">
                    <xsl:choose>
                        <xsl:when test="position() = 1">
                            <C1:BusinessPhoneNumber>
                                <xsl:value-of select="."/>
                            </C1:BusinessPhoneNumber>
                        </xsl:when>
                        <xsl:when test="position() &lt;= 2">
                            <xsl:element name="{concat('C1:Business', position(), 'PhoneNumber')}">
                                <xsl:value-of select="."/>
                            </xsl:element>
                        </xsl:when>
                    </xsl:choose>
                </xsl:if>
            </xsl:for-each>

            <C1:CarPhoneNumber><xsl:value-of select="Telephone[Type='CAR']/Content"/></C1:CarPhoneNumber>
            <C1:MobilePhoneNumber><xsl:value-of select="Telephone[Type='CELL']/Content"/></C1:MobilePhoneNumber>
            <C1:PagerNumber><xsl:value-of select="Telephone[Type='PAGER']/Content"/></C1:PagerNumber>
            <C1:AssistnamePhoneNumber><xsl:value-of select="Telephone[Type='Assistant']/Content"/></C1:AssistnamePhoneNumber>
            <C2:CompanyMainPhone><xsl:value-of select="Telephone[Type='Company']/Content"/></C2:CompanyMainPhone>
            <C1:RadioPhoneNumber><xsl:value-of select="Telephone[Type='Radio']/Content"/></C1:RadioPhoneNumber>

            <C1:JobTitle><xsl:value-of select="Title/Content"/></C1:JobTitle>

            <C1:WebPage><xsl:value-of select="Url/Content"/></C1:WebPage>

            <xsl:for-each select="Anniversary[position() = 1]">
                <C1:Anniversary><xsl:value-of select="convert:contact_anniversary_to_airsync()"/></C1:Anniversary>
            </xsl:for-each>
            <xsl:for-each select="Birthday[position() = 1]">
                <C1:Birthday><xsl:value-of select="convert:contact_birthday_to_airsync()"/></C1:Birthday>
            </xsl:for-each>
	    <xsl:for-each select="Note/Content[position() = 1]">
		<Rtf><xsl:value-of select="convert:all_description_to_airsync()"/></Rtf>
	    </xsl:for-each>


        </AS:ApplicationData>
    </xsl:template>

</xsl:transform>
