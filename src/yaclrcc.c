/*
 * Name:        yaclrcc.h
 * Description: Yet another CLR compiler compiler.
 * Author:      cosh.cage#hotmail.com
 * File ID:     0208240241B0208240241L00977
 * License:     GPLv2.
 */
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h>
#include <wchar.h>
#include <stdlib.h>
#include "StoneValley/src/svstring.h"
#include "StoneValley/src/svqueue.h"
#include "StoneValley/src/svtree.h"
#include "StoneValley/src/svset.h"
#include "StoneValley/src/svgraph.h"
#include "StoneValley/src/svstack.h"
#include "svregex.h"
#include "yaclrcc.h"

#define GETABS(num) ((num) >=0 ? (num) : -(num))

typedef struct _st_DFASEQ
{
	size_t  num;
	P_DFA   pdfa;
	size_t  curstate;
} DFASEQ, * P_DFASEQ;

typedef enum en_BNFType
{
	BT_TERMINATOR = 1,
	BT_NONTERMINATOR
} BNFType;

typedef struct st_BNFELEMENT
{
	ptrdiff_t name;
	struct st_mark
	{
		BOOL bmark;
		P_SET_T pset;
	} m;
	
} BNFELEMENT, * P_BNFELEMENT;

typedef struct st_NFAELE
{
	size_t    id;
	P_ARRAY_Z parrBNFLst;
} NFAELE, * P_NFAELE;

typedef struct st_TBLELEMENT
{
	ptrdiff_t name;
	void (*action)(void *);
} TBLELEMENT, * P_TBLELEMENT;

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

size_t Lexer(P_QUEUE_L pq, wchar_t wc)
{
	size_t a[2];

	a[0] = wc;
	a[1] = 0;

	strTraverseLinkedListSC_N(pq->pfront, NULL, cbftvsLexerPuppet, (size_t)a);

	return a[1];
}

static int cbftvsLexerDestroyPuppet(void * pitem, size_t param)
{
	P_DFASEQ pdfaq = (P_DFASEQ)((P_NODE_S)pitem)->pdata;

	DWC4100(param);

	DestroyDFA(pdfaq->pdfa);
	return CBF_CONTINUE;
}

static void LexDestroy(P_QUEUE_L pq)
{
	strTraverseLinkedListSC_N(pq->pfront, NULL, cbftvsLexerDestroyPuppet, 0);
	queDeleteL(pq);
}

static int cbfcmpWChar_t(const void * px, const void * py)
{
	return (int)*(wchar_t *)px - (int)*(wchar_t *)py;
}

static int cbftvsCmpPtrdifft(const void * px, const void * py)
{
	ptrdiff_t x = *(ptrdiff_t *)px;
	ptrdiff_t y = *(ptrdiff_t *)py;
	if (x > y) return 1;
	if (x < y) return -1;
	return 0;
}

static int cbftvsCmpPtrdifftAsSeq(const void * px, const void * py)
{
	ptrdiff_t x = *(ptrdiff_t *)px;
	ptrdiff_t y = *(ptrdiff_t *)py;
	if (x < 0)
		x = -((ptrdiff_t)ACC + x);
	if (y < 0)
		y = -((ptrdiff_t)ACC + y);
	return x - y;
}

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
		setInsertT(psetGmrSmbl, &be.name, sizeof(ptrdiff_t), cbftvsCmpPtrdifftAsSeq);
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

static int cbftvsDestroyParrlistPuppet(void * pitem, size_t param)
{
	DWC4100(param);
	if (NULL != ((P_BNFELEMENT)pitem)->m.pset)
		setDeleteT(((P_BNFELEMENT)pitem)->m.pset);
	return CBF_TERMINATE;
}

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

void DestroyParrList(P_ARRAY_Z parrlist)
{
	strTraverseArrayZ(parrlist, sizeof(P_ARRAY_Z), cbftvsDestroyParrlist, 0, FALSE);
	strDeleteArrayZ(parrlist);
}

static int cbftvsPrintParrlistPuppet(void * pitem, size_t param)
{
	P_BNFELEMENT pbe = (P_BNFELEMENT)pitem;
	DWC4100(param);
	printf("(%zd) ", pbe->name);
	printf("%c", pbe->m.bmark ? '.' : ' ');
	return CBF_CONTINUE;
}

static int cbftvsPrintSetCLR(void * pitem, size_t param)
{
	DWC4100(param);
	printf("%zd, ", *(ptrdiff_t *)P2P_TNODE_BY(pitem)->pdata);
	return CBF_CONTINUE;
}

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

void PrintParrList(P_ARRAY_Z parrlist)
{
	strTraverseArrayZ(parrlist, sizeof(P_ARRAY_Z), cbftvsPrintParrlist, 0, FALSE);
}

static P_SET_T FIRST(P_ARRAY_Z parrBNFLst, ptrdiff_t a)
{
	P_SET_T pset = setCreateT();
	if (a > 0)	/* a is a terminator. */
		setInsertT(pset, &a, sizeof(ptrdiff_t), cbftvsCmpPtrdifft);
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
				setInsertT(pset, &((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), 1))->name, sizeof(ptrdiff_t), cbftvsCmpPtrdifft);
		}
	}
	return pset;
}

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
			if (!setIsEqualT(((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFx) - 1))->m.pset, ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFy) - 1))->m.pset, cbftvsCmpPtrdifft))
				return FALSE;
		}
	}
	return TRUE;
}

static int cbfCompareBNFs(void * pitem, size_t param)
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
			if (!setIsEqualT(((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFx) - 1))->m.pset, ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFy) - 1))->m.pset, cbftvsCmpPtrdifft))
				return FALSE;
		}
	}
	return TRUE;
}

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
			if (!setIsEqualT(((P_BNFELEMENT)strLocateItemArrayZ(parrBNFx, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFx) - 1))->m.pset, ((P_BNFELEMENT)strLocateItemArrayZ(parrBNFy, sizeof(BNFELEMENT), strLevelArrayZ(parrBNFy) - 1))->m.pset, cbftvsCmpPtrdifft))
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
			
			grpBFSL(pgrpC, (size_t)pNFAEle0, cbfCompareBNFs, (size_t)a);

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
			setInsertT(((P_BNFELEMENT)strLocateItemArrayZ(*(P_ARRAY_Z *)strLocateItemArrayZ(parrI, sizeof(P_ARRAY_Z), 0), sizeof(BNFELEMENT), 1))->m.pset, &i, sizeof(ptrdiff_t), cbftvsCmpPtrdifft);
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

static int cbftvsDestroyNFAGraph(void * pitem, size_t param)
{
	P_VERTEX_L pvtx = (P_VERTEX_L)pitem;
	P_NFAELE pNFAEle = (P_NFAELE)pvtx->vid;
	DWC4100(param);
	DestroyParrList(pNFAEle->parrBNFLst);
	free(pNFAEle);

	return CBF_CONTINUE;
}

static void DestroyNFAGraph(P_GRAPH_L pgrpC, size_t vid0)
{
	grpBFSL(pgrpC, vid0, cbftvsDestroyNFAGraph, 0);
}

static int cbftvsCountNFAVertices(void * pitem, size_t param)
{
	0[(size_t *)param]++;
	DWC4100(pitem);
	return CBF_CONTINUE;
}

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

static int cbftvsFillTablePuppet(void * pitem, size_t param)
{
	P_EDGE pedge = (P_EDGE)((P_NODE_S)pitem)->pdata;
	P_MATRIX ptbl = (P_MATRIX)0[(size_t *)param];
	P_VERTEX_L pvtx = (P_VERTEX_L)2[(size_t *)param];

	ptrdiff_t * pi = svBinarySearch(&pedge->weight, ptbl->arrz.pdata, ptbl->col, sizeof(ptrdiff_t), cbftvsCmpPtrdifft);

	/* ACTION Shift and GOTO. */
	if (NULL != pi)
	{
		ptrdiff_t g = ((P_NFAELE)pedge->vid)->id + 1;
		strSetValueMatrix(ptbl, ((P_NFAELE)pvtx->vid)->id + 1, pi - (ptrdiff_t *)ptbl->arrz.pdata, &g, sizeof(ptrdiff_t));
	}

	return CBF_CONTINUE;
}

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

static int cbftvsFillReduce(void * pitem, size_t param)
{
	ptrdiff_t v = *(ptrdiff_t *)P2P_TNODE_BY(pitem)->pdata;
	P_MATRIX ptbl = (P_MATRIX)0[(size_t *)param];
	P_VERTEX_L pvtx = (P_VERTEX_L)2[(size_t *)param];
	P_ARRAY_Z parr = (P_ARRAY_Z)3[(size_t *)param];

	ptrdiff_t * pi = svBinarySearch(&v, ptbl->arrz.pdata, ptbl->col, sizeof(ptrdiff_t), cbftvsCmpPtrdifft);
	
	if (((P_BNFELEMENT)strLocateItemArrayZ(parr, sizeof(BNFELEMENT), 0))->name == -1)
		strSetValueMatrix(ptbl, ((P_NFAELE)pvtx->vid)->id + 1, pi - (ptrdiff_t *)ptbl->arrz.pdata, &v, sizeof(ptrdiff_t));
	else
	{
		v = -FindBNFInSetGLst((P_ARRAY_Z)1[(size_t *)param], (P_ARRAY_Z)3[(size_t *)param]);
		strSetValueMatrix(ptbl, ((P_NFAELE)pvtx->vid)->id + 1, pi - (ptrdiff_t *)ptbl->arrz.pdata, &v, sizeof(ptrdiff_t));
	}

	return CBF_CONTINUE;
}

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
		svQuickSort(ptbl->arrz.pdata, ptbl->col, sizeof(ptrdiff_t), cbftvsCmpPtrdifft);

		/* Fill table. */
		a[0] = (size_t)ptbl;
		a[1] = (size_t)parrG;

		grpBFSL(pgrpC, vid0, cbftvsFillTable, (size_t)a);
	}

	return ptbl;
}

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
	setRemoveT(psetGrammarSymbol, &x, sizeof(ptrdiff_t), cbftvsCmpPtrdifftAsSeq);

	ptbl = BuildLR1Table(psetGrammarSymbol, parrBNFLst, pgrpC, i);

	DestroyNFAGraph(pgrpC, i);

	setDeleteT(psetGrammarSymbol);

	if (pparrG)
		*pparrG = parrBNFLst;

	return ptbl;
}

static BOOL CLRParse(P_MATRIX ptable, P_ARRAY_Z parrG, CBF_GetSymbol cbfgs, CBF_Reduce cbfrdc, CBF_Error cbferr)
{
	BOOL r = TRUE;

	P_STACK_L pstk = stkCreateL();

	ptrdiff_t a = cbfgs(), s = 1, x, y, * pi, t, A;

	stkPushL(pstk, &s, sizeof(ptrdiff_t));

	for (;;)
	{
		stkPeepL(&s, sizeof(ptrdiff_t), pstk);
		pi = svBinarySearch(&a, ptable->arrz.pdata, ptable->col, sizeof(ptrdiff_t), cbftvsCmpPtrdifft);
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

				pi = svBinarySearch(&A, ptable->arrz.pdata, ptable->col, sizeof(ptrdiff_t), cbftvsCmpPtrdifft);
				if (NULL != pi)
				{
					strGetValueMatrix(&y, ptable, t, pi - (ptrdiff_t *)ptable->arrz.pdata, sizeof(ptrdiff_t));
					stkPushL(pstk, &y, sizeof(ptrdiff_t));
					cbfrdc(A);
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
				cbferr();
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

void DestroyCLRTable(P_MATRIX ptbl)
{
	strDeleteMatrix(ptbl);
}

