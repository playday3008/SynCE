<?xml version="1.0" encoding="UTF-8"?>

<!-- Amended for Opensync 0.3x -->

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:common="http://synce.org/common"
               xmlns:task="http://synce.org/task"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               xmlns:T="http://synce.org/formats/airsync_wm5/tasks"
               exclude-result-prefixes="common task T AS">

<xsl:template match="ApplicationData | AS:ApplicationData">
	
	<todo>

		<!-- OpenSync 0.3. This is very different -->

		<xsl:for-each select = "T:Timezone[position() = 1]">
                	<xsl:value-of select="tz:ConvertASTimezoneToVcal()"/>
        	</xsl:for-each>

		<xsl:value-of select="common:AllTZToOpensync()"/>

		<!-- Opensync 0.3 -  Alarm is common with events -->

	        <xsl:for-each select="T:Reminder[position() = 1]">
                	<Alarm>
                	        <xsl:attribute name="Value">DURATION</xsl:attribute>
                        	<AlarmAction>DISPLAY</AlarmAction>
	                        <AlarmDescription><xsl:value-of select="Subject"/></AlarmDescription>
        	                <AlarmTrigger><xsl:value-of select="common:AlarmFromAirsync()"/></AlarmTrigger>
                	</Alarm>
	        </xsl:for-each>

		<!-- Opensync 0.3x - 'Subject' is an easy conversion -->

		<xsl:for-each select="T:Subject[position() = 1]">
		    	<Summary>
				<Content>
					<xsl:value-of select="."/>
				</Content>
			</Summary>
		</xsl:for-each>

		<!-- Opensync 0.3x - UTCxxxxDate and xxxxDate need to be handled together
                     Note that the extension checks for the presence of UtcDueDate and
                     UtcStartDate -->

               	<xsl:for-each select="T:DueDate[position() = 1]">
                    <Due><xsl:value-of select="task:DateFromAirsync()"/></Due>
        	</xsl:for-each>

               	<xsl:for-each select="T:StartDate[position() = 1]">
                    <DateStarted><xsl:value-of select="task:DateFromAirsync()"/></DateStarted>
                </xsl:for-each>

                <!-- OpenSync 0.3, same mechanism as events -->

                <xsl:for-each select="T:Sensitivity[position() = 1]">
                    <Class><Content><xsl:value-of select="common:ClassFromAirsync()"/></Content></Class>
                </xsl:for-each>

                <!-- OpenSync 0.3 - Categories remain the same -->

                <xsl:if test="count(T:Categories) &gt; 0">
                    <Categories>
                        <xsl:for-each select="T:Categories">
                            <xsl:for-each select="T:Category">
                                <Category><xsl:value-of select="."/></Category>
                            </xsl:for-each>
                        </xsl:for-each>
                    </Categories>
                </xsl:if>

		<!-- Opensync 0.3x - Completion is synced with limits' -->

		<xsl:for-each select="T:Complete">
			<xsl:value-of select="task:StatusFromAirsync()"/>
		</xsl:for-each>

		<!-- Opensync 0.3x - Priority handled similarly -->

		<xsl:for-each select="T:Importance">
			<Priority>
				<Content><xsl:value-of select="task:PriorityFromAirsync()"/></Content>
			</Priority>
		</xsl:for-each>
	
		<!-- Opensync 0.3x - Content handled similarly -->

		<xsl:for-each select="T:Rtf">
			<Description>
				<Content><xsl:value-of select="common:OSTextFromAirsyncRTF()"/></Content>
			</Description>
		</xsl:for-each>
	</todo>
</xsl:template>

</xsl:transform>
