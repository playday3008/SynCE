<?xml version="1.0" encoding="UTF-8"?>

<!-- Adapted for Opensync 0.3x -->

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:common="http://synce.org/common"
               xmlns:task="http://synce.org/task"
               xmlns:tz="http://synce.org/tz"
               exclude-result-prefixes="common task tz">

<xsl:template match="/todo">

    <AS:ApplicationData xmlns:AS="http://synce.org/formats/airsync_wm5/airsync" xmlns="http://synce.org/formats/airsync_wm5/tasks">

        <!-- AirSync tasks do not use a timezone. Instead we need to maintain two time fields for each
             timed entity: StartDate and UTCStartDate, also DueDate and UTCDueDate. How brain-damaged
             is this... 
             However, we retain the timezone read code here as there is the possibility that
             timezones may be supported in future -->

        <!-- Timezones are way different in OpenSync 0.30. We need to capture the likelihood of a 
             timezone in an Opensync task and use this to compute UTCxxxxDate. However, we do not
             export a timezone to Airsync. We gather together all timezone-specific entries before
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

        <!-- OpenSync 0.3 schema allows unbounded number of 'Alarm' elements 
             TODO: Wrong. Will update -->

        <xsl:for-each select="Alarm[position() = 1]">
	        <Reminder><xsl:value-of select="common:AlarmToAirsync()"/></Reminder>
        </xsl:for-each>


        <!-- Opensync 0.3x - Content -->

	<xsl:for-each select="Status/Content[position() = 1]">
		<Complete><xsl:value-of select = "task:StatusToAirsync()"/></Complete>
	</xsl:for-each>

        <!-- Opensync 0.3x -DateDue and DateStarted both require UTC conversion. 
             Note that Airsync chokes if StartDate is present without DueDate - this
             is quite legal for OS. DueDate without StartDate seems OK. -->

        <xsl:for-each select="Due[position() = 1]">
                <DueDate><xsl:value-of select="task:DateToAirsyncLocal()"/></DueDate>
                <UtcDueDate><xsl:value-of select="task:DateToAirsyncUTC()"/></UtcDueDate>
	</xsl:for-each>
	<xsl:if test="count(Due) = 0">
		<xsl:for-each select="DateStarted[position() = 1]">
			<DueDate><xsl:value-of select="task:DateToAirsyncLocal()"/></DueDate>
			<UtcDueDate><xsl:value-of select="task:DateToAirsyncUTC()"/></UtcDueDate>
		</xsl:for-each>
	</xsl:if>

        <xsl:for-each select="DateStarted[position() = 1]">
                <StartDate><xsl:value-of select="task:DateToAirsyncLocal()"/></StartDate>
                <UtcStartDate><xsl:value-of select="task:DateToAirsyncUTC()"/></UtcStartDate>
	</xsl:for-each>

	<!-- Opensync 0.3x - Priority conversion similar -->

	<xsl:for-each select="Priority/Content[position() = 1]">
		<Importance><xsl:value-of select = "task:PriorityToAirsync()"/></Importance>
	</xsl:for-each>

        <!-- Opensync 0.3x - Recurrence rule is unbounded -->

        <xsl:for-each select="RecurrenceRule">
            <Recurrence>
                <xsl:value-of select="common:RecurrenceRuleToAirsync()"/>
            </Recurrence>
        </xsl:for-each>

	<!-- Opensync 0.3x - Class is the same -->

        <xsl:for-each select="Class/Content[position() = 1]">
		<Sensitivity><xsl:value-of select="common:ClassToAirsync()"/></Sensitivity>
	</xsl:for-each>

        <!-- OpenSync 0.3 - Categories remain the same -->

        <xsl:if test="count(Category) &gt; 0 or count(Categories) &gt; 0">
        	<Categories>
        		<xsl:for-each select="Categories">
            			<xsl:for-each select="Category">
                            		<Category><xsl:value-of select="."/></Category>
                        	</xsl:for-each>
                    	</xsl:for-each>
                </Categories>
        </xsl:if>

	<!-- Opensync 0.3x - Summary and Description handled similar to events -->

	<xsl:for-each select="Summary[position()=1]">
		<Subject><xsl:value-of select="Content"/></Subject>
        </xsl:for-each>
	
	<xsl:for-each select="Description/Content[position() = 1]">
		<Rtf><xsl:value-of select="common:OSTextToAirsyncRTF()"/></Rtf>
	</xsl:for-each>

    </AS:ApplicationData>

</xsl:template>

</xsl:transform>
