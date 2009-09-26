<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:tz="http://synce.org/tz"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               xmlns:C="http://synce.org/formats/airsync_wm5/calendar"
               exclude-result-prefixes="convert tz C AS">

    <!-- NOTE: We must specify the AS namespace here in order for our tests to run correctly.  The current
               tests have the ApplicationData element placed inside the 'http://synce.org/formats/airsync_wm5/contacts' namespace.
               However, when we get data from the device, it will be a fragment of an XML document and thus the ApplicationData
               element will not be part of a namespace. -->

    <xsl:template match="ApplicationData | AS:ApplicationData">
        <vcal>

            <xsl:if test="C:Timezone">
                <xsl:value-of select="tz:ConvertASTimezoneToVcal()"/>
            </xsl:if>

            <Event>
                <xsl:apply-templates/>
            </Event>

        </vcal>
    </xsl:template>


    <xsl:template match="C:Reminder">
        <Alarm>
            <AlarmTrigger>
                <Content><xsl:value-of select="convert:event_reminder_from_airsync()"/></Content>
                <Value>DURATION</Value>
                <Related>START</Related>
            </AlarmTrigger>
            <AlarmAction>DISPLAY</AlarmAction>
            <AlarmDescription><xsl:value-of select="../C:Subject"/></AlarmDescription>
        </Alarm>
    </xsl:template>

    <xsl:template match="C:BusyStatus">
        <Transparency><Content>
            <xsl:choose>
                <xsl:when test="string(.) = '0'">
                    <xsl:text>TRANSPARENT</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>OPAQUE</xsl:text> <!-- 'Busy' is our default value -->
                </xsl:otherwise>
            </xsl:choose>
        </Content></Transparency>
    </xsl:template>

    <xsl:template match="C:DtStamp">
        <LastModified><Content><xsl:value-of select="convert:event_datetime_from_airsync()"/></Content></LastModified>
    </xsl:template>

    <xsl:template match="C:StartTime">
        <DateStarted><xsl:value-of select="convert:event_starttime_from_airsync()"/></DateStarted>
    </xsl:template>

    <xsl:template match="C:EndTime">
        <DateEnd><xsl:value-of select="convert:event_endtime_from_airsync()"/></DateEnd>
    </xsl:template>

    <xsl:template match="C:Location">
        <Location><Content><xsl:value-of select="."/></Content></Location>
    </xsl:template>

    <xsl:template match="C:Subject">
        <Summary><Content><xsl:value-of select="."/></Content></Summary>
    </xsl:template>

    <xsl:template match="C:Rtf">
        <Description><Content><xsl:value-of select="convert:all_description_from_airsync()"/></Content></Description>
    </xsl:template>

    <xsl:template match="C:Sensitivity">
        <Class><Content>
            <xsl:choose>
                <xsl:when test="string(.) = '2'">
                    <xsl:text>PRIVATE</xsl:text>
                </xsl:when>
                <xsl:when test="string(.) = '3'">
                    <xsl:text>CONFIDENTIAL</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>PUBLIC</xsl:text> <!-- 'PUBLIC' is our default value -->
                </xsl:otherwise>
            </xsl:choose>
        </Content></Class>
    </xsl:template>

    <xsl:template match="C:Categories">
        <Categories>
            <xsl:apply-templates />
        </Categories>
    </xsl:template>

    <xsl:template match="C:Category">
        <Category><xsl:value-of select="."/></Category>
    </xsl:template>

    <xsl:template match="C:Attendees">
        <xsl:apply-templates/>
    </xsl:template>
    <xsl:template match="C:Attendee">
        <Attendee>
            <xsl:if test="C:Email">
                <Content>MAILTO:<xsl:value-of select="C:Email"/></Content>
            </xsl:if>
            <CommonName><xsl:value-of select="C:Name"/></CommonName>
        </Attendee>
    </xsl:template>

    <xsl:template match="C:Recurrence">
        <RecurrenceRule>
            <xsl:value-of select="convert:event_recurrence_from_airsync()"/>
        </RecurrenceRule>
    </xsl:template>

    <xsl:template match="C:Exceptions">
        <xsl:apply-templates/>
    </xsl:template>
    <xsl:template match="C:Exception">
        <xsl:if test="C:Deleted = 1">
            <ExclusionDate>
                <Value>DATE</Value>
                <xsl:apply-templates/>
            </ExclusionDate>
        </xsl:if>
    </xsl:template>
    <xsl:template match="C:ExceptionStartTime">
        <Content><xsl:value-of select="convert:event_datetime_short_from_airsync()"/></Content>
    </xsl:template>

    <xsl:template match="*">
        <!-- <xsl:message>Ignored element <xsl:value-of select="name()"/></xsl:message> -->
    </xsl:template>

</xsl:transform>
