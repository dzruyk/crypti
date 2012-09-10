#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "macros.h"

#define PRIM_NUM (sizeof(prime_numbers)/sizeof(unsigned int))

static const unsigned int prime_numbers[] = {
	5,
	11,
	19,
	37,
	73,
	109,
	163,
	251,
	367,
	557,
	823,
	1237,
	1861,
	2777,
	4177,
	6247,
	9371,
	14057,
	21089,
	31627,
	47431,
	71143,
	106721,
	160073,
	240101,
	360163,
	540217,
	810343,
	1215497,
	1823231,
	2734867,
	4102283,
	6153409,
	9230113,
	13845163
};

unsigned int prime_nearest(size_t size)
{
	int i;

	for (i = 0; i < PRIM_NUM; i++) {
		if (size <= prime_numbers[i])
			return prime_numbers[i];
	}

	return prime_numbers[PRIM_NUM-1];
}
