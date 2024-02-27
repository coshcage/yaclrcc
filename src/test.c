/*
 * Name:        test.h
 * Description: Compiler launcher.
 * Author:      cosh.cage#hotmail.com
 * File ID:     0208240241B0211242136L00070
 * License:     GPLv2.
 */
#include <stdio.h>
#include <wchar.h>
#include "yaclrcc.h"

#define STR L"A : S;\nS : C C;\nC : 1 C;\nC : 2;\n"

size_t i, j;

const wchar_t * wc = L"ccccdddd";

P_QUEUE_L pq;

ptrdiff_t GetSymbol(void)
{
	ptrdiff_t r = 0;
	wchar_t c;
	if (i <= j)
	{
		c = wc[i++];
		if (c != L'\0')
			r = Lexer(pq, c);
		else
			r = (ptrdiff_t)ACC;
	}
	return r;
}

int Reduce(ptrdiff_t n)
{
	printf("REDUCE! %ld\n", n);
	return CBF_TERMINATE;
}

int Error(void)
{
	printf("ERROR!\n");
	return CBF_TERMINATE;
}

int main()
{
	
	P_MATRIX ptbl;
	P_ARRAY_Z parrG = NULL;
	pq = LexCompile(L"c\nd\n");

	ptbl = ConstructCLRTable(STR, &parrG);

	PrintCLRTable(ptbl);

	j = wcslen(wc);
	
	CLRParse(ptbl, parrG, GetSymbol, Reduce, Error);

	LexDestroy(pq);

	DestroyParrList(parrG);

	DestroyCLRTable(ptbl);

	return 0;
}
