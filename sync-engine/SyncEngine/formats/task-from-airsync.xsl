<?xml version="1.0" encoding="UTF-8"?>

<xsl:transform version="1.0"
               xmlns:xsl="http://www.w3.org/1999/XSL/Transform"
               xmlns:convert="http://synce.org/convert"
               xmlns:tz="http://synce.org/tz"
               xmlns:AS="http://synce.org/formats/airsync_wm5/airsync"
               xmlns:T="http://synce.org/formats/airsync_wm5/tasks"
               exclude-result-prefixes="convert T AS">

    <xsl:template match="ApplicationData | AS:ApplicationData">
        <vcal>
            <xsl:if test="T:Timezone">
                <xsl:value-of select="tz:ConvertASTimezoneToVcal()"/>
            </xsl:if>

            <Todo>
                <xsl:apply-templates/>
            </Todo>
        </vcal>
    </xsl:template>

    <xsl:template match="T:Reminder">
        <Alarm>
            <AlarmTrigger>
                <Content><xsl:value-of select="convert:event_reminder_from_airsync()"/></Content>
                <Value>DURATION</Value>
                <Related>START</Related>
            </AlarmTrigger>
            <AlarmAction>DISPLAY</AlarmAction>
            <AlarmDescription><xsl:value-of select="../T:Subject"/></AlarmDescription>
        </Alarm>
    </xsl:template>

    <xsl:template match="T:Subject">
           <Summary><Content><xsl:value-of select="."/></Content></Summary>
    </xsl:template>

    <xsl:template match="T:DueDate">
            <DateDue><xsl:value-of select="convert:task_due_date_from_airsync()"/></DateDue>
    </xsl:template>

    <xsl:template match="T:StartDate">
            <DateStarted><xsl:value-of select="convert:task_start_date_from_airsync()"/></DateStarted>
    </xsl:template>

    <xsl:template match="T:Sensitivity">
        <Class><Content>
            <xsl:choose>
                <xsl:when test="string(.) = '2'">
                    <xsl:text>PRIVATE</xsl:text>
                </xsl:when>
                <xsl:when test="string(.) = '3'">
                    <xsl:text>CONFIDENTIAL</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                    <xsl:text>PUBLIC</xsl:text> <!-- 'PUBLIC' is our default value -->
                </xsl:otherwise>
            </xsl:choose>
        </Content></Class>
    </xsl:template>

    <xsl:template match="T:Categories">
        <Categories>
            <xsl:apply-templates />
        </Categories>
    </xsl:template>

    <xsl:template match="T:Category">
        <Category><xsl:value-of select="."/></Category>
    </xsl:template>

    <xsl:template match="T:Complete">
        <!--
            We only sync the 'COMPLETED' state here. Evo2 maintains a number of 
            different status values for various states of a job except COMPLETED
            and we don't want to clobber these. AirStink seems only to maintain
            the two states: Not Completed and Completed. However, we force 
            the PercentComplete field to 100 if the task is marked as completed
        -->
        <xsl:if test="string(.) = '1'">
            <Status><Content>COMPLETED</Content></Status>
            <PercentComplete><Content>100</Content></PercentComplete>
        </xsl:if>
    </xsl:template>

    <xsl:template match="T:Importance">
        <Priority><Content>
            <xsl:choose>
                <xsl:when test="string(.) = '0'">
                    <xsl:text>7</xsl:text>
                </xsl:when>
                <xsl:when test="string(.) = '1'">
                    <xsl:text>5</xsl:text>
                </xsl:when>
                <xsl:when test="string(.) = '2'">
                    <xsl:text>3</xsl:text>
                </xsl:when>
                <xsl:otherwise>
                <!-- We can use the unspecced one here if we get such an one from Airsync -->
                    <xsl:text>0</xsl:text>
                </xsl:otherwise>
            </xsl:choose>
        </Content></Priority>
    </xsl:template>

    <xsl:template match="T:Rtf">
        <Description><Content><xsl:value-of select="convert:all_description_from_airsync()"/></Content></Description>
    </xsl:template>

    <xsl:template match="*">
        <!-- <xsl:message>Ignored element <xsl:value-of select="name()"/></xsl:message> -->
    </xsl:template>

</xsl:transform>
