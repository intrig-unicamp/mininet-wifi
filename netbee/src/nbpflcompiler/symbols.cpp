#include "encapgraph.h"
#include "symbols.h"

Symbol * Symbol::copy()
{
	return new Symbol(this->SymKind);
}

Symbol *SymbolTemp::copy()
{
	return new SymbolTemp(Temp, Name);
}

Symbol * SymbolIntConst::copy()
{
	return new SymbolIntConst(Value);
}

Symbol *SymbolStrConst::copy()
{
	return new SymbolStrConst(Name, Size, MemOffset);
}

Symbol *SymbolLabel::copy()
{
	return new SymbolLabel(LblKind, Label, Name);
}

Symbol *SymbolField::copy()
{
	return new SymbolField(Name, FieldType, PDLFieldInfo);
}

Symbol *SymbolFieldContainer::copy()
{
	return new SymbolFieldContainer(Name, FieldType, PDLFieldInfo);
}

Symbol *SymbolFieldFixed::copy()
{
	return new SymbolFieldFixed(Name, Size, PDLFieldInfo, DefPoint, IndexTemp, ValueTemp);
}

Symbol *SymbolFieldBitField::copy()
{
	return new SymbolFieldBitField(Name, Mask, PDLFieldInfo);
}

Symbol *SymbolFieldPadding::copy()
{
	return new SymbolFieldPadding(Name, Align, PDLFieldInfo, DefPoint);
}

Symbol *SymbolFieldVarLen::copy()
{
	return new SymbolFieldVarLen(Name, PDLFieldInfo, LenExpr, DefPoint, IndexTemp, LenTemp);
}

Symbol *SymbolFieldTokEnd::copy()
{
	return new SymbolFieldTokEnd(Name,EndTok, EndTokSize,EndRegEx, PDLFieldInfo, EndOff, EndDiscard, DefPoint, IndexTemp, LenTemp, DiscTemp);
}
Symbol *SymbolFieldTokWrap::copy()
{
	return new SymbolFieldTokWrap(Name,BeginTok, BeginTokSize,EndTok, EndTokSize,BeginRegEx ,EndRegEx, PDLFieldInfo, BeginOff, EndOff, EndDiscard, DefPoint, IndexTemp, LenTemp, DiscTemp);
}
Symbol *SymbolFieldLine::copy()
{
	return new SymbolFieldLine(Name, PDLFieldInfo, DefPoint, IndexTemp, LenTemp);
}

Symbol *SymbolFieldPattern::copy()
{
	return new SymbolFieldPattern(Name, Pattern, PDLFieldInfo, DefPoint, IndexTemp, LenTemp);
}

Symbol *SymbolFieldEatAll::copy()
{
	return new SymbolFieldEatAll(Name, PDLFieldInfo, DefPoint, IndexTemp, LenTemp);
}

Symbol *SymbolVariable::copy()
{
	return new SymbolVariable(Name, VarType, Validity);
}

Symbol *SymbolVarInt::copy()
{
	return new SymbolVarInt(Name, Validity, Temp);
}

Symbol *SymbolVarProto::copy()
{
	return new SymbolVarProto(Name, Validity);
}

Symbol *SymbolVarBufRef::copy()
{
	return new SymbolVarBufRef(Name, Validity, RefType);
}

Symbol *SymbolVarBuffer::copy()
{
	return new SymbolVarBuffer(Name, Size, Validity);
}

Symbol *SymbolProto::copy()
{
	return new SymbolProto(*Info, ID);
}
