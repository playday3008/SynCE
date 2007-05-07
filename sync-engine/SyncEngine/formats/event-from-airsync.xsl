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

        <xsl:for-each select = "C:Timezone[position() = 1]">
                <xsl:value-of select="tz:ConvertASTimezoneToVcal()"/>
        </xsl:for-each>

            <Event>
                <xsl:for-each select="C:Reminder[position() = 1]">
                    <Alarm>
                        <AlarmTrigger>
                            <Content><xsl:value-of select="convert:event_reminder_from_airsync()"/></Content>
                            <Value>DURATION</Value>
                            <Related>START</Related>
                        </AlarmTrigger>
                        <AlarmAction>DISPLAY</AlarmAction>
                        <AlarmDescription><xsl:value-of select="Subject"/></AlarmDescription>
                    </Alarm>
                </xsl:for-each>

                <xsl:for-each select="C:BusyStatus[position() = 1]">
                    <Transparency><Content><xsl:value-of select="convert:event_busystatus_from_airsync()"/></Content></Transparency>
                </xsl:for-each>

                <xsl:for-each select="C:DtStamp[position() = 1]">
                    <LastModified><Content><xsl:value-of select="convert:event_dtstamp_from_airsync()"/></Content></LastModified>
                </xsl:for-each>

                <xsl:for-each select="C:StartTime[position() = 1]">
                    <DateStarted><xsl:value-of select="convert:event_starttime_from_airsync()"/></DateStarted>
                </xsl:for-each>

                <xsl:for-each select="C:EndTime[position() = 1]">
                    <DateEnd><xsl:value-of select="convert:event_endtime_from_airsync()"/></DateEnd>
                </xsl:for-each>

                <Location><Content><xsl:value-of select="C:Location"/></Content></Location>

                <Summary><Content><xsl:value-of select="C:Subject"/></Content></Summary>

		<xsl:for-each select="C:Rtf">
			<Description><Content><xsl:value-of select="convert:all_description_from_airsync()"/></Content></Description>
		</xsl:for-each>

                <xsl:for-each select="Sensitivity[position() = 1]">
                    <Class><Content><xsl:value-of select="convert:event_sensitivity_from_airsync()"/></Content></Class>
                </xsl:for-each>

                <Categories>
                    <xsl:for-each select="C:Categories">
                        <xsl:for-each select="C:Category">
                            <Category><xsl:value-of select="."/></Category>
                        </xsl:for-each>
                    </xsl:for-each>
                </Categories>

                <xsl:for-each select="C:Attendees/C:Attendee">
                    <Attendee>
                        <xsl:value-of select="convert:event_attendee_from_airsync()"/>
                    </Attendee>
                </xsl:for-each>

                <xsl:for-each select="C:Recurrence[position() = 1]">
                    <RecurrenceRule>
                        <xsl:value-of select="convert:event_recurrence_from_airsync()"/>
                    </RecurrenceRule>
                </xsl:for-each>

                <xsl:for-each select="C:Exceptions/C:Exception">
                    <ExclusionDate>
                        <xsl:value-of select="convert:event_exception_from_airsync()"/>
                    </ExclusionDate>
                </xsl:for-each>

            </Event>
        </vcal>
    </xsl:template>

</xsl:transform>
