<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:AS="AirSync:"
               xmlns:C="POOMCAL:"
               exclude-result-prefixes="convert C AS">

    <!-- NOTE: We must specify the AS namespace here in order for our tests to run correctly.  The current
               tests have the ApplicationData element placed inside the 'AirSync:' namespace.  However, when
               we get data from the device, it will be a fragment of an XML document and thus the ApplicationData
               element will not be part of a namespace. -->

    <xsl:template match="ApplicationData | AS:ApplicationData">
        <vcal>
            <Event>
                <xsl:for-each select="C:Reminder[position() = 1]">
                    <Alarm>
                        <AlarmTrigger>
                            <Content><xsl:value-of select="convert:event_reminder_from_airsync()"/></Content>
                            <Value>DURATION</Value>
                            <Related>Start</Related>
                        </AlarmTrigger>
                        <AlarmAction>DISPLAY</AlarmAction>
                        <AlarmDescription><xsl:value-of select="Subject"/></AlarmDescription>
                    </Alarm>
                </xsl:for-each>

                <xsl:for-each select="C:BusyStatus[position() = 1]">
                    <Transparency><Content><xsl:value-of select="convert:event_busystatus_from_airsync()"/></Content></Transparency>
                </xsl:for-each>

                <xsl:for-each select="C:DtStamp[position() = 1]">
                    <LastModified><Content><xsl:value-of select="convert:event_dtstamp_to_airsync()"/></Content></LastModified>
                </xsl:for-each>

                <xsl:for-each select="C:StartTime[position() = 1]">
                    <DateStarted><Content><xsl:value-of select="convert:event_starttime_from_airsync()"/></Content></DateStarted>
                </xsl:for-each>

                <xsl:for-each select="C:EndTime[position() = 1]">
                    <DateEnd><Content><xsl:value-of select="convert:event_endtime_from_airsync()"/></Content></DateEnd>
                </xsl:for-each>

                <Location><Content><xsl:value-of select="C:Location"/></Content></Location>

                <Summary><Content><xsl:value-of select="C:Subject"/></Content></Summary>

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
