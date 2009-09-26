<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:C1="http://synce.org/formats/airsync_wm5/contacts"
               xmlns:C2="http://synce.org/formats/airsync_wm5/contacts2"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               exclude-result-prefixes="convert C1 C2">

    <xsl:template match="ApplicationData | AS:ApplicationData">
        <contact>

            <Name>
                <xsl:apply-templates select="C1:FirstName | C1:LastName | C1:MiddleName | C1:Suffix | C1:Title" mode="name"/>
            </Name>

            <xsl:if test="(C1:HomeCity | C1:HomeCountry | C1:HomePostalCode | C1:HomeState | C1:HomeStreet)">
                <Address>
                    <Type>HOME</Type>
                    <xsl:apply-templates select="C1:HomeCity | C1:HomeCountry | C1:HomePostalCode | C1:HomeState | C1:HomeStreet" mode="addr"/>
                </Address>
            </xsl:if>

            <xsl:if test="(C1:BusinessCity | C1:BusinessCountry | C1:BusinessPostalCode | C1:BusinessState | C1:BusinessStreet)">
                <Address>
                    <Type>WORK</Type>
                    <xsl:apply-templates select="C1:BusinessCity | C1:BusinessCountry | C1:BusinessPostalCode | C1:BusinessState | C1:BusinessStreet" mode="addr"/>
                </Address>
            </xsl:if>

            <xsl:if test="(C1:OtherCity | C1:OtherCountry | C1:OtherPostalCode | C1:OtherState | C1:OtherStreet)">
                <Address>
                    <Type>OTHER</Type>
                    <xsl:apply-templates select="C1:OtherCity | C1:OtherCountry | C1:OtherPostalCode | C1:OtherState | C1:OtherStreet" mode="addr"/>
                </Address>
            </xsl:if>

            <xsl:if test="(C1:CompanyName | C1:Department | C1:OfficeLocation)">
                <Organization>
                    <xsl:apply-templates select="C1:CompanyName" mode="org"/>
                    <xsl:apply-templates select="C1:Department" mode="org"/>
                    <xsl:apply-templates select="C1:OfficeLocation" mode="org"/>
                </Organization>
            </xsl:if>

            <xsl:apply-templates/>

        </contact>
    </xsl:template>


    <xsl:template match="C1:FileAs">
        <FileAs><Content><xsl:value-of select="."/></Content></FileAs>
        <FormattedName><Content><xsl:value-of select="."/></Content></FormattedName>
    </xsl:template>

    <xsl:template match="C1:FirstName" mode="name">
        <FirstName><xsl:value-of select="."/></FirstName>
    </xsl:template>
    <xsl:template match="C1:LastName" mode="name">
        <LastName><xsl:value-of select="."/></LastName>
    </xsl:template>
    <xsl:template match="C1:MiddleName" mode="name">
        <Additional><xsl:value-of select="."/></Additional>
    </xsl:template>
    <xsl:template match="C1:Suffix" mode="name">
        <Suffix><xsl:value-of select="."/></Suffix>
    </xsl:template>
    <xsl:template match="C1:Title" mode="name">
        <Prefix><xsl:value-of select="."/></Prefix>
    </xsl:template>

    <xsl:template match="C2:NickName">
        <Nickname><Content><xsl:value-of select="."/></Content></Nickname>
    </xsl:template>

    <xsl:template match="C1:Picture">
        <Photo><Content><xsl:value-of select="."/></Content><Encoding>B</Encoding></Photo>
    </xsl:template>

    <xsl:template match="C1:HomeCity | C1:BusinessCity | C1:OtherCity" mode="addr">
        <City><xsl:value-of select="."/></City>
    </xsl:template>
    <xsl:template match="C1:HomeCountry | C1:BusinessCountry | C1:OtherCountry" mode="addr">
        <Country><xsl:value-of select="."/></Country>
    </xsl:template>
    <xsl:template match="C1:HomePostalCode | C1:BusinessPostalCode | C1:OtherPostalCode" mode="addr">
        <PostalCode><xsl:value-of select="."/></PostalCode>
    </xsl:template>
    <xsl:template match="C1:HomeState | C1:BusinessState | C1:OtherState" mode="addr">
        <Region><xsl:value-of select="."/></Region>
    </xsl:template>
    <xsl:template match="C1:HomeStreet | C1:BusinessStreet | C1:OtherStreet" mode="addr">
        <Street><xsl:value-of select="."/></Street>
    </xsl:template>

    <xsl:template match="C1:Categories">
        <Categories>
            <xsl:apply-templates/>
        </Categories>
    </xsl:template>
    <xsl:template match="C1:Category">
        <Category><xsl:value-of select="."/></Category>
    </xsl:template>

    <xsl:template match="C1:AssistantName">
        <Assistant><Content><xsl:value-of select="."/></Content></Assistant>
    </xsl:template>

    <xsl:template match="C1:Email1Address | C1:Email2Address | C1:Email3Address">
        <EMail><Content><xsl:value-of select="."/></Content></EMail>
    </xsl:template>

    <xsl:template match="C2:IMAddress | C2:IMAddress2 | C2:IMAddress3">
        <IM-MSN><Content><xsl:value-of select="."/></Content></IM-MSN>
    </xsl:template>

    <xsl:template match="C2:ManagerName">
        <Manager><Content><xsl:value-of select="."/></Content></Manager>
    </xsl:template>

    <xsl:template match="C1:CompanyName" mode="org">
        <Name><xsl:value-of select="."/></Name>
    </xsl:template>
    <xsl:template match="C1:Department" mode="org">
        <Department><xsl:value-of select="."/></Department>
    </xsl:template>
    <xsl:template match="C1:OfficeLocation" mode="org">
        <Unit><xsl:value-of select="."/></Unit>
    </xsl:template>

    <xsl:template match="C1:Spouse">
        <Spouse><Content><xsl:value-of select="."/></Content></Spouse>
    </xsl:template>

    <xsl:template name="telephone">
        <xsl:param name="location"/>
        <xsl:param name="nature"/>
        <Telephone>
            <Type><xsl:value-of select="$location"/></Type>
            <Type><xsl:value-of select="$nature"/></Type>
            <Content><xsl:value-of select="."/></Content>
        </Telephone>
    </xsl:template>

    <xsl:template match="C1:HomeFaxNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'HOME'"/>
            <xsl:with-param name="nature" select="'FAX'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:BusinessFaxNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'WORK'"/>
            <xsl:with-param name="nature" select="'FAX'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:HomePhoneNumber | C1:Home2PhoneNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'HOME'"/>
            <xsl:with-param name="nature" select="'VOICE'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:BusinessPhoneNumber | C1:Business2PhoneNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'WORK'"/>
            <xsl:with-param name="nature" select="'VOICE'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:CarPhoneNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'CAR'"/>
            <xsl:with-param name="nature" select="'VOICE'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:MobilePhoneNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'CELL'"/>
            <xsl:with-param name="nature" select="'VOICE'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:PagerNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'PAGER'"/>
            <xsl:with-param name="nature" select="'VOICE'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:AssistnamePhoneNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'ASSISTANT'"/>
            <xsl:with-param name="nature" select="'VOICE'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C2:CompanyMainPhone">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'COMPANY'"/>
            <xsl:with-param name="nature" select="'VOICE'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:RadioPhoneNumber">
        <xsl:call-template name="telephone">
            <xsl:with-param name="location" select="'RADIO'"/>
            <xsl:with-param name="nature" select="'VOICE'"/>
        </xsl:call-template>
    </xsl:template>

    <xsl:template match="C1:JobTitle">
        <Title><Content><xsl:value-of select="."/></Content></Title>
    </xsl:template>

    <xsl:template match="C1:WebPage">
        <Url><Content><xsl:value-of select="."/></Content></Url>
    </xsl:template>

    <xsl:template match="C1:Anniversary">
        <Anniversary><Content>
            <xsl:value-of select="translate(substring-before(string(.),'T'),'-','')"/>
        </Content></Anniversary>
    </xsl:template>

    <xsl:template match="C1:Birthday">
        <Birthday><Content>
            <xsl:value-of select="translate(substring-before(string(.),'T'),'-','')"/>
        </Content></Birthday>
    </xsl:template>

    <xsl:template match="C1:Rtf">
        <Note><Content><xsl:value-of select="convert:all_description_from_airsync()"/></Content></Note>
    </xsl:template>

    <!--<xsl:template match="processing-instruction()|comment()">-->
    <xsl:template match="*">
    </xsl:template>


</xsl:transform>
