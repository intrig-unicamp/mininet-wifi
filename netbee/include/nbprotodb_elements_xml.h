/*****************************************************************************/
/*                                                                           */
/* Copyright notice: please read file license.txt in the NetBee root folder. */
/*                                                                           */
/*****************************************************************************/


/*!
	\file nbprotodb_elements_xml.h

	This file lists the XML elements that are defined in the NetPDL language.
*/

// Syntax for NetPDL element names
// nbNETPDL_ELEMENT(ElementName, ElementStringID, ElementCodeID, PtrToAllocationFunction, PtrToCrossLinkFunction PtrToCleanupFunction, PtrToSerializeFunction)

nbNETPDL_ELEMENT("netpdl",               nbNETPDL_EL_NETPDL,              nbNETPDL_IDEL_NETPDL,              CreateElementNetPDL, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("protocol",             nbNETPDL_EL_PROTO,               nbNETPDL_IDEL_PROTO,               CreateElementProto,   OrganizeElementProto, DeleteElementProto,   SerializeElementProto)

nbNETPDL_ELEMENT("adt-list",             nbNETPDL_EL_ADTLIST,             nbNETPDL_IDEL_ADTLIST,             CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("adt",                  nbNETPDL_EL_ADT,                 nbNETPDL_IDEL_ADT,                 CreateElementAdt, OrganizeElementAdt, DeleteElementAdt, SerializeElementGeneric)
nbNETPDL_ELEMENT("replace",              nbNETPDL_EL_REPLACE,             nbNETPDL_IDEL_REPLACE,             CreateElementReplace, OrganizeElementReplace, DeleteElementField, SerializeElementGeneric)

nbNETPDL_ELEMENT("format",               nbNETPDL_EL_FORMAT,              nbNETPDL_IDEL_FORMAT,              CreateElementGeneric,   OrganizeElementGeneric, DeleteElementGeneric,   SerializeElementProto)
nbNETPDL_ELEMENT("encapsulation",        nbNETPDL_EL_ENCAPSULATION,       nbNETPDL_IDEL_ENCAPSULATION,       CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)

nbNETPDL_ELEMENT("nextproto",            nbNETPDL_EL_NEXTPROTO,           nbNETPDL_IDEL_NEXTPROTO,           CreateElementNextProto, OrganizeElementGeneric, DeleteElementNextProto, SerializeElementGeneric)
nbNETPDL_ELEMENT("nextproto-candidate",  nbNETPDL_EL_NEXTPROTOCANDIDATE,  nbNETPDL_IDEL_NEXTPROTOCANDIDATE,  CreateElementNextProto, OrganizeElementGeneric, DeleteElementNextProto, SerializeElementGeneric)

nbNETPDL_ELEMENT("execute-code",         nbNETPDL_EL_EXECUTECODE,         nbNETPDL_IDEL_EXECUTECODE,         CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("verify",               nbNETPDL_EL_VERIFY,              nbNETPDL_IDEL_VERIFY,              CreateElementExecuteX, OrganizeElementGeneric, DeleteElementExecuteX, SerializeElementGeneric)
nbNETPDL_ELEMENT("init",                 nbNETPDL_EL_INIT,                nbNETPDL_IDEL_INIT,                CreateElementExecuteX, OrganizeElementGeneric, DeleteElementExecuteX, SerializeElementGeneric)
nbNETPDL_ELEMENT("before",               nbNETPDL_EL_BEFORE,              nbNETPDL_IDEL_BEFORE,              CreateElementExecuteX, OrganizeElementGeneric, DeleteElementExecuteX, SerializeElementGeneric)
nbNETPDL_ELEMENT("after",                nbNETPDL_EL_AFTER,               nbNETPDL_IDEL_AFTER,               CreateElementExecuteX, OrganizeElementGeneric, DeleteElementExecuteX, SerializeElementGeneric)
nbNETPDL_ELEMENT("variable",             nbNETPDL_EL_VARIABLE,            nbNETPDL_IDEL_VARIABLE,            CreateElementVariable, OrganizeElementGeneric, DeleteElementVariable, SerializeElementGeneric)
nbNETPDL_ELEMENT("lookuptable",          nbNETPDL_EL_LOOKUPTABLE,         nbNETPDL_IDEL_LOOKUPTABLE,         CreateElementLookupTable, OrganizeElementGeneric, DeleteElementLookupTable, SerializeElementGeneric)
nbNETPDL_ELEMENT("key",                  nbNETPDL_EL_KEY,                 nbNETPDL_IDEL_KEY,                 CreateElementKeyData, OrganizeElementGeneric, DeleteElementKeyData, SerializeElementGeneric)
nbNETPDL_ELEMENT("data",                 nbNETPDL_EL_DATA,                nbNETPDL_IDEL_DATA,                CreateElementKeyData, OrganizeElementGeneric, DeleteElementKeyData, SerializeElementGeneric)
nbNETPDL_ELEMENT("alias",                nbNETPDL_EL_ALIAS,               nbNETPDL_IDEL_ALIAS,               CreateElementAlias, OrganizeElementGeneric, DeleteElementAlias, SerializeElementGeneric)

nbNETPDL_ELEMENT("fields",               nbNETPDL_EL_FIELDS,              nbNETPDL_IDEL_FIELDS,              CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("field",                nbNETPDL_EL_FIELD,               nbNETPDL_IDEL_FIELD,               CreateElementField, OrganizeElementField, DeleteElementField, SerializeElementGeneric)
nbNETPDL_ELEMENT("subfield",	         nbNETPDL_EL_SUBFIELD,            nbNETPDL_IDEL_SUBFIELD,            CreateElementSubfield, OrganizeElementSubfield, DeleteElementField, SerializeElementGeneric)
nbNETPDL_ELEMENT("fieldmatch",           nbNETPDL_EL_FIELDMATCH,          nbNETPDL_IDEL_FIELDMATCH,          CreateElementFieldmatch, OrganizeElementFieldmatch, DeleteElementField, SerializeElementGeneric)
nbNETPDL_ELEMENT("adtfield",             nbNETPDL_EL_ADTFIELD,            nbNETPDL_IDEL_ADTFIELD,            CreateElementAdtfield, OrganizeElementAdtfield, DeleteElementField, SerializeElementGeneric)
nbNETPDL_ELEMENT("map",                  nbNETPDL_EL_MAP,                 nbNETPDL_IDEL_MAP,                 CreateElementMap, OrganizeElementMap, DeleteElementMap, SerializeElementGeneric)
nbNETPDL_ELEMENT("set",				     nbNETPDL_EL_SET,                 nbNETPDL_IDEL_SET,                 CreateElementSet, OrganizeElementSet, DeleteElementSet, SerializeElementGeneric)
nbNETPDL_ELEMENT("exit-when",            nbNETPDL_EL_EXITWHEN,            nbNETPDL_IDEL_EXITWHEN,            CreateElementExitWhen, OrganizeElementGeneric, DeleteElementExitWhen, SerializeElementGeneric)
nbNETPDL_ELEMENT("default-item",         nbNETPDL_EL_DEFAULTITEM,         nbNETPDL_IDEL_DEFAULTITEM,         CreateElementDefaultItem, OrganizeElementDefaultItem, DeleteElementField, SerializeElementGeneric)
nbNETPDL_ELEMENT("choice",               nbNETPDL_EL_CHOICE,              nbNETPDL_IDEL_CHOICE,              CreateElementChoice, OrganizeElementChoice, DeleteElementChoice, SerializeElementGeneric)
nbNETPDL_ELEMENT("includeblk",           nbNETPDL_EL_INCLUDEBLK,          nbNETPDL_IDEL_INCLUDEBLK,          CreateElementIncludeBlk, OrganizeElementIncludeBlk, DeleteElementIncludeBlk, SerializeElementGeneric)
nbNETPDL_ELEMENT("block",                nbNETPDL_EL_BLOCK,               nbNETPDL_IDEL_BLOCK,               CreateElementBlock, OrganizeElementBlock, DeleteElementBlock, SerializeElementGeneric)

nbNETPDL_ELEMENT("assign-variable",	     nbNETPDL_EL_ASSIGNVARIABLE,      nbNETPDL_IDEL_ASSIGNVARIABLE,      CreateElementAssignVariable, OrganizeElementGeneric, DeleteElementAssignVariable, SerializeElementGeneric)
nbNETPDL_ELEMENT("assign-lookuptable",	 nbNETPDL_EL_ASSIGNLOOKUPTABLE,   nbNETPDL_IDEL_ASSIGNLOOKUPTABLE,   CreateElementAssignLookupTable, OrganizeElementGeneric, DeleteElementAssignLookupTable, SerializeElementGeneric)
nbNETPDL_ELEMENT("update-lookuptable",   nbNETPDL_EL_UPDATELOOKUPTABLE,   nbNETPDL_IDEL_UPDATELOOKUPTABLE,   CreateElementUpdateLookupTable, OrganizeElementUpdateLookupTable, DeleteElementUpdateLookupTable, SerializeElementGeneric)
nbNETPDL_ELEMENT("lookupkey",            nbNETPDL_EL_LOOKUPKEY,           nbNETPDL_IDEL_LOOKUPKEY,           CreateElementLookupKeyData, OrganizeElementGeneric, DeleteElementLookupKeyData, SerializeElementGeneric)
nbNETPDL_ELEMENT("lookupdata",           nbNETPDL_EL_LOOKUPDATA,          nbNETPDL_IDEL_LOOKUPDATA,          CreateElementLookupKeyData, OrganizeElementGeneric, DeleteElementLookupKeyData, SerializeElementGeneric)

nbNETPDL_ELEMENT("if",                   nbNETPDL_EL_IF,                  nbNETPDL_IDEL_IF,                  CreateElementIf, OrganizeElementIf, DeleteElementIf, SerializeElementGeneric)
nbNETPDL_ELEMENT("if-true",              nbNETPDL_EL_IFTRUE,              nbNETPDL_IDEL_IFTRUE,              CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("if-false",             nbNETPDL_EL_IFFALSE,             nbNETPDL_IDEL_IFFALSE,             CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("switch",               nbNETPDL_EL_SWITCH,              nbNETPDL_IDEL_SWITCH,              CreateElementSwitch, OrganizeElementSwitch, DeleteElementSwitch, SerializeElementGeneric)
nbNETPDL_ELEMENT("case",                 nbNETPDL_EL_CASE,                nbNETPDL_IDEL_CASE,                CreateElementCase,    OrganizeElementCase, DeleteElementCase, SerializeElementGeneric)
nbNETPDL_ELEMENT("default",              nbNETPDL_EL_DEFAULT,             nbNETPDL_IDEL_DEFAULT,             CreateElementDefault, OrganizeElementGeneric, DeleteElementDefault, SerializeElementGeneric)
nbNETPDL_ELEMENT("loop",                 nbNETPDL_EL_LOOP,                nbNETPDL_IDEL_LOOP,                CreateElementLoop,    OrganizeElementLoop, DeleteElementLoop, SerializeElementGeneric)
nbNETPDL_ELEMENT("loopctrl",             nbNETPDL_EL_LOOPCTRL,            nbNETPDL_IDEL_LOOPCTRL,            CreateElementLoopCtrl, OrganizeElementGeneric, DeleteElementLoopCtrl, SerializeElementGeneric)
nbNETPDL_ELEMENT("missing-packetdata",   nbNETPDL_EL_MISSINGPACKETDATA,   nbNETPDL_IDEL_MISSINGPACKETDATA,   CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)

nbNETPDL_ELEMENT("visualization",        nbNETPDL_EL_NETPDLSHOW,          nbNETPDL_IDEL_NETPDLSHOW,          CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("showtemplate",         nbNETPDL_EL_SHOWTEMPLATE,        nbNETPDL_IDEL_SHOWTEMPLATE,        CreateElementShowTemplate, OrganizeElementShowTemplate, DeleteElementShowTemplate, SerializeElementShowTemplate)
nbNETPDL_ELEMENT("showsumtemplate",      nbNETPDL_EL_SHOWSUMTEMPLATE,     nbNETPDL_IDEL_SHOWSUMTEMPLATE,     CreateElementShowSumTemplate, OrganizeElementGeneric, DeleteElementShowSumTemplate, SerializeElementShowSumTemplate)
nbNETPDL_ELEMENT("showmap",              nbNETPDL_EL_SHOWMAP,             nbNETPDL_IDEL_SHOWMAP,             CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("showdtl",              nbNETPDL_EL_SHOWDTL,             nbNETPDL_IDEL_SHOWDTL,             CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("text",                 nbNETPDL_EL_TEXT,                nbNETPDL_IDEL_TEXT,                CreateElementShowCodeText, OrganizeElementGeneric, DeleteElementShowCodeText, SerializeElementGeneric)
nbNETPDL_ELEMENT("section",              nbNETPDL_EL_SECTION,             nbNETPDL_IDEL_SECTION,             CreateElementShowCodeSection, OrganizeElementSection, DeleteElementShowCodeSection, SerializeElementGeneric)
nbNETPDL_ELEMENT("protofield",           nbNETPDL_EL_PROTOFIELD,          nbNETPDL_IDEL_PROTOFIELD,          CreateElementShowCodeProtoField, OrganizeElementGeneric, DeleteElementShowCodeProtoField, SerializeElementGeneric)
nbNETPDL_ELEMENT("protohdr",             nbNETPDL_EL_PROTOHDR,            nbNETPDL_IDEL_PROTOHDR,            CreateElementShowCodeProtoHdr, OrganizeElementGeneric, DeleteElementShowCodeProtoHdr, SerializeElementGeneric)
nbNETPDL_ELEMENT("packethdr",               nbNETPDL_EL_PKTHDR,              nbNETPDL_IDEL_PKTHDR,              CreateElementShowCodePacketHdr, OrganizeElementGeneric, DeleteElementShowCodePacketHdr, SerializeElementGeneric)

nbNETPDL_ELEMENT("showsumstruct",        nbNETPDL_EL_SHOWSUMSTRUCT,       nbNETPDL_IDEL_SHOWSUMSTRUCT,       CreateElementGeneric, OrganizeElementGeneric, DeleteElementGeneric, SerializeElementGeneric)
nbNETPDL_ELEMENT("sumsection",           nbNETPDL_EL_SUMSECTION,          nbNETPDL_IDEL_SUMSECTION,          CreateElementSumSection, OrganizeElementGeneric, DeleteElementSumSection, SerializeElementGeneric)

// This is a dummy element for terminating the element list.
nbNETPDL_ELEMENT("",                     nbNETPDL_EL_INVALID,             nbNETPDL_IDEL_INVALID,             NULL, NULL, NULL, NULL)


