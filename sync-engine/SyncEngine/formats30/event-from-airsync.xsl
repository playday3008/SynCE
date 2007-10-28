<?xml version="1.0" encoding="UTF-8"?>

<!-- This file conforms to the OpenSync>v0.3 schemas -->

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

            <event>

		<!-- TODO for OpenSync 0.3. This is very different -->

                <xsl:for-each select = "C:Timezone[position() = 1]">
                     <xsl:value-of select="tz:ConvertASTimezoneToVcal()"/>
                </xsl:for-each>

		<!-- OpenSync 0.3 - Value becomes ATTRIBUTE -->

                <xsl:for-each select="C:Reminder[position() = 1]">
                    <Alarm>
                        <xsl:attribute name="Value">DURATION</xsl:attribute>
                        <AlarmAction>DISPLAY</AlarmAction>
                        <AlarmDescription><xsl:value-of select="Subject"/></AlarmDescription>
                        <AlarmTrigger><xsl:value-of select="convert:event_reminder_from_airsync()"/></AlarmTrigger>
                    </Alarm>
                </xsl:for-each>

		<!-- OpenSync 0.3 - 'Transparency' becomes TimeTransparency -->

                <xsl:for-each select="C:BusyStatus[position() = 1]">
                    <TimeTransparency><xsl:value-of select="convert:event_busystatus_from_airsync()"/></TimeTransparency>
                </xsl:for-each>

		<!-- OpenSync 0.3 - LastModified remains the same, TimezoneID is an attribute, and the date value type is specced -->

                <xsl:for-each select="C:DtStamp[position() = 1]">
                    <LastModified>
                        <xsl:attribute name="Value">DATE-TIME</xsl:attribute>
                        <xsl:value-of select="convert:event_dtstamp_from_airsync()"/>
                    </LastModified>
                </xsl:for-each>

		<!-- OpenSync 0.3 - DateStarted remains the same. TimezoneID is an attribute as is date value type -->

                <xsl:for-each select="C:StartTime[position() = 1]">
                    <DateStarted>
                        <xsl:attribute name="Value">DATE-TIME</xsl:attribute>
                        <xsl:value-of select="convert:event_starttime_from_airsync()"/>
                    </DateStarted>
                </xsl:for-each>

                <!-- OpenSync 0.3 - As for DateStarted -->

                <xsl:for-each select="C:EndTime[position() = 1]">
                    <DateEnd>
                         <xsl:attribute name="Value">DATE-TIME</xsl:attribute>
                         <xsl:value-of select="convert:event_endtime_from_airsync()"/>
                    </DateEnd>
                </xsl:for-each>

                <!-- OpenSync 0.3 - Could have more than one Content to indicate more than one line
                     but it appears that AirSync does not support this anyway -->

                <Location>
                    <Content>
                        <xsl:value-of select="C:Location"/>
                    </Content>
                </Location>

                <!-- OpenSync 0.3 - As for Location -->

                <Summary>
                    <Content>
                        <xsl:value-of select="C:Subject"/>
                    </Content>
                </Summary>

                <!-- OpenSync 0.3 - Description as for Location - could contain multiple Content lines -->

		<xsl:for-each select="C:Rtf">
			<Description>
                            <Content><xsl:value-of select="convert:all_description_from_airsync()"/></Content>
                        </Description>
		</xsl:for-each>

                <!-- OpenSync 0.3 - Remove 'Content', correct function -->

                <xsl:for-each select="Sensitivity[position() = 1]">
                    <Class><xsl:value-of select="convert:event_sensitivity_from_airsync()"/></Class>
                </xsl:for-each>

                <!-- OpenSync 0.3 - Categories remain the same -->

                <Categories>
                    <xsl:for-each select="C:Categories">
                        <xsl:for-each select="C:Category">
                            <Category><xsl:value-of select="."/></Category>
                        </xsl:for-each>
                    </xsl:for-each>
                </Categories>

                <!-- OpenSync 0.3 - For the moment, keep Attendee entries the same -->

                <xsl:for-each select="C:Attendees/C:Attendee">
                    <Attendee>
                        <xsl:value-of select="convert:event_attendee_from_airsync()"/>
                    </Attendee>
                </xsl:for-each>

                <!-- OpenSync 0.3 - Major changes to Recurrences. Modify function -->

                <xsl:for-each select="C:Recurrence[position() = 1]">
                    <RecurrenceRule>
                        <xsl:value-of select="convert:event_recurrence_from_airsync()"/>
                    </RecurrenceRule>
                </xsl:for-each>

                <!-- OpenSync 0.3 - ExclusionDate becomes ExceptionDateTime -->

                <xsl:for-each select="C:Exceptions/C:Exception">
                    <ExceptionDateTime>
                        <xsl:attribute name="Value">DATE-TIME</xsl:attribute>
                        <xsl:value-of select="convert:event_exception_from_airsync()"/>
                    </ExceptionDateTime>
                </xsl:for-each>

            </event>
    </xsl:template>

</xsl:transform>
