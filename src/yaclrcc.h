/*
 * Name:        yaclrcc.h
 * Description: Yet another CLR compiler compiler.
 * Author:      cosh.cage#hotmail.com
 * File ID:     0208240241B0208240241L00027
 * License:     GPLv2.
 */
#ifndef _YACLRCC_H_
#define _YACLRCC_H_

#include "StoneValley/src/svstring.h"

#define ACC ((~(size_t)0) >> 1)

typedef ptrdiff_t(*CBF_GetSymbol)(void);

typedef ptrdiff_t(*CBF_Reduce)(ptrdiff_t n);

typedef void (*CBF_Error)(void);

P_MATRIX ConstructCLRTable(wchar_t * wcsbnf, P_ARRAY_Z * pparrG);
void PrintCLRTable(P_MATRIX ptbl);
void DestroyParrList(P_ARRAY_Z parrlist);
void DestroyCLRTable(P_MATRIX ptbl);

#endif
