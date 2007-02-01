<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:C1="POOMCONTACTS:"
               xmlns:C2="POOMCONTACTS2:"
               exclude-result-prefixes="convert C1 C2">

    <xsl:template match="ApplicationData">
        <contact>
            <FileAs><Content><xsl:value-of select="C1:FileAs"/></Content></FileAs>

            <FormattedName><Content><xsl:value-of select="C1:FileAs"/></Content></FormattedName>

            <Name>
                <FirstName><xsl:value-of select="C1:FirstName"/></FirstName>
                <LastName><xsl:value-of select="C1:LastName"/></LastName>
                <Additional><xsl:value-of select="C1:MiddleName"/></Additional>
                <Suffix><xsl:value-of select="C1:Suffix"/></Suffix>
                <Prefix><xsl:value-of select="C1:Title"/></Prefix>
            </Name>

            <Nickname><Content><xsl:value-of select="C2:NickName"/></Content></Nickname>

            <Photo><xsl:value-of select="C1:Picture"/></Photo>

            <xsl:for-each select="(C1:HomeCity | C1:HomeCountry | C1:HomePostalCode | C1:HomeState | C1:HomeStreet)[position() = 1]">
                <Address>
                    <Type>HOME</Type>
                    <City><xsl:value-of select="../C1:HomeCity"/></City>
                    <Country><xsl:value-of select="../C1:HomeCountry"/></Country>
                    <PostalCode><xsl:value-of select="../C1:HomePostalCode"/></PostalCode>
                    <Region><xsl:value-of select="../C1:HomeState"/></Region>
                    <Street><xsl:value-of select="../C1:HomeStreet"/></Street>
                </Address>
            </xsl:for-each>

            <xsl:for-each select="(C1:BusinessCity | C1:BusinessCountry | C1:BusinessPostalCode | C1:BusinessState | C1:BusinessStreet)[position() = 1]">
                <Address>
                    <Type>WORK</Type>
                    <City><xsl:value-of select="../C1:BusinessCity"/></City>
                    <Country><xsl:value-of select="../C1:BusinessCountry"/></Country>
                    <PostalCode><xsl:value-of select="../C1:BusinessPostalCode"/></PostalCode>
                    <Region><xsl:value-of select="../C1:BusinessState"/></Region>
                    <Street><xsl:value-of select="../C1:BusinessStreet"/></Street>
                </Address>
            </xsl:for-each>

            <xsl:for-each select="(C1:OtherCity | C1:OtherCountry | C1:OtherPostalCode | C1:OtherState | C1:OtherStreet)[position() = 1]">
                <Address>
                    <Type>OTHER</Type>
                    <City><xsl:value-of select="../C1:OtherCity"/></City>
                    <Country><xsl:value-of select="../C1:OtherCountry"/></Country>
                    <PostalCode><xsl:value-of select="../C1:OtherPostalCode"/></PostalCode>
                    <Region><xsl:value-of select="../C1:OtherState"/></Region>
                    <Street><xsl:value-of select="../C1:OtherStreet"/></Street>
                </Address>
            </xsl:for-each>

            <Categories>
                <xsl:for-each select="C1:Categories">
                    <xsl:for-each select="C1:Category">
                        <Category><xsl:value-of select="."/></Category>
                    </xsl:for-each>
                </xsl:for-each>
            </Categories>

            <Assistant><Content><xsl:value-of select="C1:AssistantName"/></Content></Assistant>

            <xsl:for-each select="*[starts-with(local-name(), 'Email')]">
                <EMail><Content><xsl:value-of select="."/></Content></EMail>
            </xsl:for-each>

            <xsl:for-each select="*[starts-with(local-name(), 'IMAddress')]">
                <IM-MSN><Content><xsl:value-of select="."/></Content></IM-MSN>
            </xsl:for-each>

            <Manager><Content><xsl:value-of select="C2:ManagerName"/></Content></Manager>

            <Organization>
                <Name><xsl:value-of select="C1:CompanyName"/></Name>
                <Department><xsl:value-of select="C1:Department"/></Department>
                <Unit><xsl:value-of select="C1:OfficeLocation"/></Unit>
            </Organization>

            <Spouse><Content><xsl:value-of select="C1:Spouse"/></Content></Spouse>

            <xsl:for-each select="C1:HomeFaxNumber[position() = 1]">
                <Telephone>
                    <Type>HOME</Type>
                    <Type>FAX</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:BusinessFaxNumber[position() = 1]">
                <Telephone>
                    <Type>WORK</Type>
                    <Type>FAX</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:HomePhoneNumber[position() = 1]">
                <Telephone>
                    <Type>HOME</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:Home2PhoneNumber[position() = 1]">
                <Telephone>
                    <Type>HOME</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:BusinessPhoneNumber[position() = 1]">
                <Telephone>
                    <Type>WORK</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:Business2PhoneNumber[position() = 1]">
                <Telephone>
                    <Type>WORK</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:CarPhoneNumber[position() = 1]">
                <Telephone>
                    <Type>CAR</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:MobilePhoneNumber[position() = 1]">
                <Telephone>
                    <Type>CELL</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:PagerNumber[position() = 1]">
                <Telephone>
                    <Type>PAGER</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:AssistnamePhoneNumber[position() = 1]">
                <Telephone>
                    <Type>Assistant</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C2:CompanyMainPhone[position() = 1]">
                <Telephone>
                    <Type>Company</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <xsl:for-each select="C1:RadioPhoneNumber[position() = 1]">
                <Telephone>
                    <Type>Radio</Type>
                    <Content><xsl:value-of select="."/></Content>
                </Telephone>
            </xsl:for-each>

            <Title><Content><xsl:value-of select="C1:JobTitle"/></Content></Title>

            <Url><Content><xsl:value-of select="C1:WebPage"/></Content></Url>

            <xsl:for-each select="C1:Anniversary[position() = 1]">
                <Anniversary><xsl:value-of select="convert:contact_anniversary_from_airsync()"/></Anniversary>
            </xsl:for-each>
            <xsl:for-each select="C1:Birthday[position() = 1]">
                <Birthday><xsl:value-of select="convert:contact_birthday_from_airsync()"/></Birthday>
            </xsl:for-each>

        </contact>
    </xsl:template>

</xsl:transform>
