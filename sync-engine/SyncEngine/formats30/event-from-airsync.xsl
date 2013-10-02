<?xml version="1.0" encoding="UTF-8"?>

<!-- This file conforms to the OpenSync>v0.3 schemas -->

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:common="http://synce.org/common"
               xmlns:event="http://synce.org/event"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               xmlns:C="http://synce.org/formats/airsync_wm5/calendar"
               exclude-result-prefixes="common event C AS">

    <!-- NOTE: We must specify the AS namespace here in order for our tests to run correctly.  The current
               tests have the ApplicationData element placed inside the 'http://synce.org/formats/airsync_wm5/contacts' namespace.
               However, when we get data from the device, it will be a fragment of an XML document and thus the ApplicationData
               element will not be part of a namespace. -->

    <xsl:template match="ApplicationData | AS:ApplicationData">

            <event>

		<!-- TODO for OpenSync 0.3. This is very different -->

                <xsl:for-each select = "C:Timezone[position() = 1]">
                     <xsl:value-of select="common:TZFromAirsync()"/>
                </xsl:for-each>

		<xsl:value-of select="common:AllTZToOpensync()"/>

		<!-- OpenSync 0.3 - AlarmTrigger is becomes attribute 'Value' -->

                <xsl:for-each select="C:Reminder[position() = 1]">
                    <AlarmDisplay>
			<xsl:attribute name="RelatedType">START</xsl:attribute>
                        <xsl:attribute name="Value">DURATION</xsl:attribute>
                        <AlarmDescription><xsl:value-of select="../C:Subject[position() = 1]"/></AlarmDescription>
                        <AlarmTrigger><xsl:value-of select="common:AlarmFromAirsync()"/></AlarmTrigger>
                    </AlarmDisplay>
                </xsl:for-each>

		<!-- OpenSync 0.3 - 'Transparency' becomes TimeTransparency -->

                <xsl:for-each select="C:BusyStatus[position() = 1]">
                    <TimeTransparency><Content><xsl:value-of select="event:TimeTransparencyFromAirsync()"/></Content></TimeTransparency>
                </xsl:for-each>

		<!-- OpenSync 0.3 - LastModified remains the same, TimezoneID is an attribute, and the date value type is specced -->

                <xsl:for-each select="C:DtStamp[position() = 1]">
                    <LastModified>
                        <xsl:value-of select="common:OSDateFromAirsync()"/>
                    </LastModified>
                </xsl:for-each>

		<!-- OpenSync 0.3 - DateStarted remains the same. TimezoneID is an attribute as is date value type -->

                <xsl:for-each select="C:StartTime[position() = 1]">
                    <DateStarted>
                        <xsl:value-of select="common:OSDateFromAirsync()"/>
                    </DateStarted>
                </xsl:for-each>

                <!-- OpenSync 0.3 - As for DateStarted -->

                <xsl:for-each select="C:EndTime[position() = 1]">
                    <DateEnd>
                         <xsl:value-of select="common:OSDateFromAirsync()"/>
                    </DateEnd>
                </xsl:for-each>

                <!-- OpenSync 0.3 - Could have more than one Content to indicate more than one line
                     but it appears that AirSync does not support this anyway -->

                <xsl:for-each select="C:Location[position()=1]">
                    <Location>
                        <Content>
                            <xsl:value-of select="."/>
                        </Content>
                    </Location>
                </xsl:for-each>

                <!-- OpenSync 0.3 - As for Location -->

                <xsl:for-each select="C:Subject[position()=1]">
                    <Summary>
                        <Content>
                            <xsl:value-of select="."/>
                        </Content>
                    </Summary>
                </xsl:for-each>

                <!-- OpenSync 0.3 - Description as for Location - could contain multiple Content lines -->

		<xsl:for-each select="C:Rtf">
			<Description>
                            <Content><xsl:value-of select="common:OSTextFromAirsyncRTF()"/></Content>
                        </Description>
		</xsl:for-each>

                <!-- OpenSync 0.3  -->

                <xsl:for-each select="C:Sensitivity[position() = 1]">
                    <Class><Content><xsl:value-of select="common:ClassFromAirsync()"/></Content></Class>
                </xsl:for-each>

                <!-- OpenSync 0.3 - Categories remain the same -->

                <xsl:if test="count(C:Categories) &gt; 0">
                    <Categories>
                        <xsl:for-each select="C:Categories">
                            <xsl:for-each select="C:Category">
                                <Category><xsl:value-of select="."/></Category>
                            </xsl:for-each>
                        </xsl:for-each>
                    </Categories>
                </xsl:if>

                <!-- OpenSync 0.3 - For the moment, keep Attendee entries the same -->

                <xsl:for-each select="C:Attendees/C:Attendee">
                    <Attendee>
                        <xsl:value-of select="event:AttendeeFromAirsync()"/>
                    </Attendee>
                </xsl:for-each>

                <!-- OpenSync 0.3 - Major changes to Recurrences. See function -->

                <xsl:for-each select="C:Recurrence[position() = 1]">
                    <RecurrenceRule>
                        <xsl:value-of select="common:RecurrenceRuleFromAirsync()"/>
                    </RecurrenceRule>
                </xsl:for-each>

                <!-- OpenSync 0.3 - ExclusionDate becomes ExceptionDateTime -->

                <xsl:for-each select="C:Exceptions/C:Exception">
                    <ExceptionDateTime>
                        <xsl:attribute name="Value">DATE-TIME</xsl:attribute>
                        <xsl:value-of select="event:ExceptionDateTimeFromAirsync()"/>
                    </ExceptionDateTime>
                </xsl:for-each>

            </event>
    </xsl:template>

</xsl:transform>
