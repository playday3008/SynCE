<?xml version="1.0" encoding="UTF-8"?>

<!--
    http://msdn.microsoft.com/en-us/library/microsoft.windowsmobile.pocketoutlook.contact_properties.aspx
    http://msdn.microsoft.com/en-us/library/ee160866.aspx
-->
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

            <xsl:apply-templates/>

        </AS:ApplicationData>
    </xsl:template>


    <xsl:template match="Name">
        <xsl:apply-templates/>
    </xsl:template>

    <xsl:template match="Name/FirstName">
        <C1:FirstName><xsl:value-of select="."/></C1:FirstName>
    </xsl:template>

    <xsl:template match="Name/LastName">
        <C1:LastName><xsl:value-of select="."/></C1:LastName>
    </xsl:template>

    <xsl:template match="Name/Additional">
        <C1:MiddleName><xsl:value-of select="."/></C1:MiddleName>
    </xsl:template>

    <xsl:template match="Name/Suffix">
        <C1:Suffix><xsl:value-of select="."/></C1:Suffix>
    </xsl:template>

    <xsl:template match="Name/Prefix">
        <C1:Title><xsl:value-of select="."/></C1:Title>
    </xsl:template>

    <xsl:template match="Nickname">
        <C2:NickName><xsl:value-of select="Content"/></C2:NickName>
    </xsl:template>

    <xsl:template match="Photo">
        <C1:Picture><xsl:value-of select="Content[position()=1]"/></C1:Picture>
    </xsl:template>

    <xsl:template match="Address">
        <!-- Type can be lowercase: http://tools.ietf.org/html/rfc2426#page-35 -->
        <xsl:variable name="type" select="convert:all_upper_case(string(Type))"/>
        <xsl:choose>
            <xsl:when test="$type = 'HOME' or $type = ''">
                <xsl:call-template name="address-elem">
                    <xsl:with-param name="prefix" select="'Home'"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="$type = 'WORK'">
                <xsl:call-template name="address-elem">
                    <xsl:with-param name="prefix" select="'Business'"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:when test="$type = 'OTHER'">
                <xsl:call-template name="address-elem">
                    <xsl:with-param name="prefix" select="'Other'"/>
                </xsl:call-template>
            </xsl:when>
            <xsl:otherwise>
                <xsl:message><xsl:value-of select="$type"/></xsl:message>
                <xsl:call-template name="address-elem">
                    <xsl:with-param name="prefix" select="''"/>
                </xsl:call-template>
            </xsl:otherwise>
        </xsl:choose>
    </xsl:template>

    <xsl:template name="address-elem">
        <xsl:param name="prefix"/>

        <xsl:element name="{concat('C1:',$prefix,'City')}">
            <xsl:value-of select="City"/>
        </xsl:element>
        <xsl:element name="{concat('C1:',$prefix,'Country')}">
            <xsl:value-of select="Country"/>
        </xsl:element>
        <xsl:element name="{concat('C1:',$prefix,'PostalCode')}">
            <xsl:value-of select="PostalCode"/>
        </xsl:element>
        <xsl:element name="{concat('C1:',$prefix,'State')}">
            <xsl:value-of select="Region"/>
        </xsl:element>
        <xsl:element name="{concat('C1:',$prefix,'Street')}">
            <xsl:value-of select="Street"/>
            <xsl:if test="count(ExtendedAddress) &gt; 0">
                <xsl:text>
</xsl:text>
                <xsl:value-of select="ExtendedAddress"/>
            </xsl:if>
            <xsl:if test="count(PostalBox) &gt; 0">
                <xsl:text>
PO Box </xsl:text>
                <xsl:value-of select="PostalBox"/>
            </xsl:if>
        </xsl:element>

    </xsl:template>

    <xsl:template match="Categories">
        <C1:Categories>
            <xsl:apply-templates/>
        </C1:Categories>
    </xsl:template>

    <xsl:template match="Category">
        <C1:Category><xsl:value-of select="."/></C1:Category>
    </xsl:template>

    <xsl:template match="Assistant">
        <C1:AssistantName><xsl:value-of select="Content"/></C1:AssistantName>
    </xsl:template>

    <xsl:template match="EMail">
        <xsl:variable name="position" select="count(preceding-sibling::EMail) + 1"/>
        <xsl:if test="$position &lt;= 3">
            <xsl:element name="{concat('C1:Email', $position, 'Address')}">
                <xsl:value-of select="Content"/>
            </xsl:element>
        </xsl:if>
    </xsl:template>

    <xsl:template match="IM-MSN">
        <xsl:variable name="position" select="count(preceding-sibling::IM-MSN) + 1"/>
        <xsl:choose>
            <xsl:when test="$position = 1">
                <C2:IMAddress>
                    <xsl:value-of select="Content"/>
                </C2:IMAddress>
            </xsl:when>
            <xsl:when test="$position &lt;= 3">
                <xsl:element name="{concat('C2:IMAddress', $position)}">
                    <xsl:value-of select="Content"/>
                </xsl:element>
            </xsl:when>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="Manager">
            <C2:ManagerName><xsl:value-of select="Content"/></C2:ManagerName>
    </xsl:template>

    <xsl:template match="Organization">
        <xsl:apply-templates/>
    </xsl:template>

    <xsl:template match="Organization/Name">
            <C1:CompanyName><xsl:value-of select="."/></C1:CompanyName>
    </xsl:template>

    <xsl:template match="Organization/Department">
            <C1:Department><xsl:value-of select="."/></C1:Department>
    </xsl:template>

    <xsl:template match="Organization/Unit">
            <C1:OfficeLocation><xsl:value-of select="."/></C1:OfficeLocation>
    </xsl:template>

    <xsl:template match="Spouse">
            <C1:Spouse><xsl:value-of select="Content"/></C1:Spouse>
    </xsl:template>

    <xsl:template match="Telephone">
        <!-- Type is case-insensitive -->
        <xsl:choose>
            <xsl:when test="convert:contact_has_type('HOME') and convert:contact_has_type('FAX')">
                <C1:HomeFaxNumber>
                    <xsl:value-of select="Content"/>
                </C1:HomeFaxNumber>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('HOME')">
                <xsl:choose>
                    <xsl:when test="convert:contact_position() = 2">
                        <C1:Home2PhoneNumber>
                            <xsl:value-of select="Content"/>
                        </C1:Home2PhoneNumber>
                    </xsl:when>
                    <xsl:otherwise>
                        <C1:HomePhoneNumber>
                            <xsl:value-of select="Content"/>
                        </C1:HomePhoneNumber>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('WORK') and convert:contact_has_type('FAX')">
                <C1:BusinessFaxNumber>
                    <xsl:value-of select="Content"/>
                </C1:BusinessFaxNumber>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('WORK')">
                <xsl:choose>
                    <xsl:when test="convert:contact_position() = 2">
                        <C1:Business2PhoneNumber>
                            <xsl:value-of select="Content"/>
                        </C1:Business2PhoneNumber>
                    </xsl:when>
                    <xsl:otherwise>
                        <C1:BusinessPhoneNumber>
                            <xsl:value-of select="Content"/>
                        </C1:BusinessPhoneNumber>
                    </xsl:otherwise>
                </xsl:choose>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('CELL')">
                <C1:MobilePhoneNumber>
                    <xsl:value-of select="Content"/>
                </C1:MobilePhoneNumber>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('CAR')">
                <C1:CarPhoneNumber>
                    <xsl:value-of select="Content"/>
                </C1:CarPhoneNumber>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('PAGER')">
                <C1:PagerNumber>
                    <xsl:value-of select="Content"/>
                </C1:PagerNumber>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('ASSISTANT')">
                <C1:AssistnamePhoneNumber>
                    <xsl:value-of select="Content"/>
                </C1:AssistnamePhoneNumber>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('RADIO')">
                <C1:RadioPhoneNumber>
                    <xsl:value-of select="Content"/>
                </C1:RadioPhoneNumber>
            </xsl:when>
            <xsl:when test="convert:contact_has_type('COMPANY')">
                <C1:CompanyMainPhone>
                    <xsl:value-of select="Content"/>
                </C1:CompanyMainPhone>
            </xsl:when>
        </xsl:choose>
    </xsl:template>

    <xsl:template match="Title">
        <C1:JobTitle><xsl:value-of select="Content"/></C1:JobTitle>
    </xsl:template>

    <xsl:template match="Url">
        <C1:WebPage><xsl:value-of select="Content"/></C1:WebPage>
    </xsl:template>

    <xsl:template match="Anniversary">
        <C1:Anniversary><xsl:value-of select="convert:contact_anniversary_to_airsync()"/></C1:Anniversary>
    </xsl:template>

    <xsl:template match="Birthday">
        <C1:Birthday><xsl:value-of select="convert:contact_birthday_to_airsync()"/></C1:Birthday>
    </xsl:template>

    <xsl:template match="Note">
        <C1:Body><xsl:value-of select="Content"/></C1:Body>
    </xsl:template>

    <xsl:template match="*">
    </xsl:template>

</xsl:transform>
