<?xml version="1.0" encoding="UTF-8"?>

<!-- This file conforms to OpenSync > 0.30 schemas -->

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:event="http://synce.org/event"
               xmlns:common="http://synce.org/common"
               exclude-result-prefixes="common event">

    <xsl:template match="/event">

        <AS:ApplicationData xmlns:AS="http://synce.org/formats/airsync_wm5/airsync" xmlns="http://synce.org/formats/airsync_wm5/calendar">

            <!-- Timezones are way different in OpenSync 0.30 
                 We gather together all timezone-specific entries before
                 anything else is processed -->

            <xsl:for-each select="Timezone">
                <xsl:value-of select="common:HandleOSTZ()"/>
            </xsl:for-each>
            <xsl:for-each select="TimezoneComponent">
                <xsl:value-of select="common:HandleOSTZComponent()"/>
            </xsl:for-each>
            <xsl:for-each select="TimezoneRule">
                <xsl:value-of select="common:HandleOSTZRule()"/>
            </xsl:for-each>

            <xsl:value-of select="common:CurrentTZToAirsync()"/>

            <!-- OpenSync 0.3 schema allows unbounded number of 'Alarm' elements -->

            <xsl:for-each select="AlarmDisplay[position() = 1]">
                <Reminder><xsl:value-of select="common:AlarmToAirsync()"/></Reminder>
            </xsl:for-each>

            <!-- OpenSync 0.3 - Transparency becomes TimeTransparency -->

            <xsl:for-each select="TimeTransparency[position() = 1]">
                <BusyStatus><xsl:value-of select="event:TimeTransparencyToAirsync()"/></BusyStatus>
            </xsl:for-each>

            <!-- OpenSync 0.3 - LastModified: Ignore the attribute for now -->

            <xsl:for-each select="LastModified[position() = 1]">
                <DtStamp><xsl:value-of select="common:OSDateToAirsync()"/></DtStamp>
            </xsl:for-each>

            <xsl:if test="not(LastModified)">
                <DtStamp><xsl:value-of select="common:AirsyncDateFromNow()"/></DtStamp>
            </xsl:if>

            <!-- OpenSync 0.3 - DateStarted uses DateValueType which is consistent
                 and uses the 'Value' attribute to distinguish between DATE and
                 DATE-TIME. Unfortunately, OpenSync does not always set this attribute
                 so if all else fails, use an extension to get it by brute force -->

            <xsl:for-each select="DateStarted[position() = 1]">
                <AllDayEvent>
                    <xsl:choose>
                        <xsl:when test="@Value = 'DATE'">1</xsl:when>
                        <xsl:when test="@Value = 'DATE-TIME'">0</xsl:when>
                        <xsl:otherwise><xsl:value-of select="event:AllDayEventToAirsync()"/></xsl:otherwise>
                    </xsl:choose>
                </AllDayEvent>
            </xsl:for-each>

            <!-- OpenSync 0.3 - The StartTime is a standard OpenSync date -->

            <xsl:for-each select="DateStarted[position() = 1]">
                <StartTime><xsl:value-of select="common:OSDateToAirsync()"/></StartTime>
            </xsl:for-each>

            <!-- OpenSync 0.3 - DateEnd is a standard OpenSync date -->

            <xsl:for-each select="DateEnd[position() = 1]">
                <EndTime><xsl:value-of select="common:OSDateToAirsync()"/></EndTime>
            </xsl:for-each>

            <!-- OpenSync 0.3 - Location, Subject and Description are unchanged -->

            <xsl:for-each select="Location[position()=1]">
                <Location><xsl:value-of select="Content"/></Location>
            </xsl:for-each>

            <xsl:for-each select="Summary[position()=1]">
                <Subject><xsl:value-of select="Content"/></Subject>
            </xsl:for-each>

	    <xsl:for-each select="Description/Content[position() = 1]">
		<Rtf><xsl:value-of select="common:OSTextToAirsyncRTF()"/></Rtf>
	    </xsl:for-each>

            <!-- OpenSync 0.3 - Class has same core content -->

            <xsl:for-each select="Class[position() = 1]">
                <Sensitivity><xsl:value-of select="common:ClassToAirsync()"/></Sensitivity>
            </xsl:for-each>

            <!-- OpenSync 0.3 - Categories remain the same -->

            <xsl:if test="count(Category) &gt; 0">
                <Categories>
                    <xsl:for-each select="Categories">
                        <xsl:for-each select="Category">
                            <Category><xsl:value-of select="."/></Category>
                        </xsl:for-each>
                    </xsl:for-each>
                </Categories>
            </xsl:if>

            <!-- OpenSync 0.3 - For the moment, keep Attendees the same -->

            <xsl:if test="count(Attendee) &gt; 0">
                <Attendees>
                    <xsl:for-each select="Attendee">
                        <Attendee>
                            <xsl:value-of select="event:AttendeeToAirsync()"/>
                        </Attendee>
                    </xsl:for-each>
                </Attendees>
            </xsl:if>

            <!-- OpenSync 0.3 - Major changes to recurrences, update function -->

            <xsl:for-each select="RecurrenceRule[position() = 1]">
		<Recurrence>
                    <xsl:value-of select="common:RecurrenceRuleToAirsync()"/>
                </Recurrence>
            </xsl:for-each>

            <!-- OpenSync 0.3 - ExceptionDateTime is a standard OS date-time complex, but
                 AirSync lists them  -->

            <xsl:if test="count(ExceptionDateTime) &gt; 0">
                <Exceptions>
                    <xsl:for-each select="ExceptionDateTime">
                        <Exception>
                            <xsl:value-of select="event:ExceptionDateTimeToAirsync()"/>
                        </Exception>
                    </xsl:for-each>
                </Exceptions>
            </xsl:if>

        </AS:ApplicationData>
    </xsl:template>

</xsl:transform>
