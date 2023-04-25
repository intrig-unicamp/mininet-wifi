<?xml version="1.0" encoding="UTF-8"?>

<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform" >
	<xsl:output method="html" omit-xml-declaration="no" indent="yes"/>

	
	<xsl:template match="/">
		<html>
			<head>
				<link rel="stylesheet" type="text/css" href="netvm_instructionset.css"/> 
				<title>NetVm Instruction Set</title>
			</head>
			<body>
				<h1>NetVM Instruction Set</h1>

<p>This document lists all the instructions present in the NetVM instruction 
set, grouped according to their family. Each instruction may have different 
variants, which are listed in the same section of the &quot;base&quot; instruction.</p>
<h4>Legend</h4>
<ul>
	<li><b>Description</b>: it present the general behavior of the instruction.</li>
	<li><b>Exceptions</b>: it lists the conditions that makes the instruction failing.</li>
	<li><b>Stack transitions</b>: it shows which parameters must be on the stack before 
calling the function, and how the stack is modified after calling the function itself. 
For instance, <code>value1, value2 --&gt; result</code> means that two parameters must be 
present on the stack, which are replaced by a single value <code>result</code> when the 
function terminates; hence this function reduces the the stack of one position.
Please note that in this notation <code>value1</code> is the top of the stack.</li>
</ul>
<p>With respect operands, the general rule is that the first operand is always the first
inserted in the stack, while the last operand is the last one inserted. In other words, the
last operand is always the top of the stack.</p>


				<xsl:apply-templates/>
			</body>
		</html>
	</xsl:template>
	
	
	<xsl:template match="Section">
		<h2><xsl:value-of select="@Name" /></h2>
		<xsl:for-each select=".">
			<xsl:apply-templates select="Instruction"/>
		</xsl:for-each>
	</xsl:template>

	<xsl:template match="Instruction">
		<table id="TableInstruction" border="0" width="95%" cellspacing="0" cellpadding="3" bordercolor="#111111">
		<tr>
			<td>
				<table border="0" width="100%" cellspacing="0" cellpadding="5">
				<tr>
					<th id="ItemName">
						<code><xsl:value-of select="@Name" /></code>
					</th>
				</tr>
				</table>
			</td>
		</tr>
		<tr>
			<td>
				<table border="0" width="100%" cellspacing="0" cellpadding="3">
				<tr>
					<td width="20%">
						<font id="ItemDescription">Description:</font>
					</td>
					<td>
						<xsl:value-of select="Description" />
					</td>
				</tr>
				<tr>
					<td width="20%">
						<font id="ItemDescription">Exceptions:</font>
					</td>
					<td>
						<xsl:value-of select="Exception" />
					</td>
				</tr>
				<tr>
					<td width="20%">
						<font id="ItemDescription">Stack Transitions:</font>
					</td>
					<td>
						<code> <xsl:value-of select="StackTransition" /> </code>
					</td>
				</tr>
				</table>
			</td>
		</tr>
		<tr>
			<td>
				<table border="1"  width="100%" cellspacing="0" cellpadding="3">
					<xsl:for-each select=".">
						<xsl:apply-templates select="Variants"/>
					</xsl:for-each>
				</table>
			</td>
		</tr>
		</table>
		<br/>
	</xsl:template>
	
	<xsl:template match="Variants">
		<tr>
			<th width="100%" colspan="3">
				Variants
			</th>
		</tr>
		<xsl:for-each select=".">
			<xsl:apply-templates select="Variant"/>
		</xsl:for-each>
	</xsl:template>

	<xsl:template match="Variant">
		<tr>
			<td width="10%">
				<code><xsl:value-of select="Signature"/></code>
			</td>
			<td width="20%">
				<code><xsl:value-of select="Parameters"/></code>
			</td>
			<td width="65%">
				<xsl:value-of select="Details"/>
			</td>
		</tr>
	</xsl:template>

</xsl:stylesheet>
