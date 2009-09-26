<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:tz="http://synce.org/tz"
               xmlns="http://synce.org/formats/airsync_wm5/calendar"
               exclude-result-prefixes="convert tz">

    <xsl:template match="/vcal">
        <AS:ApplicationData xmlns:AS="http://synce.org/formats/airsync_wm5/airsync" xmlns="http://synce.org/formats/airsync_wm5/calendar">
            <xsl:apply-templates/>
        </AS:ApplicationData>
    </xsl:template>


    <xsl:template match="Timezone">
        <Timezone><xsl:value-of select="tz:ExtractTZData()"/></Timezone>
    </xsl:template>


    <xsl:template match="Event">

        <xsl:apply-templates/>

        <xsl:if test="not(Transparency)">
            <!-- Default to OPAQUE: http://tools.ietf.org/html/rfc2445#section-4.8.2.7 -->
            <BusyStatus>2</BusyStatus>
        </xsl:if>

        <xsl:if test="not(LastModified/Content)">
            <DtStamp><xsl:value-of select="convert:event_dtstamp_from_now()"/></DtStamp>
        </xsl:if>
    
        <xsl:if test="Attendee">
            <Attendees>
                <xsl:apply-templates select="Attendee" mode="attendees"/>
            </Attendees>
        </xsl:if>

        <xsl:if test="ExclusionDate">
            <Exceptions>
                <xsl:apply-templates select="ExclusionDate" mode="exclusion"/>
            </Exceptions>
        </xsl:if>

    </xsl:template>

    <xsl:template match="Alarm">
        <xsl:apply-templates/>
    </xsl:template>

    <xsl:template match="AlarmTrigger">
        <Reminder><xsl:value-of select="convert:event_reminder_to_airsync()"/></Reminder>
    </xsl:template>

    <xsl:template match="LastModified">
        <DtStamp><xsl:value-of select="convert:event_time_to_airsync()"/></DtStamp>
    </xsl:template>

    <xsl:template match="DateStarted">
        <xsl:choose>
            <xsl:when test="Value = 'DATE'">
                <AllDayEvent>1</AllDayEvent>
            </xsl:when>
            <xsl:otherwise>
                <AllDayEvent>0</AllDayEvent>
            </xsl:otherwise>
        </xsl:choose>
        <StartTime><xsl:value-of select="convert:event_time_to_airsync()"/></StartTime>
    </xsl:template>

    <xsl:template match="DateEnd">
        <EndTime><xsl:value-of select="convert:event_time_to_airsync()"/></EndTime>
    </xsl:template>

    <xsl:template match="Location">
        <Location><xsl:value-of select="Content"/></Location>
    </xsl:template>

    <xsl:template match="Summary">
        <Subject><xsl:value-of select="Content"/></Subject>
    </xsl:template>

    <xsl:template match="Description">
            <Rtf><xsl:value-of select="convert:all_description_to_airsync()"/></Rtf>
    </xsl:template>

    <xsl:template match="Class">
        <Sensitivity>
            <xsl:choose>
                <xsl:when test="Content = 'PRIVATE'">
                    <xsl:text>2</xsl:text>
                </xsl:when>
                <xsl:when test="Content = 'CONFIDENTIAL'">
                    <xsl:text>3</xsl:text>
                </xsl:when>
                <xsl:otherwise> <!-- "PUBLIC" is our default value -->
                    <xsl:text>0</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </Sensitivity>
    </xsl:template>

    <xsl:template match="Categories">
        <Categories>
            <xsl:apply-templates/>
        </Categories>
    </xsl:template>

    <xsl:template match="Category">
        <Category><xsl:value-of select="."/></Category>
    </xsl:template>

    <xsl:template match="Attendee" mode="attendees">
        <Attendee>
            <xsl:if test="CommonName">
                <Name><xsl:value-of select="CommonName"/></Name>
            </xsl:if>
            <Email><xsl:value-of select="substring(Content,8)"/></Email>
        </Attendee>
    </xsl:template>

    <xsl:template match="RecurrenceRule">
        <Recurrence>
            <xsl:value-of select="convert:event_recurrence_to_airsync()"/>
        </Recurrence>
    </xsl:template>

    <xsl:template match="ExclusionDate" mode="exclusion">
        <Exception>
            <xsl:choose>
                <xsl:when test="Value = 'DATE'">
                    <Deleted>1</Deleted>
                    <ExceptionStartTime>
                        <xsl:value-of select="convert:event_datetime_from_airsync(Content)"/>
                    </ExceptionStartTime>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:message>Exclusions with values other than 'DATE' are not supported</xsl:message>
                </xsl:otherwise>
            </xsl:choose>
        </Exception>
    </xsl:template>

    <xsl:template match="Transparency">
        <BusyStatus>
            <xsl:choose>
                <xsl:when test="Content = 'TRANSPARENT'">
                    <xsl:text>0</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>2</xsl:text> <!-- 'Busy' is our default value -->
                </xsl:otherwise>
            </xsl:choose>
        </BusyStatus>
    </xsl:template>

    <xsl:template match="*">
        <!-- <xsl:message>Ignored tag <xsl:value-of select="name(.)"/></xsl:message> -->
    </xsl:template>

</xsl:transform>
