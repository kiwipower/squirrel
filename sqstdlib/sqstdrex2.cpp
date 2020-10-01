/* see copyright notice in squirrel.h */
#include <new>
#include <squirrel.h>
#include <sqstdio.h>
#include <string.h>
#include <sqstdrex2.h>
#include "sqstdstream.h"
#include "re2/re2.h"

using namespace re2;

static SQUserPointer rex2_typetag = NULL;

#define SETUP_REGEXP2(v) \
    RE2 *self = NULL; \
    { if(SQ_FAILED(sq_getinstanceup(v,1,(SQUserPointer*)&self,rex2_typetag))) \
        return sq_throwerror(v,_SC("invalid type tag"));  } \
    if(!self)  \
        return sq_throwerror(v,_SC("the regexp2 is invalid"));

static SQInteger _regexp2_releasehook(SQUserPointer p, SQInteger SQ_UNUSED_ARG(size))
{
    RE2* self = (RE2*)p;
    self->~RE2();
    sq_free(self, sizeof(RE2));
    return 1;
}

static SQInteger _regexp2_constructor(HSQUIRRELVM v)
{
    RE2* self = NULL;
    if (SQ_FAILED(sq_getinstanceup(v, 1, (SQUserPointer*)&self, rex2_typetag))) {
        return sq_throwerror(v, _SC("invalid type tag"));
    }
    if (self != NULL) {
        return sq_throwerror(v, _SC("invalid regexp object"));
    }

    SQInteger nparam = sq_gettop(v);

    const SQChar* expression;
    sq_getstring(v, 2, &expression);

    uint32_t expressionSize = strlen(expression);
    uint32_t wrappedExpressionSize = expressionSize + 3;

    SQChar* wrappedExpression = (SQChar * )(sq_malloc(wrappedExpressionSize));

    wrappedExpression[0] = '(';
    memcpy(&wrappedExpression[1], expression, expressionSize);
    wrappedExpression[wrappedExpressionSize-2] = ')';
    wrappedExpression[wrappedExpressionSize-1] = '\0';

    RE2* re2 = new (sq_malloc(sizeof(RE2)))RE2(wrappedExpression);

    sq_free(wrappedExpression, wrappedExpressionSize);

    //assert(re.ok());

    if (SQ_FAILED(sq_setinstanceup(v, 1, re2))) {
        re2->~RE2();
        sq_free(re2, sizeof(RE2));
        return sq_throwerror(v, _SC("cannot create regexp2"));
    }
    sq_setreleasehook(v, 1, _regexp2_releasehook);

    return 0;
}

static SQInteger _regexp2_capture(HSQUIRRELVM v)
{
    SETUP_REGEXP2(v);

    SQInteger nparam = sq_gettop(v);

    SQInteger offset = 0;

    if (nparam >= 3) {
        sq_getinteger(v, 3, &offset);
    }

    const SQChar* comparisonString;
    sq_getstring(v, 2, &comparisonString);

    if (offset >= (SQInteger)strlen(comparisonString)) {
        return 0;
    }

    uint32_t numberOfCapturingGroups = self->NumberOfCapturingGroups();
    std::vector<re2::StringPiece> stringPieces(numberOfCapturingGroups);
    std::vector<RE2::Arg> arguments (numberOfCapturingGroups);
    std::vector<RE2::Arg*> argumentsPointers (numberOfCapturingGroups);

    for (uint32_t i = 0; i < numberOfCapturingGroups; ++i) {
        arguments[i] = &stringPieces[i];
        argumentsPointers[i] = &arguments[i];
    }

    RE2::PartialMatchN(re2::StringPiece(comparisonString +offset), *self, argumentsPointers.data(), numberOfCapturingGroups);

    sq_newarray(v, 0);
    for (uint32_t i = 0; i < numberOfCapturingGroups; ++i) {
        sq_newtableex(v, 2);

        SQInteger begin = (SQInteger)((const SQChar*)stringPieces[i].begin() - comparisonString);
        SQInteger end = (SQInteger)((const SQChar*)stringPieces[i].end() - comparisonString);

        if (begin == end) {
            sq_pushstring(v, "begin", -1);
            sq_pushinteger(v, -1);
            sq_createslot(v, -3);
            sq_pushstring(v, "end", -1);
            sq_pushinteger(v, -1);
            sq_createslot(v, -3);
        }
        else {
            sq_pushstring(v, "begin", -1);
            sq_pushinteger(v, begin);
            sq_createslot(v, -3);
            sq_pushstring(v, "end", -1);
            sq_pushinteger(v, end);
            sq_createslot(v, -3);
        }

        sq_arrayappend(v, -2);
    }

    return 1;
}

static SQInteger _regexp2_match(HSQUIRRELVM v)
{
    SETUP_REGEXP2(v);

    SQInteger nparam = sq_gettop(v);

    const SQChar* comparisonString;
    sq_getstring(v, 2, &comparisonString);

    sq_pushbool(v, RE2::FullMatch(re2::StringPiece(comparisonString), *self ));

    return 1;
}

static SQInteger _regexp2_search(HSQUIRRELVM v)
{
    SETUP_REGEXP2(v);

    SQInteger nparam = sq_gettop(v);

    SQInteger offset = 0;

    if (nparam >= 3) {
        sq_getinteger(v, 3, &offset);
    }

    const SQChar* comparisonString;
    sq_getstring(v, 2, &comparisonString);

    if (offset >= (SQInteger)strlen(comparisonString)) {
        return 0;
    }

    uint32_t numberOfCapturingGroups = self->NumberOfCapturingGroups();
    std::vector<re2::StringPiece> stringPieces(numberOfCapturingGroups);
    std::vector<RE2::Arg> arguments(numberOfCapturingGroups);
    std::vector<RE2::Arg*> argumentsPointers(numberOfCapturingGroups);

    for (uint32_t i = 0; i < numberOfCapturingGroups; ++i) {
        arguments[i] = &stringPieces[i];
        argumentsPointers[i] = &arguments[i];
    }

    SQBool result = RE2::PartialMatchN(re2::StringPiece(comparisonString + offset), *self, argumentsPointers.data(), numberOfCapturingGroups);

    if (result == SQFalse) {
        return 0;
    }

    uint32_t matchedArgument = 0;
    SQInteger highestBegin = 0;

    for (uint32_t i = 0; i < numberOfCapturingGroups; ++i) {

        SQInteger begin = (SQInteger)((const SQChar*)stringPieces[i].begin() - comparisonString);

        if (begin > highestBegin) {
            highestBegin = begin;
            matchedArgument = i;
        }
    }
    

    SQInteger begin = (SQInteger)((const SQChar*)stringPieces[matchedArgument].begin() - comparisonString);
    SQInteger end = (SQInteger)((const SQChar*)stringPieces[matchedArgument].end() - comparisonString);

    sq_newtableex(v, 2);
    sq_pushstring(v, "begin", -1);
    sq_pushinteger(v, begin);
    sq_createslot(v, -3);
    sq_pushstring(v, "end", -1);
    sq_pushinteger(v, end);
    sq_createslot(v, -3);
    
    return 1;
}

// capture comparisonString, startIndex - return array
// match comparisonString - return bool
// search comparisonString, startIndex, - return table (first match)
// constructor expression

#define _DECL_REGEXP2_FUNC(name,nparams,typecheck) {_SC(#name),_regexp2_##name,nparams,typecheck}
static const SQRegFunction _regexp2_methods[] = {
    _DECL_REGEXP2_FUNC(constructor,2,_SC(".s")),
    _DECL_REGEXP2_FUNC(capture,-2,_SC("xsn")),
    _DECL_REGEXP2_FUNC(match,2,_SC("xs")),
    _DECL_REGEXP2_FUNC(search,2,_SC("xsn")),
    {NULL,(SQFUNCTION)0,0,NULL}
};

#define _DECL_GLOBALREGEXP2_FUNC(name,nparams,typecheck) {_SC(#name),_g_regexp2_##name,nparams,typecheck}
static const SQRegFunction regexp2lib_funcs[] = {
    {NULL,(SQFUNCTION)0,0,NULL}
};

SQRESULT sqstd_register_regexp2lib(HSQUIRRELVM v)
{
    sq_pushstring(v, _SC("regexp2"), -1);
    sq_newclass(v, SQFalse);
    rex2_typetag = (SQUserPointer)_regexp2_methods;
    sq_settypetag(v, -1, rex2_typetag);
    SQInteger i = 0;
    while (_regexp2_methods[i].name != 0) {
        const SQRegFunction& f = _regexp2_methods[i];
        sq_pushstring(v, f.name, -1);
        sq_newclosure(v, f.f, 0);
        sq_setparamscheck(v, f.nparamscheck, f.typemask);
        sq_setnativeclosurename(v, -1, f.name);
        sq_newslot(v, -3, SQFalse);
        i++;
    }
    sq_newslot(v, -3, SQFalse);

    i = 0;
    while (regexp2lib_funcs[i].name != 0)
    {
        sq_pushstring(v, regexp2lib_funcs[i].name, -1);
        sq_newclosure(v, regexp2lib_funcs[i].f, 0);
        sq_setparamscheck(v, regexp2lib_funcs[i].nparamscheck, regexp2lib_funcs[i].typemask);
        sq_setnativeclosurename(v, -1, regexp2lib_funcs[i].name);
        sq_newslot(v, -3, SQFalse);
        i++;
    }

    return 1;
}