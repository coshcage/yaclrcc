/*
 * Name:        yaclrcc.h
 * Description: Yet another CLR compiler compiler.
 * Author:      cosh.cage#hotmail.com
 * File ID:     0208240241B0208240636L00035
 * License:     GPLv2.
 */
#ifndef _YACLRCC_H_
#define _YACLRCC_H_

#include "svstring.h"
#include "svqueue.h"

/* Accept status. */
#define ACC ((~(size_t)0) >> 1)

/* Callback functions for parser. */
typedef ptrdiff_t (*CBF_GetSymbol)(void);
typedef int (*CBF_Reduce)(ptrdiff_t n);
/* Error handle callback. */
typedef int (*CBF_Error)(void);

/* Function declarations. */
P_QUEUE_L LexCompile       (wchar_t * strlex);
void      LexDestroy       (P_QUEUE_L pq);
size_t    Lexer            (P_QUEUE_L pq,     wchar_t     wc);
P_MATRIX  ConstructCLRTable(wchar_t * wcsbnf, P_ARRAY_Z * pparrG);
void      PrintCLRTable    (P_MATRIX  ptbl);
BOOL      CLRParse         (P_MATRIX  ptable, P_ARRAY_Z   parrG,  CBF_GetSymbol cbfgs, CBF_Reduce cbfrdc, CBF_Error cbferr);
void      PrintParrList    (P_ARRAY_Z parrlist);
void      DestroyParrList  (P_ARRAY_Z parrlist);
void      DestroyCLRTable  (P_MATRIX  ptbl);

#endif
