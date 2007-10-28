<?xml version="1.0" encoding="UTF-8"?>

<!-- This file conforms to OpenSync > 0.30 schemas -->

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:tz="http://synce.org/tz"
               exclude-result-prefixes="convert tz">

    <xsl:template match="/event">

        <AS:ApplicationData xmlns:AS="http://synce.org/formats/airsync_wm5/airsync" xmlns="http://synce.org/formats/airsync_wm5/calendar">

            <!-- TODO: Timezones are way different in OpenSync 0.30 -->

            <xsl:for-each select="Timezone">
                <Timezone><xsl:value-of select="tz:ExtractTZData()"/></Timezone>
            </xsl:for-each>

            <!-- OpenSync 0.3 schema allows unbounded number of 'Alarm' elements -->

            <xsl:for-each select="Alarm[position() = 1]">
                <Reminder><xsl:value-of select="convert:event_reminder_to_airsync()"/></Reminder>
            </xsl:for-each>

            <!-- OpenSync 0.3 - Transparency becomes TimeTransparency -->

            <xsl:for-each select="TimeTransparency[position() = 1]">
                <BusyStatus><xsl:value-of select="convert:event_busystatus_to_airsync()"/></BusyStatus>
            </xsl:for-each>

            <!-- OpenSync 0.3 - LastModified: Ignore the attribute for now -->

            <xsl:for-each select="LastModified[position() = 1]">
                <DtStamp><xsl:value-of select="convert:event_dtstamp_to_airsync()"/></DtStamp>
            </xsl:for-each>

            <xsl:if test="not(LastModified)">
                <DtStamp><xsl:value-of select="convert:event_dtstamp_from_now()"/></DtStamp>
            </xsl:if>

            <!-- OpenSync 0.3 - DateStarted no longer has Content -->

            <xsl:for-each select="DateStarted[position() = 1]">
                <AllDayEvent><xsl:value-of select="convert:event_alldayevent_to_airsync()"/></AllDayEvent>
            </xsl:for-each>

            <xsl:for-each select="DateStarted[position() = 1]">
                <StartTime><xsl:value-of select="convert:event_starttime_to_airsync()"/></StartTime>
            </xsl:for-each>

            <!-- OpenSync 0.3 - DateEnd no longer has Content -->

            <xsl:for-each select="DateEnd[position() = 1]">
                <EndTime><xsl:value-of select="convert:event_endtime_to_airsync()"/></EndTime>
            </xsl:for-each>

            <!-- OpenSync 0.3 - Location, Subject and Description are unchanged -->

            <Location><xsl:value-of select="Location/Content"/></Location>

            <Subject><xsl:value-of select="Summary/Content"/></Subject>

	    <xsl:for-each select="Description/Content[position() = 1]">
		<Rtf><xsl:value-of select="convert:all_description_to_airsync()"/></Rtf>
	    </xsl:for-each>

            <!-- OpenSync 0.3 - Class no longer has Content -->

            <xsl:for-each select="Class[position() = 1]">
                <Sensitivity><xsl:value-of select="convert:event_sensitivity_to_airsync()"/></Sensitivity>
            </xsl:for-each>

            <!-- OpenSync 0.3 - Categories remain the same -->

            <Categories>
                <xsl:for-each select="Categories">
                    <xsl:for-each select="Category">
                        <Category><xsl:value-of select="."/></Category>
                    </xsl:for-each>
                </xsl:for-each>
            </Categories>

            <!-- OpenSync 0.3 - For the moment, keep Attendees the same -->

            <Attendees>
                <xsl:for-each select="Attendee">
                    <Attendee>
                        <xsl:value-of select="convert:event_attendee_to_airsync()"/>
                    </Attendee>
                </xsl:for-each>
            </Attendees>

            <!-- OpenSync 0.3 - Major changes to recurrences, update function -->

            <xsl:for-each select="RecurrenceRule[position() = 1]">
		<Recurrence>
                    <xsl:value-of select="convert:event_recurrence_to_airsync()"/>
                </Recurrence>
            </xsl:for-each>

            <!-- ExclusionDate becomes ExclusionDateTime -->

            <xsl:for-each select="ExclusionDateTime[position() = 1]">
                <Exceptions>
                    <xsl:for-each select="//ExclusionDateTime">
                        <Exception>
                            <xsl:value-of select="convert:event_exception_to_airsync()"/>
                        </Exception>
                    </xsl:for-each>
                </Exceptions>
            </xsl:for-each>

        </AS:ApplicationData>
    </xsl:template>

</xsl:transform>
