/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/



/*!
	\file nbprotodb_defs.h

	This file lists the XML attributes that are defined in the NetPDL language.
*/


// Allow including this file only once
#pragma once





//! Maximum string length allowed in this NetPDL engine (used for messages and such).
#define NETPDL_MAX_STRING 2048



/*********************************************************/
/*  NetPDL Definitions                                   */
/*********************************************************/

// NetPDL run-time variables
//

//! NetPDL variable 'linktype'
#define NETPDL_VARIABLE_LINKTYPE					"$linklayer"
//! NetPDL variable 'framelength' (length of the frame at L2)
#define NETPDL_VARIABLE_FRAMELENGTH					"$framelength"
//! NetPDL variable 'packetlength' (length of the packet at L3, excluding L2 headers)
#define NETPDL_VARIABLE_PACKETLENGTH				"$packetlength"
//! NetPDL variable 'currentoffset'
#define NETPDL_VARIABLE_CURRENTOFFSET				"$currentoffset"
//! NetPDL variable 'currentprotooffset'
#define NETPDL_VARIABLE_CURRENTPROTOOFFSET			"$currentprotooffset"
//! NetPDL variable 'timestamp_sec'
#define NETPDL_VARIABLE_TIMESTAMP_SEC				"$timestamp_sec"
//! NetPDL variable 'timestamp_usec'
#define NETPDL_VARIABLE_TIMESTAMP_USEC				"$timestamp_usec"
//! NetPDL variable 'packet'
#define NETPDL_VARIABLE_PACKETBUFFER				"$packet"
//! NetPDL variable 'nextproto'
#define NETPDL_VARIABLE_NEXTPROTO					"$nextproto"
//! NetPDL variable 'prevproto'
#define NETPDL_VARIABLE_PREVPROTO					"$prevproto"
//! NetPDL variable 'show_networknames'
#define	NETPDL_VARIABLE_SHOWNETWORKNAMES 			"$show_networknames"
//! NetPDL variable 'track_L4sessions'
#define	NETPDL_VARIABLE_TRACKL4SESSIONS		 		"$track_L4sessions"
//! NetPDL variable 'enable_tentativeproto'
#define	NETPDL_VARIABLE_ENABLETENTATIVEPROTO		"$enable_tentativeproto"
//! NetPDL variable 'enable_protoverify'
#define	NETPDL_VARIABLE_ENABLEPROTOVERIFY		 	"$enable_protoverify"
//! NetPDL variable 'protoverify_result'
#define	NETPDL_VARIABLE_PROTOVERIFYRESULT			"$protoverify_result"
//! NetPDL variable 'token_begintlen'
#define	NETPDL_VARIABLE_TOKEN_BEGINTOKENLEN			"$token_begintlen"
//! NetPDL variable 'token_fieldlen'
#define	NETPDL_VARIABLE_TOKEN_FIELDLEN			 	"$token_fieldlen"
//! NetPDL variable 'token_endtlen'
#define	NETPDL_VARIABLE_TOKEN_ENDTOKENLEN			"$token_endtlen"


// TEMP FULVIO definizioni da mettere a posto
//
#define NETPDL_SHOWTEMPLATE_ATTR_NAME				"name"
#define NETPDL_FIELD_ATTR_SHOWTEMPLATE				"showtemplate"

#define NETPDL_SHOWSUMTEMPLATE_ATTR_NAME			"name"
#define NETPDL_PROTO_ATTR_SHOWSUMTEMPLATE			"showsumtemplate"



// NetPDL common syntax
#define NETPDL_COMMON_SYNTAX_SEP_FIELDS		"."
#define NETPDL_COMMON_SYNTAX_SEP_ADTREF		":"



// NetPDL common attributes
#define NETPDL_COMMON_ATTR_NAME			"name"
#define NETPDL_COMMON_ATTR_LONGNAME		"longname"
#define NETPDL_COMMON_ATTR_EXPR			"expr"
#define NETPDL_COMMON_ATTR_VALUE		"value"
#define NETPDL_COMMON_ATTR_WHEN			"when"
#define NETPDL_COMMON_ATTR_SIZE			"size"
#define NETPDL_COMMON_ATTR_CALLHANDLE	"callhandle"

#define NETPDL_CALLHANDLE_EVENT_BEFORE	"before"
#define NETPDL_CALLHANDLE_EVENT_AFTER	"after"

// NetPDL display attributes
#define NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT					"showtype"
	#define NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_BIN			"bin"
	#define NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_HEX			"hex"
	#define NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_HEXNOX		"hexnox"
	#define NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_ASC			"ascii"
	#define NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_DEC			"dec"
	#define NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_FLOAT		"float"
	#define NETPDL_DISPLAY_ATTR_DISPLAYDEFAULT_DOUBLE		"double"


#define NETPDL_DISPLAY_ATTR_DISPLAYGROUP					"showgrp"
#define NETPDL_DISPLAY_ATTR_DISPLAYSEPARATOR				"showsep"
#define NETPDL_DISPLAY_ATTR_CUSTOMPLUGIN					"showplg"

#define NETPDL_DISPLAY_ATTR_NATIVEFUNCTION					"showfast"
	#define NETPDL_DISPLAY_ATTR_NATIVEFUNCTION_IPV4			"ipv4"
	#define NETPDL_DISPLAY_ATTR_NATIVEFUNCTION_ASCII		"ascii"
	#define NETPDL_DISPLAY_ATTR_NATIVEFUNCTION_ASCIILINE	"asciiline"
	#define NETPDL_DISPLAY_ATTR_NATIVEFUNCTION_HTTPCONTENT	"httpcontent"



// Common attributes for fields
#define NETPDL_FIELDS_COMMON_ATTR_TYPE			"type"
#define NETPDL_FIELDS_COMMON_ATTR_PATTERN		"pattern"
#define NETPDL_FIELDS_COMMON_ATTR_BEGINREGEX	"beginregex"
#define NETPDL_FIELDS_COMMON_ATTR_ENDREGEX		"endregex"

#define NETPDL_FIELDS_COMMON_ATTR_MATCH			"match"
#define NETPDL_FIELDS_COMMON_ATTR_RECURRING		"recurring"
#define NETPDL_FIELDS_COMMON_ATTR_RECURRING_YES	"yes"

//// TEMP FABIO 27/06/2008: supporto encoding sospeso
#define NETPDL_FIELDS_COMMON_ATTR_ENCODING		"encoding"
#define NETPDL_FIELDS_COMMON_ATTR_ENCODING_ASCII	"ascii"
#define NETPDL_FIELDS_COMMON_ATTR_ENCODING_UTF8		"utf-8"
#define NETPDL_FIELDS_COMMON_ATTR_ENCODING_UNICODE	"unicode"
#define NETPDL_FIELDS_COMMON_ATTR_ENCODING_BER		"ber"
#define NETPDL_FIELDS_COMMON_ATTR_ENCODING_CER		"cer"
#define NETPDL_FIELDS_COMMON_ATTR_ENCODING_DER		"der"



// Simple fields
//
#define NETPDL_FIELD_ATTR_TYPE			"type"

#define NETPDL_FIELD_ATTR_TYPE_FIXED		"fixed"
#define NETPDL_FIELD_ATTR_TYPE_BIT			"bit"
#define NETPDL_FIELD_ATTR_TYPE_BIT_EX		""
#define NETPDL_FIELD_ATTR_TYPE_VARIABLE		"variable"
#define NETPDL_FIELD_ATTR_TYPE_TOKENENDED	"tokenended"
#define NETPDL_FIELD_ATTR_TYPE_TOKENWRAPPED	"tokenwrapped"
#define NETPDL_FIELD_ATTR_TYPE_LINE			"line"
#define NETPDL_FIELD_ATTR_TYPE_PATTERN		"pattern"
#define NETPDL_FIELD_ATTR_TYPE_EATALL		"eatall"
#define NETPDL_FIELD_ATTR_TYPE_PADDING		"padding"
#define NETPDL_FIELD_ATTR_TYPE_PLUGIN		"plugin"

#define NETPDL_FIELD_ATTR_MASK				"mask"
#define NETPDL_FIELD_ATTR_BEGINTOKEN		"begintoken"
#define NETPDL_FIELD_ATTR_ENDTOKEN			"endtoken"
#define NETPDL_FIELD_ATTR_BEGINREGEX		"beginregex"
#define NETPDL_FIELD_ATTR_ENDREGEX			"endregex"
#define NETPDL_FIELD_ATTR_BEGINOFFSET		"beginoffset"
#define NETPDL_FIELD_ATTR_ENDOFFSET			"endoffset"
#define NETPDL_FIELD_ATTR_ENDDISCARD		"enddiscard"
#define NETPDL_FIELD_ATTR_ALIGN				"align"
#define NETPDL_FIELD_ATTR_PLUGINNAME		"plugin"


#define NETPDL_FIELD_ATTR_BIGENDIAN			"bigendian"
#define NETPDL_FIELD_ATTR_BIGENDIAN_YES		"yes"



// Complex fields
//
#define NETPDL_CFIELD_ATTR_TYPE			"type"

#define NETPDL_CFIELD_ATTR_TYPE_TLV			"tlv"
#define NETPDL_CFIELD_ATTR_TYPE_DELIMITED	"delimited"
#define NETPDL_CFIELD_ATTR_TYPE_ELINE		"e-line"
#define NETPDL_CFIELD_ATTR_TYPE_HDRLINE		"hdrline"
#define NETPDL_CFIELD_ATTR_TYPE_DYNAMIC		"dynamic"
#define NETPDL_CFIELD_ATTR_TYPE_ASN1		"asn1"
#define NETPDL_CFIELD_ATTR_TYPE_XML			"xml"

#define NETPDL_CFIELD_ATTR_TSIZE			"tsize"
#define NETPDL_CFIELD_ATTR_LSIZE			"lsize"
#define NETPDL_CFIELD_ATTR_VEXPR			"vexpr"
#define NETPDL_CFIELD_ATTR_SEPREGEX			"sepregex"

#define NETPDL_CFIELD_ATTR_ONPARTIALMATCH		"onpartialmatch"
#define NETPDL_CFIELD_ATTR_ONMISSINGBEGIN		"onmissingbegin"
#define NETPDL_CFIELD_ATTR_ONMISSINGEND			"onmissingend"
#define NETPDL_CFIELD_ATTR_ONEVENT_SKIPFIELD		"skipfield"
#define NETPDL_CFIELD_ATTR_ONEVENT_CONTINUE			"continue"



// Mappings
//
#define NETPDL_MAP_ATTR_REFNAME					"refname"

#define NETPDL_MAP_ATTR_TYPE_XML_PI				"xml.pi"
#define NETPDL_MAP_ATTR_TYPE_XML_DOCTYPE		"xml.doctype"
#define NETPDL_MAP_ATTR_TYPE_XML_ELEMENT		"xml.element"

#define NETPDL_MAP_ATTR_NAMESPACE				"namespace"
#define NETPDL_MAP_ATTR_HIERARCY				"hierarcy"
#define NETPDL_MAP_ATTR_ATTSVIEW				"attrview"
#define NETPDL_MAP_ATTR_ATTSVIEW_YES			"yes"



// Subfields
//
#define NETPDL_SUBFIELD_ATTR_PORTION		"portion"
#define NETPDL_CSUBFIELD_ATTR_SUBTYPE		"subtype"

#define NETPDL_SUBFIELD_ATTR_PORTION_TLV_TYPE		"tlv.type"
#define NETPDL_SUBFIELD_ATTR_PORTION_TLV_LENGTH		"tlv.length"
#define NETPDL_SUBFIELD_ATTR_PORTION_TLV_VALUE		"tlv.value"
#define NETPDL_SUBFIELD_ATTR_PORTION_HDRLINE_HNAME	"hdrline.hname"
#define NETPDL_SUBFIELD_ATTR_PORTION_HDRLINE_HVALUE	"hdrline.hvalue"
#define NETPDL_SUBFIELD_ATTR_PORTION_DYNAMIC_		"dynamic."

const char *const NETPDL_FIELDBASE_NAME_TRIVIAL	= "__trivial__";
const char *const NETPDL_FIELD_NAME_ITEM	= "item";
const char *const NETPDL_CFIELD_NAME_TLV		= "tlv";
const char *const NETPDL_CFIELD_NAME_DELIMITED	= "delimited";
const char *const NETPDL_CFIELD_NAME_LINE		= "line";
const char *const NETPDL_CFIELD_NAME_HDRLINE	= "hdrline";
const char *const NETPDL_CFIELD_NAME_DYNAMIC	= "dynamic";
const char *const NETPDL_CFIELD_NAME_ASN1		= "asn1";
const char *const NETPDL_CFIELD_NAME_XML		= "xml";
const char *const NETPDL_SUBFIELD_NAME_TLV_TYPE			= "type";
const char *const NETPDL_SUBFIELD_NAME_TLV_LENGTH		= "length";
const char *const NETPDL_SUBFIELD_NAME_TLV_VALUE		= "value";
const char *const NETPDL_SUBFIELD_NAME_HDRLINE_HNAME	= "hname";
const char *const NETPDL_SUBFIELD_NAME_HDRLINE_HVALUE	= "hvalue";
const char *const NETPDL_SUBFIELD_NAME_ASN1_ENCRULEDATA	= "encruledata";
const char *const NETPDL_SUBFIELD_NAME_XML_PROLOG		= "prolog";
const char *const NETPDL_SUBFIELD_NAME_XML_BODY			= "body";



// ADTs
//
#define NETPDL_ADT_ATTR_ADTTYPE			"adttype"
#define NETPDL_ADT_ATTR_ADTNAME			"adtname"
#define NETPDL_ADT_ATTR_BASEADT			"baseadt"
//// TEMP FABIO 27/06/2008: abilitare se si vuole adottare la tecnica
////   del namespacing per l'elemento <REPLACE>
////#define NETPDL_REPLACE_ATTR_ADTREF		"adtref"
#define NETPDL_REPLACE_ATTR_NAMEREF		"nameref"




// NetPDL Loops
//
#define NETPDL_FIELD_LOOP_ATTR_LOOPTYPE					"type"
	#define NETPDL_FIELD_LOOP_LPT_SIZE					"size"
	#define NETPDL_FIELD_LOOP_LTP_T2R					"times2repeat"
	#define NETPDL_FIELD_LOOP_LTP_WHILE					"while"
	#define NETPDL_FIELD_LOOP_LTP_DOWHILE				"do-while"

	#define NETPDL_FIELD_LOOPCTRL_ATTR_TYPE				"type"
	#define NETPDL_FIELD_LOOPCTRL_ATTR_TYPE_BREAK		"break"
	#define NETPDL_FIELD_LOOPCTRL_ATTR_TYPE_CONTINUE	"continue"

#define NETPDL_FIELD_INCLUDEBLK_NAME					"name"




// NetPDL global definitions
//
#define NETPDL_PROTOCOL_STARTPROTO			"startproto"
#define NETPDL_PROTOCOL_DEFAULTPROTO		"defaultproto"
#define NETPDL_PROTOCOL_ETHERPADDINGPROTO	"etherpadding"



// Switch - case related definitions
//
#define NETPDL_FIELD_SWITCH_ATTR_CASESENSITIVE		"casesensitive"
#define NETPDL_FIELD_SWITCH_ATTR_CASESENSITIVE_NO	"no"
#define NETPDL_CASE_ATTR_VALUE						"value"
#define NETPDL_CASE_ATTR_MAXVALUE					"maxvalue"
#define NETPDL_CASE_ATTR_SHOW						"show"



// NetPDL nextproto attributes
#define NETPDL_NEXTPROTO_ATTR_PROTO							"proto"
#define NETPDL_NEXTPROTO_ATTR_PREFERRED						"preferred" //[icerrato]


#define NETPDL_VARIABLE_ATTR_VALIDITY						"validity"
#define NETPDL_VARIABLE_ATTR_VALIDITY_THISPKT				"thispacket"
#define NETPDL_VARIABLE_ATTR_VALIDITY_STATIC				"static"
#define NETPDL_VARIABLE_ATTR_VARTYPE						"type"
#define NETPDL_VARIABLE_ATTR_VARTYPE_NUMBER					"number"
#define NETPDL_VARIABLE_ATTR_VARTYPE_BUFFER					"buffer"
#define NETPDL_VARIABLE_ATTR_VARTYPE_REFBUFFER				"refbuffer"
#define NETPDL_VARIABLE_ATTR_VARTYPE_PROTOCOL				"protocol"


// NetPDL Var and LookupTable
//
#define NETPDL_LOOKUPTABLE_KEYANDDATA_ATTR_TYPE				"type"
#define NETPDL_LOOKUPTABLE_KEYANDDATA_ATTR_TYPE_NUMBER		"number"
#define NETPDL_LOOKUPTABLE_KEYANDDATA_ATTR_TYPE_BUFFER		"buffer"
#define NETPDL_LOOKUPTABLE_KEYANDDATA_ATTR_TYPE_PROTOCOL	"protocol"

#define NETPDL_LOOKUPTABLE_ATTR_EXACTENTRIES				"exactentries"
#define NETPDL_LOOKUPTABLE_ATTR_MASKENTRIES					"maskentries"
#define NETPDL_LOOKUPTABLE_ATTR_VALIDITY					"validity"
#define NETPDL_LOOKUPTABLE_ATTR_VALIDITY_STATIC				"static"
#define NETPDL_LOOKUPTABLE_ATTR_VALIDITY_DYNAMIC			"dynamic"


#define NETPDL_UPDATELOOKUPTABLE_ATTR_ACTION							"action"
	#define NETPDL_UPDATELOOKUPTABLE_ATTR_ACTION_ADD					"add"
	#define NETPDL_UPDATELOOKUPTABLE_ATTR_ACTION_PURGE					"purge"
	#define NETPDL_UPDATELOOKUPTABLE_ATTR_ACTION_OBSOLETE				"obsolete"

#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY						"validity"
	#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_KEEPFOREVER		"keepforever"
	#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_KEEPMAXTIME		"keepmaxtime"
	#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_UPDATEONHIT		"updateonhit"
	#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_REPLACEONHIT		"replaceonhit"
	#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_VALIDITY_ADDONHIT			"addonhit"

#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_KEEPTIME					"keeptime"
#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_HITTIME					"hittime"
#define NETPDL_UPDATELOOKUPTABLE_ADD_ATTR_NEWHITTIME				"newhittime"
#define NETPDL_LOOKUPTABLE_FIELDNAME_TIMESTAMP						"timestamp"
#define NETPDL_LOOKUPTABLE_FIELDNAME_LIFETIME						"lifetime"


#define NETPDL_LOOKUPTABLE_KEYFIELD_ATTR_MASK					"mask"


#define NETPDL_ALIAS_ATTR_REPLACEWITH							"replacewith"


// Warning: these numbers cannot change, since are part of the NetPDL language definition
#define NETPDL_PROTOVERIFYRESULT_NOTFOUND				0
#define NETPDL_PROTOVERIFYRESULT_FOUND					1
#define NETPDL_PROTOVERIFYRESULT_CANDIDATE				2
#define NETPDL_PROTOVERIFYRESULT_DEFERRED				3


// Expressions
#define NETPDL_EXPR_THISFIELD			"this"


// Definitions related the the "visualization extension" section of the NetPDL
//
#define NETPDL_SHOW_ATTR_NAME							"name"
#define NETPDL_SHOW_ATTR_NAME_NEXT						"next"
#define NETPDL_SHOW_ATTR_SHOWDATA						"showdata"

#define NETPDL_SHOW_TEXT_ATTR_VALUE						"value"


// Definitions related to the structure of the capture index
//
#define NETPDL_SHOWSUMSECT_ATTR_LONGNAME				"longname"








/*********************************************************/
/*  NetPDL - PSML - PDML file header attributes          */
/*********************************************************/

#define NETPDL_DATE_ATTR		"date"
#define NETPDL_CREATOR_ATTR		"creator"
#define NETPDL_VERSION_ATTR		"version"

#define PxML_CREATOR "creator=\"NetBee Library\""
#define PxML_VERSION "version=\"0\""



/*********************************************************/
/*  Capture Summary Markup Language Definitions (PSML)   */
/*********************************************************/

// CPSMLMaker works only with ascii, so we do not use the _TEXT macro here
#define PSML_ROOT				"psml"
#define PSML_INDEXSTRUCTURE		"structure"
#define PSML_PACKET				"packet"
#define PSML_SECTION			"section"






/*********************************************************/
/*  PDML Definitions                                   */
/*********************************************************/

#define PDML_CAPTURE		"pdml"
#define PDML_PACKET			"packet"
#define PDML_FIELD			"field"
#define PDML_BLOCK			"block"
#define PDML_PROTO			"proto"
#define PDML_DUMP			"dump"

#define PDML_FIELD_ATTR_POSITION				"pos"
#define PDML_FIELD_ATTR_VALUE					"value"
#define PDML_FIELD_ATTR_NAME					"name"
#define PDML_FIELD_ATTR_LONGNAME				"longname"
#define PDML_FIELD_ATTR_SHOW					"showvalue"
#define PDML_FIELD_ATTR_SHOWDTL					"showdtl"
#define PDML_FIELD_ATTR_SIZE					"size"
#define PDML_FIELD_ATTR_MASK					"mask"
#define PDML_FIELD_ATTR_SHOWMAP					"showmap"


#define PDML_FIELD_ATTR_NAME_TIMESTAMP		"timestamp"
#define PDML_FIELD_ATTR_NAME_CAPLENGTH		"caplen"
#define PDML_FIELD_ATTR_NAME_LENGTH			"len"
#define PDML_FIELD_ATTR_NAME_NUM			"num"



