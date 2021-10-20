/* $Id: bigdigits.h $ */

/******************** SHORT COPYRIGHT NOTICE**************************
This source code is part of the BigDigits multiple-precision
arithmetic library Version 2.2 originally written by David Ireland,
copyright (c) 2001-8 D.I. Management Services Pty Limited, all rights
reserved. It is provided "as is" with no warranties. You may use
this software under the terms of the full copyright notice
"bigdigitsCopyright.txt" that should have been included with this
library or can be obtained from <www.di-mgt.com.au/bigdigits.html>.
This notice must always be retained in any copy.
******************* END OF COPYRIGHT NOTICE***************************/
/*
	Last updated:
	$Date: 2008-07-31 12:54:00 $
	$Revision: 2.2.0 $
	$Author: dai $
*/

/* Interface to BigDigits "mp" functions */

#ifndef BIGDIGITS_H_
#define BIGDIGITS_H_ 1

#include "bigdtypes.h"

/**** USER CONFIGURABLE SECTION ****/

/* Define type and size of DIGIT */

/* [v2.1] Changed to use C99 exact-width types so it will compile with 64-bit compilers. */
/* [v2.2] Put macros for exact-width types in separate file "bigdtypes.h" */

typedef uint32_t DIGIT_T;
typedef uint16_t HALF_DIGIT_T;

/* Sizes to match */
#define MAX_DIGIT 0xffffffffUL
#define MAX_HALF_DIGIT 0xffffUL	/* NB 'L' */
#define BITS_PER_DIGIT 32
#define HIBITMASK 0x80000000UL

/*	[v2.2] added option to avoid allocating temp storage
	and use fixed automatic arrays instead.
	Define NO_ALLOCS to invoke this.
	Only applicable to mp functions. Do not use with bd.
*/
/* Specifiy the maximum number of digits allowed in a temp mp array
   -- ignored unless NO_ALLOCS is defined */ 
#define MAX_FIXED_DIGITS (1024 / BITS_PER_DIGIT)

/**** END OF USER CONFIGURABLE SECTION ****/

#define BITS_PER_HALF_DIGIT (BITS_PER_DIGIT / 2)
#define BYTES_PER_DIGIT (BITS_PER_DIGIT / 8)

/* Useful macros */
#define LOHALF(x) ((DIGIT_T)((x) & MAX_HALF_DIGIT))
#define HIHALF(x) ((DIGIT_T)((x) >> BITS_PER_HALF_DIGIT & MAX_HALF_DIGIT))
#define TOHIGH(x) ((DIGIT_T)((x) << BITS_PER_HALF_DIGIT))

#define ISODD(x) ((x) & 0x1)
#define ISEVEN(x) (!ISODD(x))

#define mpISODD(x, n) (x[0] & 0x1)
#define mpISEVEN(x, n) (!(x[0] & 0x1))

#define mpNEXTBITMASK(mask, n) do{if(mask==1){mask=HIBITMASK;n--;}else{mask>>=1;}}while(0)


/*	
 * Multiple precision calculations	
 * Using known, equal ndigits
 * except where noted
*/

/*************************/
/* ARITHMETIC OPERATIONS */
/*************************/

DIGIT_T mpAdd(DIGIT_T w[], const DIGIT_T u[], const DIGIT_T v[], size_t ndigits);
	/* Computes w = u + v, returns carry */

int mpMultiply(DIGIT_T w[], const DIGIT_T u[], const DIGIT_T v[], size_t ndigits);
	/* Computes product w = u * v 
	   u, v = ndigits long; w = 2 * ndigits long */


/*************************/
/* COMPARISON OPERATIONS */
/*************************/

int mpCompare(const DIGIT_T a[], const DIGIT_T b[], size_t ndigits);
	/* Returns sign of (a - b) */


/****************************/
/* NUMBER THEORY OPERATIONS */
/****************************/

/* [v2.2] removed `const' restriction on m[] for mpModMult and mpModExp */

int mpModExp(DIGIT_T y[], const DIGIT_T x[], const DIGIT_T e[], DIGIT_T m[], size_t ndigits);
	/* Computes y = x^e mod m */


/**********************/
/* BITWISE OPERATIONS */
/**********************/

DIGIT_T mpShiftLeft(DIGIT_T a[], const DIGIT_T b[], size_t x, size_t ndigits);
	/* Computes a = b << x */

DIGIT_T mpShiftRight(DIGIT_T a[], const DIGIT_T b[], size_t x, size_t ndigits);
	/* Computes a = b >> x */


/*************************/
/* ASSIGNMENT OPERATIONS */
/*************************/

DIGIT_T mpSetZero(volatile DIGIT_T a[], size_t ndigits);
	/* Sets a = 0 */

void mpSetDigit(DIGIT_T a[], DIGIT_T d, size_t ndigits);
	/* Sets a = d where d is a single digit */

void mpSetEqual(DIGIT_T a[], const DIGIT_T b[], size_t ndigits);
	/* Sets a = b */


/**********************/
/* OTHER MP UTILITIES */
/**********************/

size_t mpSizeof(const DIGIT_T a[], size_t ndigits);
	/* Returns size i.e. number of significant non-zero digits in a */

/**************************************/
/* CORE SINGLE PRECISION CALCULATIONS */
/* (double where necessary)           */
/**************************************/

/* NOTE spMultiply and spDivide are used by almost all mp functions. 
   Using the Intel MASM alternatives gives significant speed improvements
   -- to use, define USE_SPASM as a preprocessor directive.
   [v2.2] Removed references to spasm* versions.
*/

int spMultiply(DIGIT_T p[2], DIGIT_T x, DIGIT_T y);
	/* Computes p = x * y */

DIGIT_T spDivide(DIGIT_T *q, DIGIT_T *r, const DIGIT_T u[2], DIGIT_T v);
	/* Computes quotient q = u / v, remainder r = u mod v */

/****************************/
/* RANDOM NUMBER FUNCTIONS  */
/* CAUTION: NOT thread-safe */
/****************************/


/* [Version 2.1: spBetterRand moved to spRandom.h] */

/*************************************************/
/* MEMORY ALLOCATION FUNCTIONS - USED INTERNALLY */
/*************************************************/
/* [v2.2] added option to avoid memory allocation if NO_ALLOCS is defined */
void mpFail(char *msg);

#define mpDESTROY(b, n) do{if(b)mpSetZero(b,n);}while(0)


#endif	/* BIGDIGITS_H_ */
