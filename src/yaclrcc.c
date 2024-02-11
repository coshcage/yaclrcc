/*
 * Name:        yaclrcc.h
 * Description: Yet another CLR compiler compiler.
 * Author:      cosh.cage#hotmail.com
 * File ID:     0208240241B0210242330L01300
 * License:     GPLv2.
 */
/* Macro for Visual C compiler. */
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include "svstring.h"
#include "svqueue.h"
#include "svtree.h"
#include "svset.h"
#include "svgraph.h"
#include "svstack.h"
#include "svregex.h"
#include "yaclrcc.h"

/* Get abstract value of ptrdiff_t. */
#define GETABS(num) ((num) >=0 ? (num) : -(num))

/* Structure for DFA sequence. */
typedef struct st_DFASEQ
{
	size_t  num;     /* Returning number. */
	P_DFA   pdfa;
	size_t  curstate;
} DFASEQ, * P_DFASEQ;

/* Enumeration for BNF type. */
typedef enum en_BNFType
{
	BT_TERMINATOR = 1,
	BT_NONTERMINATOR
} BNFType;

/* Structure for BNF element. */
typedef struct st_BNFELEMENT
{
	ptrdiff_t name;
	struct st_mark
	{
		BOOL bmark;
		P_SET_T pset;
	} m;
	
} BNFELEMENT, * P_BNFELEMENT;

/* Structure for NFA element. */
typedef struct st_NFAELE
{
	size_t    id;
	P_ARRAY_Z parrBNFLst;
} NFAELE, * P_NFAELE;

/* Function name: LexCompile
 * Description:   Compile L file.
 * Parameter:
 *    strlex Pointer to a wide character string.
 * Return value:  New allocated queue of a set of DFAs.
 */
P_QUEUE_L LexCompile(wchar_t * strlex)
{
	P_QUEUE_L pq;

	size_t j = 0;
	wchar_t buf[BUFSIZ] = { 0 }, * pbuf = buf;

	if (NULL != (pq = queCreateL()));
	{
		while (L'\0' != *strlex)
		{
			if (L'\n' != *strlex)
			{
				*pbuf++ = *strlex;
				*pbuf = L'\0';
			}
			else
			{
				DFASEQ dq = { 0 };
				dq.num = ++j;

				if (NULL == (dq.pdfa = CompileRegex2DFA(buf)))
					dq.num = 0;

#ifdef DEBUG
				PrintDFA(dq.pdfa);
#endif
				queInsertL(pq, &dq, sizeof(DFASEQ));

				pbuf = buf;
			}
			++strlex;
		}
	}
	return pq;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsLexerPuppet
 * Description:   Callback for lex matching.
 * Parameters:
 *      pitem Pointer to each node of a queue.
 *      param Pointer to a size_t[2] array.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsLexerPuppet(void * pitem, size_t param)
{
	size_t i;
	P_DFASEQ pdfaq = (P_DFASEQ)((P_NODE_S)pitem)->pdata;

	if (0 == pdfaq->curstate)
		pdfaq->curstate = 1;

	pdfaq->curstate = NextState(pdfaq->pdfa, pdfaq->curstate, (wchar_t)0[(size_t *)param]);
	strGetValueMatrix(&i, pdfaq->pdfa, pdfaq->curstate, 0, sizeof(size_t));
	if (i & SIGN)
	{
		1[(size_t *)param] = pdfaq->num;
		pdfaq->curstate = 1;
	}

	return CBF_CONTINUE;
}

/* Function name: Lexer
 * Description:   Lexical analyzer.
 * Parameters:
 *         pq Pointer to a queue of DFAs.
 *         wc Inputted wide character.
 * Return value:  Line number of regular expression in L file.
 */
size_t Lexer(P_QUEUE_L pq, wchar_t wc)
{
	size_t a[2];

	a[0] = wc;
	a[1] = 0;

	strTraverseLinkedListSC_N(pq->pfront, NULL, cbftvsLexerPuppet, (size_t)a);

	return a[1];
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsLexerDestroyPuppet
 * Description:   Free DFAs.
 * Parameters:
 *      pitem Pointer to each node of a queue.
 *      param N/A.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsLexerDestroyPuppet(void * pitem, size_t param)
{
	P_DFASEQ pdfaq = (P_DFASEQ)((P_NODE_S)pitem)->pdata;

	DWC4100(param);

	DestroyDFA(pdfaq->pdfa);
	return CBF_CONTINUE;
}

/* Function name: LexDestroy
 * Description:   Free the queue which is derived form LexCompile.
 * Parameter:
 *        pq Pointer to a queue of DFAs.
 * Return value:  N/A.
 */
void LexDestroy(P_QUEUE_L pq)
{
	strTraverseLinkedListSC_N(pq->pfront, NULL, cbftvsLexerDestroyPuppet, 0);
	queDeleteL(pq);
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbfcmpWChar_t
 * Description:   Compare wchar_t.
 * Parameters:
 *         px Pointer to wchar_t.
 *         py Pointer to another wchar_t.
 * Return value:  Comparation result.
 */
static int cbfcmpWChar_t(const void * px, const void * py)
{
	return (int)*(wchar_t *)px - (int)*(wchar_t *)py;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbfcmpPtrdifft
 * Description:   Compare ptrdiff_t.
 * Parameters:
 *         px Pointer to ptrdiff_t.
 *         py Pointer to another ptrdiff_t.
 * Return value:  Comparation result.
 */
static int cbfcmpPtrdifft(const void * px, const void * py)
{
	ptrdiff_t x = *(ptrdiff_t *)px;
	ptrdiff_t y = *(ptrdiff_t *)py;
	if (x > y) return 1;
	if (x < y) return -1;
	return 0;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbfcmpPtrdifftAsSeq
 * Description:   Compare ptrdiff_t and arrange result array as a sequence.
 * Parameters:
 *         px Pointer to ptrdiff_t.
 *         py Pointer to another ptrdiff_t.
 * Return value:  Comparation result.
 */
static int cbfcmpPtrdifftAsSeq(const void * px, const void * py)
{
	ptrdiff_t x = *(ptrdiff_t *)px;
	ptrdiff_t y = *(ptrdiff_t *)py;
	if (x < 0)
		x = -((ptrdiff_t)ACC + x);
	if (y < 0)
		y = -((ptrdiff_t)ACC + y);
	return x - y;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: EmitSymbol
 * Description:   Emit lexical symbol.
 * Parameters:
 *   pparrbnf Output an array of BNFs.
 *    ptriest Pointer to a trie.
 *        wcs Pointer to a buffer.
 *       type Non terminal or terminal.
 *psiSymbolCtr Number of symbols.
 *psetGmrSmbl Pointer to a set to store grammar symbols.
 * Return value:  N/A.
 */
static void EmitSymbol(P_ARRAY_Z * pparrbnf, P_TRIE_A ptriest, wchar_t * wcs, size_t type, size_t * psiSymbolCtr, P_SET_T psetGmrSmbl)
{
	size_t * ps;
	BNFELEMENT be = { 0 };
	char charbuf[BUFSIZ];
	switch (type)
	{
	case BT_TERMINATOR:
		wcstombs(charbuf, wcs, BUFSIZ);
		be.name = atoi(charbuf);
		break;
	case BT_NONTERMINATOR:
		/* Search Trie. */
		if (NULL != (ps = treSearchTrieA(ptriest, wcs, wcslen(wcs), sizeof(wchar_t), cbfcmpWChar_t)))
			be.name = (ptrdiff_t)*ps;
		else
		{
			(void)treInsertTrieA(ptriest, wcs, wcslen(wcs), sizeof(wchar_t), ((*psiSymbolCtr)++) * -1, cbfcmpWChar_t);
			be.name = (*psiSymbolCtr - 1) * -1;
		}

		break;
	}
	switch (type)
	{
	case BT_TERMINATOR:
	case BT_NONTERMINATOR:
		setInsertT(psetGmrSmbl, &be.name, sizeof(ptrdiff_t), cbfcmpPtrdifftAsSeq);
		if (NULL == *pparrbnf)
		{
			*pparrbnf = strCreateArrayZ(1, sizeof(BNFELEMENT));
			memcpy((*pparrbnf)->pdata, &be, sizeof(BNFELEMENT));
		}
		else
		{
			strResizeArrayZ(*pparrbnf, strLevelArrayZ(*pparrbnf) + 1, sizeof(BNFELEMENT));
			memcpy(strLocateItemArrayZ(*pparrbnf, sizeof(BNFELEMENT), strLevelArrayZ(*pparrbnf) - 1), &be, sizeof(BNFELEMENT));
		}
	}
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsDestroyParrlistPuppet
 * Description:   Free sets.
 * Parameters:
 *      pitem Pointer to BNFELEMENT.
 *      param N/A.
 * Return value:  CBF_TERMINATE only.
 */
static int cbftvsDestroyParrlistPuppet(void * pitem, size_t param)
{
	DWC4100(param);
	if (NULL != ((P_BNFELEMENT)pitem)->m.pset)
		setDeleteT(((P_BNFELEMENT)pitem)->m.pset);
	return CBF_TERMINATE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsDestroyParrlist
 * Description:   Free sized arrays.
 * Parameters:
 *      pitem Pointer to array.
 *      param N/A.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsDestroyParrlist(void * pitem, size_t param)
{
	DWC4100(param);
	if (NULL != *(P_ARRAY_Z *)pitem)
	{
		strTraverseArrayZ(*(P_ARRAY_Z *)pitem, sizeof(BNFELEMENT), cbftvsDestroyParrlistPuppet, 0, TRUE);
		strDeleteArrayZ(*(P_ARRAY_Z *)pitem);
	}
	return CBF_CONTINUE;
}

/* Function name: DestroyParrList
 * Description:   Free a BNF list.
 * Parameter:
 *  parrlist Pointer to a BNF list.
 * Return value:  N/A.
 */
void DestroyParrList(P_ARRAY_Z parrlist)
{
	strTraverseArrayZ(parrlist, sizeof(P_ARRAY_Z), cbftvsDestroyParrlist, 0, FALSE);
	strDeleteArrayZ(parrlist);
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsPrintParrlistPuppet
 * Description:   Print parr list.
 * Parameters:
 *      pitem Pointer to BNFELEMENT.
 *      param N/A.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsPrintParrlistPuppet(void * pitem, size_t param)
{
	P_BNFELEMENT pbe = (P_BNFELEMENT)pitem;
	DWC4100(param);
	printf("(%zd) ", pbe->name);
	printf("%c", pbe->m.bmark ? '.' : ' ');
	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsPrintSetCLR
 * Description:   Print a set.
 * Parameters:
 *      pitem Pointer to each node in a set.
 *      param N/A.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsPrintSetCLR(void * pitem, size_t param)
{
	DWC4100(param);
	printf("%zd, ", *(ptrdiff_t *)P2P_TNODE_BY(pitem)->pdata);
	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsPrintParrlistPuppet
 * Description:   Print parr list.
 * Parameters:
 *      pitem Pointer to an array.
 *      param N/A.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsPrintParrlist(void * pitem, size_t param)
{
	DWC4100(param);

	if (NULL != *(P_ARRAY_Z *)pitem)
		strTraverseArrayZ(*(P_ARRAY_Z *)pitem, sizeof(BNFELEMENT), cbftvsPrintParrlistPuppet, 0, FALSE);

	printf(" {");

	setTraverseT(((P_BNFELEMENT)strLocateItemArrayZ(*(P_ARRAY_Z *)pitem, sizeof(BNFELEMENT), strLevelArrayZ(*(P_ARRAY_Z *)pitem) - 1))->m.pset, cbftvsPrintSetCLR, 0, ETM_INORDER);

	printf("}\n");
	return CBF_CONTINUE;
}

/* Function name: PrintParrList
 * Description:   Print BNF list.
 * Parameter:
 *  parrlist Pointer to a BNF list.
 * Return value:  N/A.
 */
void PrintParrList(P_ARRAY_Z parrlist)
{
	strTraverseArrayZ(parrlist, sizeof(P_ARRAY_Z), cbftvsPrintParrlist, 0, FALSE);
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: FIRST
 * Description:   Get FIRST set of a symbol.
 * Parameters:
 * parrBNFLst Pointer to a BNF list.
 *          a A symbol.
 * Return value:  Pointer to FIRST set of a symbol.
 */
static P_SET_T FIRST(P_ARRAY_Z parrBNFLst, ptrdiff_t a)
{
	P_SET_T pset = setCreateT();
	if (a > 0)	/* a is a terminator. */
		setInsertT(pset, &a, sizeof(ptrdiff_t), cbfcmpPtrdifft);
	else /* a is a non-terminator. */
	{
		size_t i;
		for (i = 0; i < strLevelArrayZ(parrBNFLst); ++i)
		{
			P_ARRAY_Z parr = *(P_ARRAY_Z *)strLocateItemArrayZ(parrBNFLst, sizeof(P_ARRAY_Z), i);
			if 
			(
				((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), 0))->name == a &&
				((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), 1))->name > 0
			)
				setInsertT(pset, &((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), 1))->name, sizeof(ptrdiff_t), cbfcmpPtrdifft);
		}
	}
	return pset;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: BNFInSetI
 * Description:   Return whether a BNF is in set I.
 * Parameters:
 *      parrI Pointer to a BNF list.
 *pbnfTemplate Pointer to a BNF.
 * Return value:  TRUE  BNF is in the set.
 *                FALSE BNF is NOT in the set.
 */
static BOOL BNFInSetI(P_ARRAY_Z parrI, P_ARRAY_Z pbnfTemplate)
{
	BOOL r = FALSE;
	size_t i, j;
	for (i = 0; i < strLevelArrayZ(parrI); ++i)
	{
		P_ARRAY_Z pbnf = *(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), i);
		if (strLevelArrayZ(pbnfTemplate) != strLevelArrayZ(pbnf))
			continue;
		for (j = 0; j < strLevelArrayZ(pbnfTemplate); ++j)
		{
			if (((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), j))->name != ((P_BNFELEMENT)strLocateItemArrayZ(pbnfTemplate, sizeof(BNFELEMENT), j))->name)
				goto Lbl_Passover;
			if (((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), j))->m.bmark != ((P_BNFELEMENT)strLocateItemArrayZ(pbnfTemplate, sizeof(BNFELEMENT), j))->m.bmark)
				goto Lbl_Passover;
		}
		r = TRUE;
	Lbl_Passover:
		;
	}

	return r;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: CLOSURE
 * Description:   CLOSURE.
 * Parameters:
 *      parrG Pointer to a BNF list.
 *      parrI Pointer to a BNF list.
 * Return value:  N/A.
 */
static void CLOSURE(P_ARRAY_Z parrG, P_ARRAY_Z parrI)
{
	/* A -> alpha B beta, a. */
	ptrdiff_t A, alpha = 0, B = 0, beta = 0;
	size_t i, j, k = 1;
	
	for (i = 0; i < strLevelArrayZ(parrI); ++i)
	{
		P_ARRAY_Z pbnf = *(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), i);

		A = ((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), 0))->name;

		for (j = 0; j < strLevelArrayZ(pbnf); ++j)
		{
			if (((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), j))->m.bmark)
			{
				k = j + 1;
				break;
			}
		}

		for (j = k, alpha = 0, B = 0, beta = 0; j < strLevelArrayZ(pbnf); ++j)
		{
			if (((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), j))->name > 0 && 0 == alpha)
			{
				alpha = ((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), j))->name;
			}
			if (((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), j))->name < 0 && 0 == B)
			{
				B = ((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), j))->name;
			}
			else if (0 != B)
			{
				beta = ((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), j))->name;
			}
		}

		for (j = 0; j < strLevelArrayZ(parrG); ++j)
		{
			P_ARRAY_Z parr = *(P_ARRAY_Z *)strLocateItemArrayZ(parrG, sizeof(P_ARRAY_Z), j);
			((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), 0))->m.bmark = TRUE;
			if (B == ((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), 0))->name)
			{
				P_ARRAY_Z parr2;
				if (!BNFInSetI(parrI, parr))
				{
					/* Copy bnf. */
					strResizeArrayZ(parrI, strLevelArrayZ(parrI) + 1, sizeof(P_ARRAY_Z));
					parr2 = *(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), strLevelArrayZ(parrI) - 1) = strCreateArrayZ(strLevelArrayZ(parr), sizeof(BNFELEMENT));
					strCopyArrayZ(parr2, parr, sizeof(BNFELEMENT));
					((P_BNFELEMENT)strLocateItemArrayZ(parr2, sizeof(BNFELEMENT), 0))->m.bmark = TRUE;

					if (0 == beta)	/* Copy set FIRST. */
						((P_BNFELEMENT)strLocateItemArrayZ(parr2, sizeof(BNFELEMENT), strLevelArrayZ(parr2) - 1))->m.pset = setCopyT(((P_BNFELEMENT)strLocateItemArrayZ(pbnf, sizeof(BNFELEMENT), strLevelArrayZ(pbnf) - 1))->m.pset, sizeof(ptrdiff_t));
					else
						((P_BNFELEMENT)strLocateItemArrayZ(parr2, sizeof(BNFELEMENT), strLevelArrayZ(parr2) - 1))->m.pset = FIRST(parrG, B);
				}
				else
					++i;
			}
		}
	}
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: GOTO
 * Description:   GOTO.
 * Parameters:
 *      parrG Pointer to a BNF list.
 *      parrI Pointer to a BNF list.
 *          X A symbol.
 * Return value:  Pointer to a BNF list.
 */
static P_ARRAY_Z GOTO(P_ARRAY_Z parrG, P_ARRAY_Z parrI, ptrdiff_t X)
{
	P_ARRAY_Z parrJ = strCreateArrayZ(1, sizeof(P_ARRAY_Z));
	size_t i;
	ptrdiff_t x = 0;

	P_ARRAY_Z * pJ = (P_ARRAY_Z *)strLocateItemArrayZ(parrJ, sizeof(P_ARRAY_Z), 0);
	*pJ = NULL;

	for (i = 0; i < strLevelArrayZ(parrI); ++i)
	{
		size_t j, k = 1;
		P_ARRAY_Z parr = *(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), i);

		for (j = 0; j < strLevelArrayZ(parr); ++j)
		{
			if (((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), j))->m.bmark)
			{
				k = j + 1;
				break;
			}
		}

		for (j = k; j < strLevelArrayZ(parr); ++j)
		{
			x = ((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), j))->name;
			if (x == X)
			{	/* Add item to set J. */
				*pJ = strCreateArrayZ(strLevelArrayZ(parr), sizeof(BNFELEMENT));
				strCopyArrayZ(*pJ, parr, sizeof(BNFELEMENT));
				((P_BNFELEMENT)strLocateItemArrayZ(*pJ, sizeof(BNFELEMENT), strLevelArrayZ(*pJ) - 1))->m.pset = setCopyT(((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), strLevelArrayZ(parr) - 1))->m.pset, sizeof(ptrdiff_t));
				
				for (k = 0; k < strLevelArrayZ(*pJ); ++k)
				{
					if (((P_BNFELEMENT)strLocateItemArrayZ(*pJ, sizeof(BNFELEMENT), k))->m.bmark)
					{
						if (k < strLevelArrayZ(*pJ) - 1)
						{
							((P_BNFELEMENT)strLocateItemArrayZ(*pJ, sizeof(BNFELEMENT), k + 1))->m.bmark = TRUE;
							((P_BNFELEMENT)strLocateItemArrayZ(*pJ, sizeof(BNFELEMENT), k))->m.bmark = FALSE;
							break;
						}
					}
				}

				strResizeArrayZ(parrJ, strLevelArrayZ(parrJ) + 1, sizeof(P_ARRAY_Z));
				++pJ;
				*pJ = NULL;
				goto Lbl_ReturnCLOSUREJ;
			}
		}
	}

	Lbl_ReturnCLOSUREJ:

	if (strLevelArrayZ(parrJ) == 1 && NULL == *(P_ARRAY_Z *)strLocateItemArrayZ(parrJ, sizeof(P_ARRAY_Z), 0))
	{
		strDeleteArrayZ(parrJ);
		return NULL;
	}
	
	strResizeArrayZ(parrJ, strLevelArrayZ(parrJ) - 1, sizeof(P_ARRAY_Z));
	if (((P_BNFELEMENT)strLocateItemArrayZ(*(pJ - 1), sizeof(BNFELEMENT), strLevelArrayZ(*(pJ - 1)) - 1))->m.bmark)
		return parrJ;
	CLOSURE(parrG, parrJ);
	return parrJ;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: IsTheSameBNFSet
 * Description:   Return whether two BNF sets are the same or not.
 * Parameters:
 *      parrx Pointer to a BNF list.
 *      parry Pointer to a BNF list.
 * Return value:  TRUE  Same.
 *                FALSE Not the same.
 */
static BOOL IsTheSameBNFSet(P_ARRAY_Z parrx, P_ARRAY_Z parry)
{
	size_t i, j;
	if (strLevelArrayZ(parrx) != strLevelArrayZ(parry))
		return FALSE;
	for (i = 0; i < strLevelArrayZ(parrx); ++i)
	{
		P_ARRAY_Z parrBNFx = *(P_ARRAY_Z *)strLocateItemArrayZ(parrx, sizeof(P_ARRAY_Z), i);
		P_ARRAY_Z parrBNFy = *(P_ARRAY_Z *)strLocateItemArrayZ(parry, sizeof(P_ARRAY_Z), i);
		if (strLevelArrayZ(parrBNFx) != strLevelArrayZ(parrBNFy))
			return FALSE;
		for (j = 0; j < strLevelArrayZ(parrBNFx); ++j)
		{
			if (((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), j))->name != ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), j))->name)
				return FALSE;
			if (!setIsEqualT(((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFx) - 1))->m.pset, ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFy) - 1))->m.pset, cbfcmpPtrdifft))
				return FALSE;
		}
	}
	return TRUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsCompareBNFs
 * Description:   Compare BNFs.
 * Parameters:
 *      pitem Pointer to a VERTEX_L.
 *      param Pointer to a size_t array.
 * Return value:  CBF_CONTINUE and CBF_TERMINATE respectively.
 */
static int cbftvsCompareBNFs(void * pitem, size_t param)
{
	size_t * pbfound = (size_t *)param;
	P_ARRAY_Z parr = (P_ARRAY_Z)1[(size_t *)param];
	P_VERTEX_L pvtx = (P_VERTEX_L)pitem;

	if (IsTheSameBNFSet(parr, ((P_NFAELE)pvtx->vid)->parrBNFLst))
	{
		*pbfound = TRUE;
		return CBF_TERMINATE;
	}
	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: ParrInSetY
 * Description:   Return whether parr is in set Y ot not.
 * Parameters:
 *       parr Pointer to a BNF list.
 *      parry Pointer to a BNF list.
 * Return value:  TRUE  parr is in the set Y.
 *                FALSE parr is NOT in the set Y.
 */
static BOOL ParrInSetY(P_ARRAY_Z parr, P_ARRAY_Z parry)
{
	size_t i, j;
	
	P_ARRAY_Z parrBNFx = *(P_ARRAY_Z *)strLocateItemArrayZ(parr, sizeof(P_ARRAY_Z), 0);

	for (i = 0; i < strLevelArrayZ(parry); ++i)
	{
		P_ARRAY_Z parrBNFy = *(P_ARRAY_Z *)strLocateItemArrayZ(parry, sizeof(P_ARRAY_Z), i);
		if (strLevelArrayZ(parrBNFx) != strLevelArrayZ(parrBNFy))
			return FALSE;
		for (j = 0; j < strLevelArrayZ(parrBNFx); ++j)
		{
			if (((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), j))->name != ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), j))->name)
				return FALSE;
			if (((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), j))->m.bmark != ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), j))->m.bmark)
				return FALSE;
			if (!setIsEqualT(((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFx) - 1))->m.pset, ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFy) - 1))->m.pset, cbfcmpPtrdifft))
				return FALSE;
		}
	}
	return TRUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsParrInPgrpC
 * Description:   Return whether parr is in set C.
 * Parameters:
 *      pitem Pointer to a VERTEX_L.
 *      param Pointer to a size_t array.
 * Return value:  CBF_CONTINUE and CBF_TERMINATE respectively.
 */
static int cbftvsParrInPgrpC(void * pitem, size_t param)
{
	size_t i, j;
	size_t * pbfound = (size_t *)param;
	P_ARRAY_Z parr = (P_ARRAY_Z)1[(size_t *)param];
	P_VERTEX_L pvtx = (P_VERTEX_L)pitem;
	P_ARRAY_Z parry = ((P_NFAELE)pvtx->vid)->parrBNFLst;
	P_ARRAY_Z parrBNFx = *(P_ARRAY_Z *)strLocateItemArrayZ(parr, sizeof(P_ARRAY_Z), 0);

	for (i = 0; i < strLevelArrayZ(parry); ++i)
	{
		P_ARRAY_Z parrBNFy = *(P_ARRAY_Z *)strLocateItemArrayZ(parry, sizeof(P_ARRAY_Z), i);
		if (strLevelArrayZ(parrBNFx) != strLevelArrayZ(parrBNFy))
			continue;
		for (j = 0; j < strLevelArrayZ(parrBNFx); ++j)
		{
			if (((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), j))->name != ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), j))->name)
				goto Lbl_PassDetection;
			if (((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), j))->m.bmark != ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), j))->m.bmark)
				goto Lbl_PassDetection;
			if (!setIsEqualT(((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFx) - 1))->m.pset, ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFy) - 1))->m.pset, cbfcmpPtrdifft))
				goto Lbl_PassDetection;
		}
		*pbfound = TRUE;
		2[(size_t *)param] = pvtx->vid;
		return CBF_TERMINATE;
	Lbl_PassDetection:
		;
	}

	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsForEachGmrSmbl
 * Description:   Repeat each symbol.
 * Parameters:
 *      pitem Pointer to each node in the symbol set.
 *      param Pointer to a size_t array.
 * Return value:  CBF_TERMINATE only.
 */
static int cbftvsForEachGmrSmbl(void * pitem, size_t param)
{
	ptrdiff_t X = *(ptrdiff_t *)P2P_TNODE_BY(pitem)->pdata;
	if (X == -1)
		return CBF_CONTINUE;
	else
	{
		P_GRAPH_L pgrpC = (P_GRAPH_L)0[(size_t *)param];
		P_NFAELE pNFAEle = (P_NFAELE)1[(size_t *)param];
		P_QUEUE_L pq = (P_QUEUE_L)2[(size_t *)param];
		P_ARRAY_Z parrG = (P_ARRAY_Z)3[(size_t *)param];
		size_t * pNFAEleId = (size_t *)4[(size_t *)param];
		P_NFAELE pNFAEle0 = (P_NFAELE)5[(size_t *)param];

		P_ARRAY_Z parr = GOTO(parrG, pNFAEle->parrBNFLst, X);

		if (NULL != parr)
		{
			size_t a[3];

			a[0] = FALSE;
			a[1] = (size_t)parr;
			a[2] = 0;
			
			grpBFSL(pgrpC, (size_t)pNFAEle0, cbftvsCompareBNFs, (size_t)a);

			if (FALSE == a[0])
			{
				P_NFAELE pNFAEle2 = (P_NFAELE)malloc(sizeof(NFAELE));

				if (NULL != pNFAEle2)
				{
					pNFAEle2->id = (*pNFAEleId)++;
					pNFAEle2->parrBNFLst = parr;

					grpInsertVertexL(pgrpC, (size_t)pNFAEle2);
					queInsertL(pq, &pNFAEle2, sizeof(size_t));

					grpInsertEdgeL(pgrpC, (size_t)pNFAEle, (size_t)pNFAEle2, (size_t)X);
				}
			}
			else
			{
				P_ARRAY_Z parrBNF = *(P_ARRAY_Z *)strLocateItemArrayZ(pNFAEle->parrBNFLst, sizeof(P_ARRAY_Z), 0);
				if (!((P_BNFELEMENT)strLocateItemArrayZ(parrBNF, sizeof(BNFELEMENT), strLevelArrayZ(parrBNF) - 1))->m.bmark)
				{
					a[0] = FALSE;
					a[1] = (size_t)parr;
					a[2] = 0;

					grpBFSL(pgrpC, (size_t)pNFAEle0, cbftvsParrInPgrpC, (size_t)a);

					if (a[0])
						grpInsertEdgeL(pgrpC, (size_t)pNFAEle, (size_t)a[2], (size_t)X);
				}
				DestroyParrList(parr);
				parr = NULL;
			}
		}

		return CBF_CONTINUE;
	}
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: ITEMS
 * Description:   ITEMS.
 * Parameters:
 *      parrG Pointer to a BNF list.
 *psetGmrSmbl Pointer to a set of symbol.
 *         p0 Output a vertex ID.
 * Return value:  Pointer to a NFA graph.
 */
static P_GRAPH_L ITEMS(P_ARRAY_Z parrG, P_SET_T psetGmrSmbl, size_t * p0)
{
	P_GRAPH_L pgrpC = grpCreateL();
	P_NFAELE pNFAEle = (P_NFAELE)malloc(sizeof(NFAELE));
	P_QUEUE_L pq = queCreateL();
	size_t nFAEleId = 0;
	P_NFAELE pNFAEle0;

	if (NULL != pNFAEle)
	{
		pNFAEle->id = nFAEleId++;
		{
			size_t i;
			P_ARRAY_Z parrI = strCreateArrayZ(1, sizeof(P_ARRAY_Z));
			P_ARRAY_Z parrsrc = *(P_ARRAY_Z *)strLocateItemArrayZ(parrG, sizeof(P_ARRAY_Z), 0);
			P_ARRAY_Z parrdst = *(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), 0) =
				strCreateArrayZ(strLevelArrayZ(parrsrc), sizeof(BNFELEMENT));
			strCopyArrayZ(parrdst, parrsrc, sizeof(BNFELEMENT));

			((P_BNFELEMENT)strLocateItemArrayZ(*(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), 0), sizeof(BNFELEMENT), 0))->m.bmark = TRUE;
			((P_BNFELEMENT)strLocateItemArrayZ(*(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), 0), sizeof(BNFELEMENT), 1))->m.pset = setCreateT();
			i = ACC;
			setInsertT(((P_BNFELEMENT)strLocateItemArrayZ(*(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), 0), sizeof(BNFELEMENT), 1))->m.pset, &i, sizeof(ptrdiff_t), cbfcmpPtrdifft);
			CLOSURE(parrG, parrI);
			pNFAEle->parrBNFLst = parrI;
		}
		pNFAEle0 = pNFAEle;
		if (p0)
			*p0 = (size_t)pNFAEle0;
		grpInsertVertexL(pgrpC, (size_t)pNFAEle);
		queInsertL(pq, &pNFAEle, sizeof(P_NFAELE));
		while (!queIsEmptyL(pq))
		{
			size_t a[6];
			queRemoveL(&pNFAEle, sizeof(P_NFAELE), pq);
			
			a[0] = (size_t)pgrpC;
			a[1] = (size_t)pNFAEle;
			a[2] = (size_t)pq;
			a[3] = (size_t)parrG;
			a[4] = (size_t)&nFAEleId;
			a[5] = (size_t)pNFAEle0;

			setTraverseT(psetGmrSmbl, cbftvsForEachGmrSmbl, (size_t)a, ETM_INORDER);

		}
	}

	queDeleteL(pq);
	return pgrpC;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsDestroyNFAGraph
 * Description:   Free NFA graph.
 * Parameters:
 *      pitem Pointer to each VERTEX_L in the graph.
 *      param N/A.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsDestroyNFAGraph(void * pitem, size_t param)
{
	P_VERTEX_L pvtx = (P_VERTEX_L)pitem;
	P_NFAELE pNFAEle = (P_NFAELE)pvtx->vid;
	DWC4100(param);
	DestroyParrList(pNFAEle->parrBNFLst);
	free(pNFAEle);

	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: DestroyNFAGraph
 * Description:   Free NFA graph.
 * Parameters:
 *      pgrpC Pointer to NFA graph.
 *       vid0 Starting vertex ID.
 * Return value:  N/A.
 */
static void DestroyNFAGraph(P_GRAPH_L pgrpC, size_t vid0)
{
	grpBFSL(pgrpC, vid0, cbftvsDestroyNFAGraph, 0);
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsCountNFAVertices
 * Description:   Count NFA graph vertices.
 * Parameters:
 *      pitem N/A.
 *      param Pointer to a size_t integer.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsCountNFAVertices(void * pitem, size_t param)
{
	0[(size_t *)param]++;
	DWC4100(pitem);
	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsFillTableHeader
 * Description:   Fill parsing table header.
 * Parameters:
 *      pitem Pointer to each node in the set of symbol.
 *      param Pointer to a size_t array.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsFillTableHeader(void * pitem, size_t param)
{
	strSetValueMatrix
	(
		(P_MATRIX)0[(size_t *)param],
		0,
		(*(size_t *)1[(size_t *)param])++,
		(ptrdiff_t *)P2P_TNODE_BY(pitem)->pdata,
		sizeof(ptrdiff_t)
	);
	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsFillTablePuppet
 * Description:   Fill parsing table.
 * Parameters:
 *      pitem Pointer to each node in the edge list.
 *      param Pointer to a size_t array.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsFillTablePuppet(void * pitem, size_t param)
{
	P_EDGE pedge = (P_EDGE)((P_NODE_S)pitem)->pdata;
	P_MATRIX ptbl = (P_MATRIX)0[(size_t *)param];
	P_VERTEX_L pvtx = (P_VERTEX_L)2[(size_t *)param];

	ptrdiff_t * pi = svBinarySearch(&pedge->weight, ptbl->arrz.pdata, ptbl->col, sizeof(ptrdiff_t), cbfcmpPtrdifft);

	/* ACTION Shift and GOTO. */
	if (NULL != pi)
	{
		ptrdiff_t g = ((P_NFAELE)pedge->vid)->id + 1;
		strSetValueMatrix(ptbl, ((P_NFAELE)pvtx->vid)->id + 1, pi - (ptrdiff_t *)ptbl->arrz.pdata, &g, sizeof(ptrdiff_t));
	}

	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: FindBNFInSetGLst
 * Description:   Find BNF in set G.
 * Parameters:
 *      parrG Pointer to a BNF set.
 *       parr Pointer to a BNF array.
 * Return value:  Line number of BNF in set G.
 *                Especially, -1 means BNF not found.
 */
static ptrdiff_t FindBNFInSetGLst(P_ARRAY_Z parrG, P_ARRAY_Z parr)
{
	size_t i, j;
	for (i = 0; i < strLevelArrayZ(parrG); ++i)
	{
		P_ARRAY_Z parrBNFy = *(P_ARRAY_Z *)strLocateItemArrayZ(parrG, sizeof(P_ARRAY_Z), i);
		if (strLevelArrayZ(parr) != strLevelArrayZ(parrBNFy))
			continue;
		for (j = 0; j < strLevelArrayZ(parrBNFy); ++j)
		{
			if (((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), j))->name != ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), j))->name)
				goto Lbl_PassSearching;
		}
		return i;
		Lbl_PassSearching:
		;
	}
	return -1;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsFillReduce
 * Description:   Fill parsing table.
 * Parameters:
 *      pitem Pointer to each node in a set.
 *      param Pointer to a size_t array.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsFillReduce(void * pitem, size_t param)
{
	ptrdiff_t v = *(ptrdiff_t *)P2P_TNODE_BY(pitem)->pdata;
	P_MATRIX ptbl = (P_MATRIX)0[(size_t *)param];
	P_VERTEX_L pvtx = (P_VERTEX_L)2[(size_t *)param];
	P_ARRAY_Z parr = (P_ARRAY_Z)3[(size_t *)param];

	ptrdiff_t * pi = svBinarySearch(&v, ptbl->arrz.pdata, ptbl->col, sizeof(ptrdiff_t), cbfcmpPtrdifft);
	
	if (((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), 0))->name == -1)
		strSetValueMatrix(ptbl, ((P_NFAELE)pvtx->vid)->id + 1, pi - (ptrdiff_t *)ptbl->arrz.pdata, &v, sizeof(ptrdiff_t));
	else
	{
		v = -FindBNFInSetGLst((P_ARRAY_Z)1[(size_t *)param], (P_ARRAY_Z)3[(size_t *)param]);
		strSetValueMatrix(ptbl, ((P_NFAELE)pvtx->vid)->id + 1, pi - (ptrdiff_t *)ptbl->arrz.pdata, &v, sizeof(ptrdiff_t));
	}

	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: cbftvsFillTable
 * Description:   Fill parsing table.
 * Parameters:
 *      pitem Pointer to each VERTEX_L in NFA graph.
 *      param Pointer to a size_t array.
 * Return value:  CBF_CONTINUE only.
 */
static int cbftvsFillTable(void * pitem, size_t param)
{
	size_t a[4];
	P_VERTEX_L pvtx = (P_VERTEX_L)pitem;
	P_NFAELE pNFAEle = (P_NFAELE)pvtx->vid;

	memcpy(a, (const void *)param, sizeof(ptrdiff_t) * 2);
	a[2] = (size_t)pvtx;

	if (strLevelArrayZ(pNFAEle->parrBNFLst) == 1)
	{
		P_ARRAY_Z parr = *(P_ARRAY_Z *)strLocateItemArrayZ(((P_NFAELE)pvtx->vid)->parrBNFLst, sizeof(P_ARRAY_Z), 0);
		a[3] = (size_t)parr;
		setTraverseT(((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), strLevelArrayZ(parr) - 1))->m.pset, cbftvsFillReduce, (size_t)a, ETM_INORDER);
	}
	else
		strTraverseLinkedListSC_N(pvtx->adjlist, NULL, cbftvsFillTablePuppet, (size_t)a);
	
	return CBF_CONTINUE;
}

/* Attention:     This Is An Internal Function. No Interface for Library Users.
 * Function name: BuildLR1Table
 * Description:   Build CLR parsing table.
 * Parameters:
 *psetGmrSmbl Pointer to a set of symbol.
 *      parrG Pointer to BNF set.
 *      pgrpC Pointer to a NFA graph.
 *       vid0 Starting vertex ID in the NFA graph.
 * Return value:  Pointer to a matrix represented table.
 */
static P_MATRIX BuildLR1Table(P_SET_T psetGmrSmbl, P_ARRAY_Z parrG, P_GRAPH_L pgrpC, size_t vid0)
{
	size_t i = 0;

	P_MATRIX ptbl = NULL;

	grpBFSL(pgrpC, vid0, cbftvsCountNFAVertices, (size_t)&i);
	/* Count how many columns. */

	if (NULL != (ptbl = strCreateMatrix(i + 1, setSizeT(psetGmrSmbl) + 1, sizeof(ptrdiff_t))))
	{
		size_t a[2];
		ptrdiff_t t = 0;
		strSetMatrix(ptbl, &t, sizeof(ptrdiff_t));

		/* Fill table header. */
		i = 0;
		t = (ptrdiff_t)ACC;
		a[0] = (size_t)ptbl;
		a[1] = (size_t)&i;
		setTraverseT(psetGmrSmbl, cbftvsFillTableHeader, (size_t)a, ETM_INORDER);
		strSetValueMatrix(ptbl, 0, i, &t, sizeof(ptrdiff_t));
		svQuickSort(ptbl->arrz.pdata, ptbl->col, sizeof(ptrdiff_t), cbfcmpPtrdifft);

		/* Fill table. */
		a[0] = (size_t)ptbl;
		a[1] = (size_t)parrG;

		grpBFSL(pgrpC, vid0, cbftvsFillTable, (size_t)a);
	}

	return ptbl;
}

/* Function name: PrintCLRTable
 * Description:   Print CLR parsing table.
 * Parameter:
 *      ptbl  Pointer to a matrix represented table.
 * Return value:  N/A.
 */
void PrintCLRTable(P_MATRIX ptbl)
{
	if (NULL != ptbl)
	{
		size_t i, j;
		for (i = 0; i < ptbl->col; ++i)
			printf("(%zd)\t", *(ptrdiff_t *)strGetValueMatrix(NULL, ptbl, 0, i, sizeof(ptrdiff_t)));
		printf("\n");
		for (j = 1; j < ptbl->ln; ++j)
		{
			ptrdiff_t k;
			for (i = 0; i < ptbl->col; ++i)
			{
				k = *(ptrdiff_t *)strGetValueMatrix(NULL, ptbl, j, i, sizeof(ptrdiff_t));
				printf("%c%zd\t", k >= 0 ? ' ' : 'r', GETABS(k));
			}
			printf("\n");
		}
	}
}

/* Function name: ConstructCLRTable
 * Description:   Construct CLR parsing table.
 * Parameters:
 *     wcsbnf Pointer to a string of BNFs.
 *     pparrG Output a pointer to BNF set.
 * Return value:  Pointer to a matrix represented table.
 */
P_MATRIX ConstructCLRTable(wchar_t * wcsbnf, P_ARRAY_Z * pparrG)
{
	size_t siSymbolCtr = 1;
	size_t i, j, k = 0, prek = k;
	wchar_t buf[BUFSIZ] = { 0 };
	wchar_t * pbuf = buf;
	P_QUEUE_L pDFAQueue = NULL;
	P_TRIE_A ptrie = treCreateTrieA();
	P_ARRAY_Z parrBNFLst = strCreateArrayZ(1, sizeof(P_ARRAY_Z));
	P_ARRAY_Z * pparrbnf = (P_ARRAY_Z *)strLocateItemArrayZ(parrBNFLst, sizeof(P_ARRAY_Z), 0);
	P_SET_T psetGrammarSymbol = setCreateT();
	*(P_ARRAY_Z *)strLocateItemArrayZ(parrBNFLst, sizeof(P_ARRAY_Z), 0) = NULL;
	P_GRAPH_L pgrpC;
	ptrdiff_t x;
	P_MATRIX ptbl = NULL;

	pDFAQueue = LexCompile
	(
		L"(1|2|3|4|5|6|7|8|9)(0|1|2|3|4|5|6|7|8|9)*\n(A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|_)(A|B|C|D|E|F|G|H|I|J|K|L|M|N|O|P|Q|R|S|T|U|V|W|X|Y|Z|_)*\n:\n;\n\\n\n"
		/* 1------------------------------------------2---------------------------------------------------------------------------------------------------------------3--4---5 */
	);

	j = wcslen(wcsbnf);
	for (i = 0; i < j; ++i)
	{
		k = Lexer(pDFAQueue, wcsbnf[i]);
		switch (k)
		{
		case 3:
			*pbuf++ = wcsbnf[i];
			*pbuf = L'\0';
			prek = k;
			pbuf = buf;
			break;
		case 4:
			EmitSymbol(pparrbnf, ptrie, buf, prek, &siSymbolCtr, psetGrammarSymbol);

			strResizeArrayZ(parrBNFLst, strLevelArrayZ(parrBNFLst) + 1, sizeof(P_ARRAY_Z));
			pparrbnf = (P_ARRAY_Z *)strLocateItemArrayZ(parrBNFLst, sizeof(P_ARRAY_Z), strLevelArrayZ(parrBNFLst) - 1);
			*pparrbnf = NULL;

			prek = k;
			pbuf = buf;
			break;
		case 0:
			EmitSymbol(pparrbnf, ptrie, buf, prek, &siSymbolCtr, psetGrammarSymbol);

			prek = k;
			pbuf = buf;
			break;
		case 1:
		case 2:
			*pbuf++ = wcsbnf[i];
			*pbuf = L'\0';
			break;
		}
		prek = k;
	}
	LexDestroy(pDFAQueue);
	treDeleteTrieA(ptrie, sizeof(wchar_t));

	/* Shrink BNF list for 1. */
	strResizeArrayZ(parrBNFLst, strLevelArrayZ(parrBNFLst) - 1, sizeof(P_ARRAY_Z));

	pgrpC = ITEMS(parrBNFLst, psetGrammarSymbol, &i);

	x = -1;
	setRemoveT(psetGrammarSymbol, &x, sizeof(ptrdiff_t), cbfcmpPtrdifftAsSeq);

	ptbl = BuildLR1Table(psetGrammarSymbol, parrBNFLst, pgrpC, i);

	DestroyNFAGraph(pgrpC, i);

	setDeleteT(psetGrammarSymbol);

	if (pparrG)
		*pparrG = parrBNFLst;

	return ptbl;
}

/* Function name: CLRParse
 * Description:   The table driven parser.
 * Parameters:
 *     ptable Pointer to a matrix represented table.
 *      parrG Pointer to a BNF set.
 *      cbfgs Returns a symbol.
 *     cbfrdc Conduct a reducing procedure.
 *     cbferr Emmit an error.
 * Return value:  TRUE  Parsing OK.
 *                FALSE Parsing failure.
 */
BOOL CLRParse(P_MATRIX ptable, P_ARRAY_Z parrG, CBF_GetSymbol cbfgs, CBF_Reduce cbfrdc, CBF_Error cbferr)
{
	BOOL r = TRUE;

	P_STACK_L pstk = stkCreateL();

	ptrdiff_t a = cbfgs(), s = 1, x, y, * pi, t, A;

	stkPushL(pstk, &s, sizeof(ptrdiff_t));

	for (;;)
	{
		stkPeepL(&s, sizeof(ptrdiff_t), pstk);
		pi = svBinarySearch(&a, ptable->arrz.pdata, ptable->col, sizeof(ptrdiff_t), cbfcmpPtrdifft);

		if (NULL != pi)
		{
			strGetValueMatrix(&x, ptable, s, pi - (ptrdiff_t *)ptable->arrz.pdata, sizeof(ptrdiff_t));

			if (x > 0)
			{
				stkPushL(pstk, &x, sizeof(ptrdiff_t));
				a = cbfgs();
			}
			else if (x < 0)
			{
				stkPopL(&t, sizeof(ptrdiff_t), pstk);
				stkPeepL(&t, sizeof(ptrdiff_t), pstk);
				
				A = ((P_BNFELEMENT)strLocateItemArrayZ(*(P_ARRAY_Z *)strLocateItemArrayZ(parrG, sizeof(P_ARRAY_Z), -x), sizeof(BNFELEMENT), 0))->name;

				pi = svBinarySearch(&A, ptable->arrz.pdata, ptable->col, sizeof(ptrdiff_t), cbfcmpPtrdifft);
				if (NULL != pi)
				{
					strGetValueMatrix(&y, ptable, t, pi - (ptrdiff_t *)ptable->arrz.pdata, sizeof(ptrdiff_t));
					stkPushL(pstk, &y, sizeof(ptrdiff_t));

					if (CBF_CONTINUE != cbfrdc(-x))
					{
						r = TRUE;
						break;
					}
				}
				else
				{
					r = FALSE;
					break;
				}
			}
			else if ((ptrdiff_t)ACC == x)
			{
				r = TRUE;
				break;
			}
			else
			{
				if (CBF_CONTINUE != cbferr())
				{
					r = FALSE;
					break;
				}
			}
		}
		else
		{
			r = FALSE;
			break;
		}
	}

	stkDeleteL(pstk);

	return r;
}

/* Function name: DestroyCLRTable
 * Description:   Delete a CLR parsing table.
 * Parameter:
 *      ptbl Pointer to a matrix represented table.
 * Return value:  N/A.
 */
void DestroyCLRTable(P_MATRIX ptbl)
{
	strDeleteMatrix(ptbl);
}
