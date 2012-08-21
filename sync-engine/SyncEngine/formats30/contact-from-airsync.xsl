<?xml version="1.0" encoding="UTF-8"?>

<!-- Updated for Opensync 0.3x -->

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:common="http://synce.org/common"
               xmlns:contact="http://synce.org/contact"
               xmlns:C1="http://synce.org/formats/airsync_wm5/contacts"
               xmlns:C2="http://synce.org/formats/airsync_wm5/contacts2"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               exclude-result-prefixes="common contact C1 C2 AS">

    <xsl:template match="ApplicationData | AS:ApplicationData">
        <contact>

            <!-- Opensync 0.3x - FileAs and FormattedName is the same -->

            <xsl:for-each select="C1:FileAs[position()=1]">
                <FormattedName><Content><xsl:value-of select="."/></Content></FormattedName>
                <FileAs><Content><xsl:value-of select="."/></Content></FileAs>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Name', 'Nickname' and 'Photo' is the same,
                 but 'Photo' may need its attributes handling -->

            <xsl:for-each select="(C1:FirstName | C1:LastName | C1:MiddleName | C1:Suffix | C1:Title)[position()=1]">
                <Name>
                    <xsl:for-each select="../C1:LastName">
                        <LastName><xsl:value-of select="."/></LastName>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:FirstName">
                        <FirstName><xsl:value-of select="."/></FirstName>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:MiddleName">
                        <Additional><xsl:value-of select="."/></Additional>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:Suffix">
                        <Suffix><xsl:value-of select="."/></Suffix>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:Title">
                        <Prefix><xsl:value-of select="."/></Prefix>
                    </xsl:for-each>
                </Name>
            </xsl:for-each>

            <xsl:for-each select="C2:Nickname[position()=1]">
                <Nickname><Content><xsl:value-of select="."/></Content></Nickname>
            </xsl:for-each>

            <xsl:for-each select="C1:Picture[position()=1]">
                <Photo><Content><xsl:value-of select="."/></Content></Photo>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Address' uses an attribute for location and 'City' becomes
                 'Locality' -->

            <xsl:for-each select="(C1:HomeCity | C1:HomeCountry | C1:HomePostalCode | C1:HomeState | C1:HomeStreet)[position() = 1]">
                <Address Location="Home">
                    <xsl:for-each select="../C1:HomeCity">
                        <Locality><xsl:value-of select="."/></Locality>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:HomeCountry">
                        <Country><xsl:value-of select="."/></Country>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:HomePostalCode">
                        <PostalCode><xsl:value-of select="."/></PostalCode>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:HomeState">
                        <Region><xsl:value-of select="."/></Region>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:HomeStreet">
                        <Street><xsl:value-of select="."/></Street>
                    </xsl:for-each>
                </Address>
            </xsl:for-each>

            <xsl:for-each select="(C1:BusinessCity | C1:BusinessCountry | C1:BusinessPostalCode | C1:BusinessState | C1:BusinessStreet)[position() = 1]">
                <Address Location="Work">
                    <Locality><xsl:value-of select="../C1:BusinessCity"/></Locality>
                    <Country><xsl:value-of select="../C1:BusinessCountry"/></Country>
                    <PostalCode><xsl:value-of select="../C1:BusinessPostalCode"/></PostalCode>
                    <Region><xsl:value-of select="../C1:BusinessState"/></Region>
                    <Street><xsl:value-of select="../C1:BusinessStreet"/></Street>
                </Address>
            </xsl:for-each>

            <xsl:for-each select="(C1:OtherCity | C1:OtherCountry | C1:OtherPostalCode | C1:OtherState | C1:OtherStreet)[position() = 1]">
                <Address Location="Other">
                    <Locality><xsl:value-of select="../C1:OtherCity"/></Locality>
                    <Country><xsl:value-of select="../C1:OtherCountry"/></Country>
                    <PostalCode><xsl:value-of select="../C1:OtherPostalCode"/></PostalCode>
                    <Region><xsl:value-of select="../C1:OtherState"/></Region>
                    <Street><xsl:value-of select="../C1:OtherStreet"/></Street>
                </Address>
            </xsl:for-each>

            <xsl:if test="count(C1:Categories) &gt; 0">
                <Categories>
                    <xsl:for-each select="C1:Categories">
                        <xsl:for-each select="C1:Category">
                            <Category><xsl:value-of select="."/></Category>
                        </xsl:for-each>
                    </xsl:for-each>
                </Categories>
            </xsl:if>

            <!-- Opensync 0.3x - 'Assistant' is the same -->

            <xsl:for-each select="C1:AssistantName[position()=1]">
                <Assistant><Content><xsl:value-of select="C1:AssistantName"/></Content></Assistant>
            </xsl:for-each>

            <xsl:for-each select="*[starts-with(local-name(), 'Email')]">
                <EMail><Content><xsl:value-of select="."/></Content></EMail>
            </xsl:for-each>

            <xsl:for-each select="*[starts-with(local-name(), 'IMAddress')]">
                <IM-MSN Location="Home"><Content><xsl:value-of select="."/></Content></IM-MSN>
            </xsl:for-each>

            <xsl:for-each select="C2:ManagerName[position()=1]">
                <Manager><Content><xsl:value-of select="."/></Content></Manager>
            </xsl:for-each>

            <xsl:for-each select="(C1:CompanyName | C1:Department | C1:OfficeLocation)[position() = 1]">
                <Organization>
                    <xsl:for-each select="../C1:CompanyName">
                        <Name><xsl:value-of select="."/></Name>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:Department">
                        <Department><xsl:value-of select="."/></Department>
                    </xsl:for-each>
                    <xsl:for-each select="../C1:OfficeLocation">
                        <Unit><xsl:value-of select="."/></Unit>
                    </xsl:for-each>
                </Organization>
            </xsl:for-each>

            <xsl:for-each select="C1:Spouse[position()=1]">
                <Spouse><Content><xsl:value-of select="."/></Content></Spouse>
            </xsl:for-each>

            <!-- Opensync 0.3x - 'Telephone' is different -->

            <xsl:for-each select="C1:HomeFaxNumber[position() = 1]">
                <Telephone Type="Fax" Location="Home">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:BusinessFaxNumber[position() = 1]">
                <Telephone Type="Fax" Location="Work">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:HomePhoneNumber[position() = 1]">
                <Telephone Type="Voice" Location="Home">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:Home2PhoneNumber[position() = 1]">
                <Telephone Type="Voice" Location="Home">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:BusinessPhoneNumber[position() = 1]">
                <Telephone Type="Voice" Location="Work">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:Business2PhoneNumber[position() = 1]">
                <Telephone Type="Voice" Location="Work">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:CarPhoneNumber[position() = 1]">
                <Telephone Type="Voice" Location="Car">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:MobilePhoneNumber[position() = 1]">
                <Telephone Type="Cellular">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:PagerNumber[position() = 1]">
                <Telephone Type="Pager">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:AssistnamePhoneNumber[position() = 1]">
                <Telephone Type="Assistant">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C2:CompanyMainPhone[position() = 1]">
                <Telephone Type="Company">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>
            <xsl:for-each select="C1:RadioPhoneNumber[position() = 1]">
                <Telephone Type="Radio">
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <!-- Opensync 0.3x - The elements below are all the same -->

            <xsl:for-each select="C1:JobTitle[position()=1]">
                <Title><Content><xsl:value-of select="."/></Content></Title>
            </xsl:for-each>

            <xsl:for-each select="C1:WebPage[position()=1]">
                <Url><Content><xsl:value-of select="."/></Content></Url>
            </xsl:for-each>

            <xsl:for-each select="C1:Anniversary[position() = 1]">
                <Anniversary><xsl:value-of select="contact:AnniversaryFromAirsync()"/></Anniversary>
            </xsl:for-each>

            <xsl:for-each select="C1:Birthday[position() = 1]">
                <Birthday><xsl:value-of select="contact:BirthdayFromAirsync()"/></Birthday>
            </xsl:for-each>

            <xsl:for-each select="C1:Rtf">
                <Note><Content><xsl:value-of select="common:OSTextFromAirsyncRTF()"/></Content></Note>
            </xsl:for-each>

        </contact>
    </xsl:template>
</xsl:transform>
