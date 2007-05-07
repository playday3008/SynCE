<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:tz="http://synce.org/tz"
               exclude-result-prefixes="convert tz">

<xsl:template match="/vcal">

    <AS:ApplicationData xmlns:AS="http://synce.org/formats/airsync_wm5/airsync" xmlns="http://synce.org/formats/airsync_wm5/tasks">

        <!-- AirSync tasks do not use a timezone. Instead we need to maintain two time fields for each
             timed entity: StartDate and UTCStartDate, also DueDate and UTCDueDate. How brain-damaged
             is this... -->

	<xsl:for-each select="Todo/Status/Content[position() = 1]">
		<Complete><xsl:value-of select = "convert:task_status_to_airsync()"/></Complete>
	</xsl:for-each>

        <xsl:for-each select="Todo/DateDue[position() = 1]">
		<xsl:value-of select="convert:task_due_date_to_airsync()"/>
	</xsl:for-each>

	<xsl:for-each select="Todo/Priority/Content[position() = 1]">
		<Importance><xsl:value-of select = "convert:task_prio_to_airsync()"/></Importance>
	</xsl:for-each>

        <xsl:for-each select="Todo/RecurrenceRule[position() = 1]">
            <Recurrence>
                <xsl:value-of select="convert:event_recurrence_to_airsync()"/>
            </Recurrence>
        </xsl:for-each>

        <xsl:for-each select="Todo/Class/Content[position() = 1]">
		<Sensitivity><xsl:value-of select="convert:task_classification_to_airsync()"/></Sensitivity>
	</xsl:for-each>

        <xsl:for-each select="Todo/DateStarted[position() = 1]">
		<xsl:value-of select="convert:task_start_date_to_airsync()"/>
	</xsl:for-each>

        <Categories>
               <xsl:for-each select="Todo/Categories">
                   <xsl:for-each select="Category">
                       <Category><xsl:value-of select="."/></Category>
                   </xsl:for-each>
               </xsl:for-each>
        </Categories>

        <xsl:for-each select="Todo/Class/Content[position() = 1]">
		<Sensitivity><xsl:value-of select="convert:event_sensitivity_to_airsync()"/></Sensitivity>
	</xsl:for-each>

	<Subject><xsl:value-of select="Todo/Summary/Content"/></Subject>
	
	<xsl:for-each select="Todo/Description/Content[position() = 1]">
		<Rtf><xsl:value-of select="convert:all_description_to_airsync()"/></Rtf>
	</xsl:for-each>

    </AS:ApplicationData>

</xsl:template>

</xsl:transform>
