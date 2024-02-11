/*
 * Name:        test.h
 * Description: Compiler launcher.
 * Author:      cosh.cage#hotmail.com
 * File ID:     0208240241B0211250710L00064
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
	if (i < j)
	{
		return Lexer(pq, wc[i++]);
	}
	return 0;
}

int Reduce(ptrdiff_t n)
{
	printf("REDUCE! %ld\n", n);
	return CBF_TERMINATE;
}

void Error(void)
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
