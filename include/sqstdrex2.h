/*  see copyright notice in squirrel.h */
#ifndef _SQSTDREX2_H_
#define _SQSTDREX2_H_

#ifdef __cplusplus
extern "C" {
#endif
	/*
SQUIRREL_API SQUserPointer sqstd_createblob(HSQUIRRELVM v, SQInteger size);
SQUIRREL_API SQRESULT sqstd_getblob(HSQUIRRELVM v,SQInteger idx,SQUserPointer *ptr);
SQUIRREL_API SQInteger sqstd_getblobsize(HSQUIRRELVM v,SQInteger idx);
*/
SQUIRREL_API SQRESULT sqstd_register_regexp2lib(HSQUIRRELVM v);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif /*_SQSTDBLOB_H_*/

