<?xml version="1.0" encoding="UTF-8"?>

<!-- Updated for Opensync 0.3x -->

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:common="http://synce.org/common"
               xmlns:contact="http://synce.org/contact"
               exclude-result-prefixes="common contact"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               xmlns:C1="http://synce.org/formats/airsync_wm5/contacts"
               xmlns:C2="http://synce.org/formats/airsync_wm5/contacts2">

    <xsl:template match="/contact">
        <AS:ApplicationData>

            <!-- Opensync 0.3x - FileAs appears to have the same semantics, as
                 does FormattedName -->

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

            <!-- Opensync 0.3x - 'Name' appears to be the same compound type -->

            <xsl:for-each select="Name[position()=1]">
                <xsl:for-each select="FirstName[position()=1]">
                    <C1:FirstName><xsl:value-of select="."/></C1:FirstName>
                </xsl:for-each>
                <xsl:for-each select="LastName[position()=1]">
                    <C1:LastName><xsl:value-of select="."/></C1:LastName>
                </xsl:for-each>
                <xsl:for-each select="Additional[position()=1]">
                    <C1:MiddleName><xsl:value-of select="."/></C1:MiddleName>
                </xsl:for-each>
                <xsl:for-each select="Suffix[position()=1]">
                    <C1:Suffix><xsl:value-of select="."/></C1:Suffix>
                </xsl:for-each>
                <xsl:for-each select="Prefix[position()=1]">
                    <C1:Title><xsl:value-of select="."/></C1:Title>
                </xsl:for-each>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Nickname' appears unchanged -->

            <xsl:for-each select="Nickname[position()=1]">
                <C2:NickName><xsl:value-of select="Content"/></C2:NickName>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Picture' may need attributes showing file type. They
                 are of no use when going _to_ AirSync though -->

            <xsl:for-each select="Photo[position()=1]">
                <C1:Picture><xsl:value-of select="Content[position()=1]"/></C1:Picture>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Address' location is now stored in an attribute
                 'Location'. We capture the first entry with or without an attribute.
                 'City' becomes 'Locality' -->

            <xsl:for-each select="Address">
                <xsl:if test="position()=1">
                    <xsl:if test="@Location = 'Home' or not(@Location)">
                        <C1:HomeCity><xsl:value-of select="Locality"/></C1:HomeCity>
                        <C1:HomeCountry><xsl:value-of select="Country"/></C1:HomeCountry>
                        <C1:HomePostalCode><xsl:value-of select="PostalCode"/></C1:HomePostalCode>
                        <C1:HomeState><xsl:value-of select="Region"/></C1:HomeState>
                        <C1:HomeStreet><xsl:value-of select="Street"/></C1:HomeStreet>
                    </xsl:if>
                </xsl:if>
                <xsl:if test="@Location = 'Work'">
                    <C1:BusinessCity><xsl:value-of select="Locality"/></C1:BusinessCity>
                    <C1:BusinessCountry><xsl:value-of select="Country"/></C1:BusinessCountry>
                    <C1:BusinessPostalCode><xsl:value-of select="PostalCode"/></C1:BusinessPostalCode>
                    <C1:BusinessState><xsl:value-of select="Region"/></C1:BusinessState>
                    <C1:BusinessStreet><xsl:value-of select="/Street"/></C1:BusinessStreet>
                </xsl:if>
                <xsl:if test="@Location = 'Other'">
                    <C1:OtherCity><xsl:value-of select="Locality"/></C1:OtherCity>
                    <C1:OtherCountry><xsl:value-of select="Country"/></C1:OtherCountry>
                    <C1:OtherPostalCode><xsl:value-of select="PostalCode"/></C1:OtherPostalCode>
                    <C1:OtherState><xsl:value-of select="Region"/></C1:OtherState>
                    <C1:OtherStreet><xsl:value-of select="Street"/></C1:OtherStreet>
                </xsl:if>
            </xsl:for-each>


            <!-- Opensync 0.3x - 'Categories' appears unchanged -->

            <xsl:if test="count(Categories) &gt; 0">
                <C1:Categories>
                    <xsl:for-each select="Categories">
                        <xsl:for-each select="Category">
                            <C1:Category><xsl:value-of select="."/></C1:Category>
                        </xsl:for-each>
                    </xsl:for-each>
                </C1:Categories>
            </xsl:if>

            <!-- Opensync 0.3x - Assistant appears unchanged -->

            <xsl:for-each select="Assistant[position()=1]">
                <C1:AssistantName><xsl:value-of select="./Content"/></C1:AssistantName>
            </xsl:for-each>

            <!-- Opensync 0.3x - Email is OK, but may need to handle attributes
                 especially 'Location' -->

            <xsl:for-each select="EMail/Content">
                <xsl:if test="position() &lt;= 3">
                    <xsl:element name="{concat('C1:Email', position(), 'Address')}">
                        <xsl:value-of select="."/>
                    </xsl:element>
                </xsl:if>
            </xsl:for-each>

            <!-- Opensync 0.3x - IM-MSN content the same, but may need to handle attributes -->

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

            <!-- Opensync 0.3x - 'Manager', 'Organization/CompanyName', 
                 'Organization/Department' and 'Organization/Unit' are the same,
                 'Spouse' is the same -->

            <xsl:for-each select="Manager[position()=1]">
                <C2:ManagerName><xsl:value-of select="Content"/></C2:ManagerName>
            </xsl:for-each>
            <xsl:for-each select="Organization[position()=1]">
                <xsl:for-each select="Name[position()=1]">
                    <C1:CompanyName><xsl:value-of select="."/></C1:CompanyName>
                </xsl:for-each>
                <xsl:for-each select="Department[position()=1]">
                    <C1:Department><xsl:value-of select="."/></C1:Department>
                </xsl:for-each>
                <xsl:for-each select="Unit[position()=1]">
                    <C1:OfficeLocation><xsl:value-of select="."/></C1:OfficeLocation>
                </xsl:for-each>
            </xsl:for-each>

            <xsl:for-each select="Spouse[position()=1]">
                <C1:Spouse><xsl:value-of select="Content"/></C1:Spouse>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Telephone' has changed, need to handle attributes -->

            <xsl:for-each select="Telephone">
                 <xsl:choose>
                     <xsl:when test="@Location='Home' or not(@Location)">
                         <xsl:choose>
                             <xsl:when test="position()=1 and (not(@Type) or @Type='Voice')">
                                 <C1:HomePhoneNumber><xsl:value-of select="Content"/></C1:HomePhoneNumber>
                             </xsl:when>
                             <xsl:when test="(position() &lt;= 2 and position() &gt; 1) and (not(@Type) or @Type='Voice')">
                                 <xsl:element name="{concat('C1:Home', position(), 'PhoneNumber')}"><xsl:value-of select="."/></xsl:element>
                             </xsl:when>
                             <xsl:otherwise>
                                 <xsl:if test="@Type='Fax'"><C1:HomeFaxNumber><xsl:value-of select="Content"/></C1:HomeFaxNumber></xsl:if>
                             </xsl:otherwise>
                         </xsl:choose>
                     </xsl:when>
                     <xsl:when test="@Location = 'Work'">
                         <xsl:choose>
                             <xsl:when test="(position() = 1) and (not(@Type) or @Type='Voice')">
                                 <C1:BusinessPhoneNumber><xsl:value-of select="Content"/></C1:BusinessPhoneNumber>
                             </xsl:when>
                             <xsl:when test="(position() &lt;= 2 and position() &gt; 1) and (not(@Type) or @Type='Voice')">
                                 <xsl:element name="{concat('C1:Business', position(), 'PhoneNumber')}"><xsl:value-of select="Content"/></xsl:element>
                             </xsl:when>
                             <xsl:otherwise>
                                 <xsl:if test="@Type='Fax'"><C1:BusinessFaxNumber><xsl:value-of select="Content"/></C1:BusinessFaxNumber></xsl:if>
                             </xsl:otherwise>
                         </xsl:choose>
                     </xsl:when>
                     <xsl:when test="not(@Location)">
                         <C1:MobilePhoneNumber><xsl:value-of select="current()[@Type='Cellular']/Content"/></C1:MobilePhoneNumber>
                         <C1:CarPhoneNumber><xsl:value-of select="current()[@Type='Car']/Content"/></C1:CarPhoneNumber>
                         <C1:PagerNumber><xsl:value-of select="current()[@Type='Pager']/Content"/></C1:PagerNumber>
                         <C1:AssistnamePhoneNumber><xsl:value-of select="current()[@Type='Assistant']/Content"/></C1:AssistnamePhoneNumber>
                         <C2:CompanyMainPhone><xsl:value-of select="current()[@Type='Company']/Content"/></C2:CompanyMainPhone>
                         <C1:RadioPhoneNumber><xsl:value-of select="current()[@Type='Radio']/Content"/></C1:RadioPhoneNumber>
                     </xsl:when>
                 </xsl:choose>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Title' is the same -->

            <xsl:for-each select="Title[position()=0]">
                <C1:JobTitle><xsl:value-of select="Content"/></C1:JobTitle>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Url' is the same -->

            <xsl:for-each select="Url[position()=0]">
                <C1:WebPage><xsl:value-of select="Content"/></C1:WebPage>
            </xsl:for-each>

            <!-- Opensync 0.3x - Anniversary and Birthday are handled the same way -->

            <xsl:for-each select="Anniversary[position() = 1]">
                <C1:Anniversary><xsl:value-of select="contact:AnniversaryToAirsync()"/></C1:Anniversary>
            </xsl:for-each>

            <xsl:for-each select="Birthday[position() = 1]">
                <C1:Birthday><xsl:value-of select="contact:BirthdayToAirsync()"/></C1:Birthday>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Note' is handled the same way -->

            <xsl:for-each select="Note/Content[position() = 1]">
                <Rtf><xsl:value-of select="common:OSTextToAirsyncRTF()"/></Rtf>
            </xsl:for-each>

        </AS:ApplicationData>
    </xsl:template>
</xsl:transform>
