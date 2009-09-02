<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:tz="http://synce.org/tz"
               exclude-result-prefixes="convert tz">

    <xsl:template match="/vcal">

        <AS:ApplicationData xmlns:AS="http://synce.org/formats/airsync_wm5/airsync" xmlns="http://synce.org/formats/airsync_wm5/calendar">

            <xsl:for-each select="Timezone">
                <Timezone><xsl:value-of select="tz:ExtractTZData()"/></Timezone>
            </xsl:for-each>


            <xsl:for-each select="Event/Alarm/AlarmTrigger[position() = 1]">
                <Reminder><xsl:value-of select="convert:event_reminder_to_airsync()"/></Reminder>
            </xsl:for-each>

            <xsl:choose>
                <xsl:when test="Event/Transparency">
                    <xsl:for-each select="Event/Transparency/Content[position() = 1]">
                        <BusyStatus><xsl:value-of select="convert:event_busystatus_to_airsync()"/></BusyStatus>
                    </xsl:for-each>
                </xsl:when>
                <xsl:otherwise>
                    <!-- Default to OPAQUE: http://tools.ietf.org/html/rfc2445#section-4.8.2.7 -->
                    <BusyStatus>2</BusyStatus>
                </xsl:otherwise>
            </xsl:choose>

            <xsl:for-each select="Event/LastModified[position() = 1]">
                <DtStamp><xsl:value-of select="convert:event_dtstamp_to_airsync()"/></DtStamp>
            </xsl:for-each>

            <xsl:if test="not(Event/LastModified/Content)">
                <DtStamp><xsl:value-of select="convert:event_dtstamp_from_now()"/></DtStamp>
            </xsl:if>
    
            <xsl:for-each select="Event/DateStarted/Content[position() = 1]">
                <AllDayEvent><xsl:value-of select="convert:event_alldayevent_to_airsync()"/></AllDayEvent>
            </xsl:for-each>

            <xsl:for-each select="Event/DateStarted[position() = 1]">
                <StartTime><xsl:value-of select="convert:event_starttime_to_airsync()"/></StartTime>
            </xsl:for-each>

            <xsl:for-each select="Event/DateEnd[position() = 1]">
                <EndTime><xsl:value-of select="convert:event_endtime_to_airsync()"/></EndTime>
            </xsl:for-each>

            <Location><xsl:value-of select="Event/Location/Content"/></Location>

            <Subject><xsl:value-of select="Event/Summary/Content"/></Subject>

	    <xsl:for-each select="Event/Description/Content[position() = 1]">
		<Rtf><xsl:value-of select="convert:all_description_to_airsync()"/></Rtf>
	    </xsl:for-each>

            <xsl:for-each select="Event/Class/Content[position() = 1]">
                <Sensitivity><xsl:value-of select="convert:event_sensitivity_to_airsync()"/></Sensitivity>
            </xsl:for-each>

            <Categories>
                <xsl:for-each select="Event/Categories">
                    <xsl:for-each select="Category">
                        <Category><xsl:value-of select="."/></Category>
                    </xsl:for-each>
                </xsl:for-each>
            </Categories>

            <Attendees>
                <xsl:for-each select="Event/Attendee">
                    <Attendee>
                        <xsl:value-of select="convert:event_attendee_to_airsync()"/>
                    </Attendee>
                </xsl:for-each>
            </Attendees>

            <xsl:for-each select="Event/RecurrenceRule[position() = 1]">
                <Recurrence>
                    <xsl:value-of select="convert:event_recurrence_to_airsync()"/>
                </Recurrence>
            </xsl:for-each>

            <xsl:for-each select="Event/ExclusionDate[position() = 1]">
                <Exceptions>
                    <xsl:for-each select="//Event/ExclusionDate">
                        <Exception>
                            <xsl:value-of select="convert:event_exception_to_airsync()"/>
                        </Exception>
                    </xsl:for-each>
                </Exceptions>
            </xsl:for-each>

        </AS:ApplicationData>
    </xsl:template>

</xsl:transform>
