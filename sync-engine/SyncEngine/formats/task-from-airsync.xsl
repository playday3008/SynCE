<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               xmlns:T="http://synce.org/formats/airsync_wm5/tasks"
               exclude-result-prefixes="convert T AS">

<xsl:template match="ApplicationData | AS:ApplicationData">
    <vcal>
	<Todo>
               <xsl:for-each select="T:Reminder[position() = 1]">
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

               	<Summary><Content><xsl:value-of select="T:Subject"/></Content></Summary>

               	<xsl:for-each select="T:DueDate[position() = 1]">
                    <DateDue><Content><xsl:value-of select="convert:task_date_from_airsync()"/></Content></DateDue>
        	</xsl:for-each>

               	<xsl:for-each select="T:StartDate[position() = 1]">
                    <DateStarted><Content><xsl:value-of select="convert:task_date_from_airsync()"/></Content></DateStarted>
                </xsl:for-each>

		<xsl:for-each select="T:Sensitivity[position() = 1]">
		    <Class><Content><xsl:value-of select="convert:task_classification_from_airsync()"/></Content></Class>
		</xsl:for-each>

                <Categories>
                    <xsl:for-each select="T:Categories">
                        <xsl:for-each select="T:Category">
                            <Category><xsl:value-of select="."/></Category>
                        </xsl:for-each>
                    </xsl:for-each>
                </Categories>

		<xsl:for-each select="T:Complete">
			<xsl:value-of select="convert:task_status_from_airsync()"/>
		</xsl:for-each>

		<xsl:for-each select="T:Importance">
			<Priority><Content><xsl:value-of select="convert:task_prio_from_airsync()"/></Content></Priority>
		</xsl:for-each>
	
		<xsl:for-each select="T:Rtf">
			<Description><Content><xsl:value-of select="convert:all_description_from_airsync()"/></Content></Description>
		</xsl:for-each>

	</Todo>
    </vcal>
</xsl:template>

</xsl:transform>
