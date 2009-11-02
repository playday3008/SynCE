<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:tz="http://synce.org/tz"
               exclude-result-prefixes="convert tz"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               xmlns:T="http://synce.org/formats/airsync_wm5/tasks">

<xsl:template match="/vcal">

    <AS:ApplicationData>

        <!-- AirSync tasks do not use a timezone. Instead we need to maintain two time fields for each
             timed entity: StartDate and UTCStartDate, also DueDate and UTCDueDate. How brain-damaged
             is this... -->

	<xsl:for-each select="Todo/Status/Content[position() = 1]">
		<T:Complete><xsl:value-of select = "convert:task_status_to_airsync()"/></T:Complete>
	</xsl:for-each>

        <xsl:for-each select="Todo/DateDue[position() = 1]">
		<xsl:value-of select="convert:task_due_date_to_airsync()"/>
	</xsl:for-each>

	<xsl:for-each select="Todo/Priority/Content[position() = 1]">
		<T:Importance><xsl:value-of select = "convert:task_prio_to_airsync()"/></T:Importance>
	</xsl:for-each>

        <xsl:for-each select="Todo/RecurrenceRule[position() = 1]">
            <T:Recurrence>
                <xsl:value-of select="convert:event_recurrence_to_airsync()"/>
            </T:Recurrence>
        </xsl:for-each>

        <xsl:for-each select="Todo/Class/Content[position() = 1]">
		<T:Sensitivity><xsl:value-of select="convert:task_classification_to_airsync()"/></T:Sensitivity>
	</xsl:for-each>

        <xsl:for-each select="Todo/DateStarted[position() = 1]">
		<xsl:value-of select="convert:task_start_date_to_airsync()"/>
	</xsl:for-each>

        <T:Categories>
               <xsl:for-each select="Todo/Categories">
                   <xsl:for-each select="Category">
                       <T:Category><xsl:value-of select="."/></T:Category>
                   </xsl:for-each>
               </xsl:for-each>
        </T:Categories>

        <xsl:for-each select="Todo/Class/Content[position() = 1]">
		<T:Sensitivity>
            <xsl:choose>
                <xsl:when test="Content = 'PRIVATE'">
                    <xsl:text>2</xsl:text>
                </xsl:when>
                <xsl:when test="Content = 'CONFIDENTIAL'">
                    <xsl:text>3</xsl:text>
                </xsl:when>
                <xsl:otherwise> <!-- "PUBLIC" is our default value -->
                    <xsl:text>0</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </T:Sensitivity>
	</xsl:for-each>

	<T:Subject><xsl:value-of select="Todo/Summary/Content"/></T:Subject>
	
	<xsl:for-each select="Todo/Description">
		<T:Rtf><xsl:value-of select="convert:all_description_to_airsync()"/></T:Rtf>
	</xsl:for-each>

    </AS:ApplicationData>

</xsl:template>

</xsl:transform>
