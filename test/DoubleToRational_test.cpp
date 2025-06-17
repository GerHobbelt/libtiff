
/*
 * Copyright (c) 2012, Frank Warmerdam <warmerdam@pobox.com>
 *
 * Permission to use, copy, modify, distribute, and sell this software and 
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the names of
 * Sam Leffler and Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Sam Leffler and Silicon Graphics.
 * 
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY 
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.  
 * 
 * IN NO EVENT SHALL SAM LEFFLER OR SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF 
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE 
 * OF THIS SOFTWARE.
 */

/*
 * TIFF Library
 *
 * -- Module copied from custom_dir.c --
 *===========  Purpose ===================================================================================
 * Test DoubleToRational() function interface and accuracy
 *
 */

#define DOUBLE2RAT_DEBUGOUTPUT 1

#include "tif_config.h"
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#ifdef HAVE_UNISTD_H 
# include <unistd.h> 
#endif 

#include "tiffio.h"
#include "tiffiop.h"
#include "tif_dir.h"
#include "tifftest.h"

#include "ElapseTimer.hpp"

 /*--: Rational2Double: limits.h for definition of UINT32_MAX etc. */
//#include <stdio.h>
//#include <stdlib.h>
//#include <math.h>
#include <float.h>
#include <limits.h>
#include <stdint.h>

/* Defines from  tif_dirwrite.c */
void DoubleToRational(const double value, uint32_t *num, uint32_t *denom);
void DoubleToSrational(const double value, int32_t *num, int32_t *denom);
void DoubleToRational_direct(const double value, uint32_t *num, uint32_t *denom);
void DoubleToSrational_direct(const double value, int32_t *num, int32_t *denom);

/* Defines from original functions at end of this module */
void DoubleToRational2(const double value, uint32_t *num, uint32_t *denom);
void DoubleToSrational2(const double value, int32_t *num, int32_t *denom);
void DoubleToRationalOld(const double value, uint32_t *num, uint32_t *denom);
void DoubleToSrationalOld(const double value, int32_t *num, int32_t *denom);


void printFile(FILE* fpFile, int idx, double dblIn, long long uNum, long long uDenom, long long uNumOld, long long uDenomOld, long long uNum3, long long uDenom3)
{
	double auxDouble, auxDoubleOld;
	double dblDiff, dblDiffToDouble, dblDiffToDoubleOld, dblDiffToDouble3;
	int		oldBetter;
	if (fpFile != NULL) {
		auxDouble = (double)uNum / (double)uDenom;
		auxDoubleOld = (double)uNumOld / (double)uDenomOld;
		dblDiff = auxDoubleOld - auxDouble;
		dblDiffToDouble = auxDouble - dblIn;
		dblDiffToDoubleOld = auxDoubleOld - dblIn;
		dblDiffToDouble3 = (double)uNum3 / (double)uDenom3 - dblIn;
		oldBetter = 0;
		if (fabs(dblDiffToDouble) < fabs(dblDiffToDoubleOld))
			oldBetter = 1;
		if (fabs(dblDiffToDouble) > fabs(dblDiffToDoubleOld))
			oldBetter = 2;
		if (fabs(dblDiffToDouble) > fabs(dblDiffToDouble3) && fabs(dblDiffToDoubleOld) > fabs(dblDiffToDouble3))
			oldBetter = 3;
		fprintf(fpFile, "\n%4d:%26.12f -> %14lld / %12lld = %24.12f diff=%15.8e |  %14lld / %12lld = %24.12f diff=%15.8e | %d", idx, dblIn, uNum, uDenom, auxDouble, dblDiffToDouble, uNumOld, uDenomOld, auxDoubleOld, dblDiffToDoubleOld, oldBetter);
		if (oldBetter == 3)
			fprintf(fpFile, " | %14lld / %12lld = %24.12f diff=%15.8e", uNum3, uDenom3, (double)uNum3 / (double)uDenom3, dblDiffToDouble3);
	}
}

int
main(void)
{
	static const char filename[] = "DoubleToRational_Test.txt";
	FILE *			fpFile;

	int				ret = 0, i;
	double			dblIn;
	double			auxDouble = 0.0;
	double			auxDoubleOld = 0.0;

	uint32_t			uNum, uDenom;
	int32_t			sNum, sDenom;
	uint32_t			uNumOld, uDenomOld;
	int32_t			sNumOld, sDenomOld;
	uint32_t			uNum3, uDenom3;
	int32_t			sNum3, sDenom3;

#define N_SIZE  2000

	double		auxDoubleArrayW[2 * N_SIZE];
	double		dblDiff;
	double		dblDiffToDouble, dblDiffToDoubleOld;

#define RATIONAL_EPS (1.0/30000.0) /* reduced difference of rational values, approx 3.3e-5 */

	ElapseTimer eTimer;

	double		dblElapsedTm[30*N_SIZE];
	int			iTm = 0;

	/*-- Fill test data arrays for writing ----------- */
	/* special values */
	i = 0;
	auxDoubleArrayW[i] = 5.0 / 2.0; i++;
	auxDoubleArrayW[i] = 13.0 / 7.0; i++;
	auxDoubleArrayW[i] = 1.0 / (double)0xFFFFFFFFU; i++;
	auxDoubleArrayW[i] = (double)0x7FFFFFFFU; i++;
	auxDoubleArrayW[i] = (double)0xFFFFFFFFU; i++;
	auxDoubleArrayW[i] = 3.1415926535897932384626433832795f; i++; /* PI in float-precision */
	auxDoubleArrayW[i] = 3.1415926535897932384626433832795; i++;  /* PI in double-precision */
	auxDoubleArrayW[i] = sqrt(2.0); i++;
	auxDoubleArrayW[i] = (13.0 / 7.0)*34535353.0; i++;
	assert(i < N_SIZE); /* check, if i is still in range of array */
	for (; i < N_SIZE/2; i++) {
		auxDoubleArrayW[i] = (double)((i + 1) * 36897) / 4.5697;
	}
int stop = 1817; /*debugging*/
	for (; i < N_SIZE; i++) {
		if (i == stop) /*debugging*/
			int a = 2;
		auxDouble = (4.0*(double)UINT32_MAX / (double)N_SIZE);
		auxDoubleArrayW[i] = (double)i * auxDouble / 4.3;
		auxDoubleArrayW[i] = (double)(i * 4.0 * UINT32_MAX/N_SIZE) / 4.3;
		//if (auxDoubleArrayW[i] > 3633742097.0)
	}

	/*-- Output file -- */
	fpFile = NULL;
	fpFile = fopen(filename, "at");
	fprintf(fpFile, "\n\n=======================================================");


	/*-- compare old and new routines --*/
	for (i = 0; i < N_SIZE; i++) {
		if (i==stop) /*debugging*/
				int b = 2;
		/*-- UN-Signed --*/
		dblIn = auxDoubleArrayW[i];
		eTimer.Duration(true);
		DoubleToRational2(auxDoubleArrayW[i], &uNum, &uDenom);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		DoubleToRationalOld(auxDoubleArrayW[i], &uNumOld, &uDenomOld);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		DoubleToRational_direct(auxDoubleArrayW[i], &uNum3, &uDenom3);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		auxDouble = (double)uNum / (double)uDenom;
		auxDoubleOld = (double)uNumOld / (double)uDenomOld;
		dblDiff = auxDoubleOld - auxDouble;
		dblDiffToDouble = auxDouble - dblIn;
		dblDiffToDoubleOld = auxDoubleOld - dblIn;
		//if (uNum != uNumOld || uDenom != uDenomOld || fabs(dblDiff) > RATIONAL_EPS || _isnan(dblDiff)) {
		//	printf("DoubleToRational unterschied für %f (%f): New= %d / %d; Old = %d / %d", auxDoubleArrayW[i], dblDiff, uNum, uDenom, uNumOld, uDenomOld);
		//}
		printFile(fpFile, i, dblIn, uNum, uDenom, uNumOld, uDenomOld, uNum3, uDenom3);
		/*-- Reciprocal --*/
		dblIn = 1.0 / auxDoubleArrayW[i];
		eTimer.Duration(true);
		DoubleToRational2(dblIn, &uNum, &uDenom);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		DoubleToRationalOld(dblIn, &uNumOld, &uDenomOld);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		DoubleToRational_direct(auxDoubleArrayW[i], &uNum3, &uDenom3);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		auxDouble = (double)uNum / (double)uDenom;
		auxDoubleOld = (double)uNumOld / (double)uDenomOld;
		dblDiff = auxDoubleOld - auxDouble;
		dblDiffToDouble = auxDouble - dblIn;
		dblDiffToDoubleOld = auxDoubleOld - dblIn;
		//if (uNum != uNumOld || uDenom != uDenomOld || fabs(dblDiff) > RATIONAL_EPS || _isnan(dblDiff)) {
		//	printf("DoubleToRational unterschied für %f (%f): New= %d / %d; Old = %d / %d", dblIn, dblDiff, uNum, uDenom, uNumOld, uDenomOld);
		//}
		printFile(fpFile, i, dblIn, uNum, uDenom, uNumOld, uDenomOld, uNum3, uDenom3);

		/*-- Signed --*/
		dblIn = -1.0 * auxDoubleArrayW[i];
		eTimer.Duration(true);
		DoubleToSrational2(dblIn, &sNum, &sDenom);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		DoubleToSrationalOld(dblIn, &sNumOld, &sDenomOld);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		DoubleToSrational_direct(auxDoubleArrayW[i], &sNum3, &sDenom3);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		auxDouble = (double)sNum / (double)sDenom;
		auxDoubleOld = (double)sNumOld / (double)sDenomOld;
		dblDiff = auxDoubleOld - auxDouble;
		dblDiffToDouble = auxDouble - dblIn;
		dblDiffToDoubleOld = auxDoubleOld - dblIn;
		//if (sNum != sNumOld || sDenom != sDenomOld || fabs(dblDiff) > RATIONAL_EPS || _isnan(dblDiff)) {
		//	printf("DoubleToRational unterschied für %f (%f): New= %d / %d; Old = %d / %d", dblIn, dblDiff, sNum, sDenom, sNumOld, sDenomOld);
		//}
		printFile(fpFile, i, dblIn, sNum, sDenom, sNumOld, sDenomOld, sNum3, sDenom3);
		/*-- Signed - Reciprocal --*/
		dblIn = -1.0 / auxDoubleArrayW[i];
		eTimer.Duration(true);
		DoubleToSrational2(dblIn, &sNum, &sDenom);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		DoubleToSrationalOld(dblIn, &sNumOld, &sDenomOld);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		DoubleToSrational_direct(auxDoubleArrayW[i], &sNum3, &sDenom3);
		dblElapsedTm[iTm] = eTimer.Duration(true); iTm++;
		auxDouble = (double)sNum / (double)sDenom;
		auxDoubleOld = (double)sNumOld / (double)sDenomOld;
		dblDiff = auxDoubleOld - auxDouble;
		dblDiffToDouble = auxDouble - dblIn;
		dblDiffToDoubleOld = auxDoubleOld - dblIn;
		//if (sNum != sNumOld || sDenom != sDenomOld || fabs(dblDiff) > RATIONAL_EPS || _isnan(dblDiff)) {
		//	printf("DoubleToRational unterschied für %f (%f): New= %d / %d; Old = %d / %d", dblIn, dblDiff, sNum, sDenom, sNumOld, sDenomOld);
		//}
		printFile(fpFile, i, dblIn, sNum, sDenom, sNumOld, sDenomOld, sNum3, sDenom3);
	}



	fclose(fpFile);

	return ret;
} /* main() */




   /** -----  Rational2Double: Double To Rational Conversion ----------------------------------------------------------
   * There is a mathematical theorem to convert real numbers into a rational (integer fraction) number.
   * This is called "continuous fraction" which uses the Euclidean algorithm to find the greatest common divisor (GCD).
   *  (ref. e.g. https://de.wikipedia.org/wiki/Kettenbruch or https://en.wikipedia.org/wiki/Continued_fraction
   *             https://en.wikipedia.org/wiki/Euclidean_algorithm)
   * The following functions implement the 
   * - ToRationalEuclideanGCD()		auxiliary function which mainly implements euclidian GCD
   * - DoubleToRational()			conversion function for un-signed rationals
   * - DoubleToSrational()			conversion function for signed rationals
   ------------------------------------------------------------------------------------------------------------------*/

/**---- ToRationalEuclideanGCD() -----------------------------------------
 * Calculates the rational fractional of a double input value
 * using the Euclidean algorithm to find the greatest common divisor (GCD)
------------------------------------------------------------------------*/
void ToRationalEuclideanGCD(const double value, int blnUseSignedRange, int blnUseSmallRange, uint64_t *ullNum, uint64_t *ullDenom)
{
	/* Internally, the integer variables can be bigger than the external ones,
	* as long as the result will fit into the external variable size.
	*/
	uint64_t val, numSum[3] = { 0, 1, 0 }, denomSum[3] = { 1, 0, 0 };
	uint64_t aux, bigNum, bigDenom;
	uint64_t returnLimit;
	int i;
	uint64_t nMax;
	double fMax;
	uint32_t maxDenom;
	bool blnUseReciprocal;
	/*-- nMax and fMax defines the initial accuracy of the starting fractional,
	*   or better, the highest used integer numbers used within the starting fractional (bigNum/bigDenom).
	*   There are two approaches, which can accidentially lead to different accuracies just depending on the value.
	*   Therefore, blnUseSmallRange steers this behaviour.
	*   For long long nMax = ((9223372036854775807-1)/2); for long nMax = ((2147483647-1)/2);
	*/
	if (blnUseSmallRange) {
		nMax = (uint64_t)((UINT32_MAX - 1) / 2);
	}
	else {
		nMax = (UINT64_MAX - 1) / 2;
	}
	fMax = (double)nMax;

	/*-- For the Euclidean GCD define the denominator range, so that it stays within size of uint32_t variables.
	*   maxDenom should be INT32_MAX for negative values and UINT32_MAX for positive ones.
	*   Also the final returned value of ullNum and ullDenom is limited according to signed- or unsigned-range.
	*/
	if (blnUseSignedRange) {
		maxDenom = INT32_MAX;
		returnLimit = INT32_MAX;
	}
	else {
		maxDenom = UINT32_MAX;
		returnLimit = UINT32_MAX;
	}

	/*-- Bigger values are treated with more accuracy. Therefore check and use reciprocal. --*/
	blnUseReciprocal = false;
    auto v = value;
	//if (v < 1.0) {
	//	v = 1.0 / v;
	//	blnUseReciprocal = true;
	//}

	/*-- First generate a rational fraction (bigNum/bigDenom) which represents the value
	*   as a rational number with the highest accuracy. Therefore, uint64_t (uint64) is needed.
	*   This rational fraction is then reduced using the Euclidean algorithm to find the greatest common divisor (GCD).
	*   bigNum   = big numinator of value without fraction (or cut residual fraction)
	*   bigDenom = big denominator of value
	*-- Break-criteria so that uint64 cast to "bigNum" introduces no error and bigDenom has no overflow,
	*   and stop with enlargement of fraction when the double-value of it reaches an integer number without fractional part.
	*/
	bigDenom = 1;
	while ((v != floor(v)) && (v < fMax) && (bigDenom < nMax)) {
		bigDenom <<= 1;
		v *= 2;
	}
	bigNum = (uint64_t)v;

	/*-- Start Euclidean algorithm to find the greatest common divisor (GCD) -- */
#define MAX_ITERATIONS 64
	for (i = 0; i < MAX_ITERATIONS; i++) {
		/* if bigDenom is not zero, calculate integer part of fraction. */
		if (bigDenom == 0) {
			val = 0;
			if (i > 0) break;	/* if bigDenom is zero, exit loop, but execute loop just once */
		}
		else {
			val = bigNum / bigDenom;
		}

		/* Set bigDenom to reminder of bigNum/bigDenom and bigNum to previous denominator bigDenom. */
		aux = bigNum;
		bigNum = bigDenom;
		bigDenom = aux % bigDenom;

		/* calculate next denominator and check for its given maximum */
		aux = val;
		if (denomSum[1] * val + denomSum[0] >= maxDenom) {
			aux = (maxDenom - denomSum[0]) / denomSum[1];
			if (aux * 2 >= val || denomSum[1] >= maxDenom)
				i = (MAX_ITERATIONS + 1);			/* exit but execute rest of for-loop */
			else
				break;
		}
		/* calculate next numerator to numSum2 and save previous one to numSum0; numSum1 just copy of numSum2. */
		numSum[2] = aux * numSum[1] + numSum[0];
		numSum[0] = numSum[1];
		numSum[1] = numSum[2];
		/* calculate next denominator to denomSum2 and save previous one to denomSum0; denomSum1 just copy of denomSum2. */
		denomSum[2] = aux * denomSum[1] + denomSum[0];
		denomSum[0] = denomSum[1];
		denomSum[1] = denomSum[2];
	}

	/*-- Check and adapt for final variable size and return values; reduces internal accuracy; denominator is kept in ULONG-range with maxDenom -- */
	while (numSum[1] > returnLimit || denomSum[1] > returnLimit) {
		numSum[1] = numSum[1] / 2;
		denomSum[1] = denomSum[1] / 2;
	}

	/* return values */
	if (blnUseReciprocal) {
		*ullNum = denomSum[1];
		*ullDenom = numSum[1];
	} else {
		*ullNum = numSum[1];
		*ullDenom = denomSum[1];
	}

}  /*-- euclideanGCD() -------------- */


/**---- DoubleToRational() -----------------------------------------------
   /* -----  Rational2Double: Double To Rational Conversion ---------------------
   * There is a mathematical theorem to convert real numbers into a rational (integer fraction) number.
   * This is called "continuous fraction" which uses the Euclidean algorithm to find the greatest common divisor (GCD).
   *  (ref. e.g. https://de.wikipedia.org/wiki/Kettenbruch or https://en.wikipedia.org/wiki/Continued_fraction
   *             https://en.wikipedia.org/wiki/Euclidean_algorithm)
   */
void DoubleToRational2(const double value, uint32_t *num, uint32_t *denom)
{
	/*---- UN-SIGNED RATIONAL ---- */

	double dblDiff, dblDiff2;
	uint64_t ullNum, ullDenom, ullNum2, ullDenom2;

#ifdef DOUBLE2RAT_DEBUGOUTPUT
	/*-- For debugging purposes, check accuracy of this routine -- */
	double f_new, f_old, f_new2, f_old2;	/*debugging*/
#endif


	/*-- Check for negative values. If so it is an error. */
	if (value < 0) {
		*num = *denom = 0;
		TIFFErrorExt(0, "TIFFLib: DoubeToRational()", " Negative Value for Unsigned Rational given.");
		return;
	}

	/*-- Check for too big numbers (> UINT32_MAX) -- */
	if (value > UINT32_MAX) {
		*num = 0xFFFFFFFF;
		*denom = 0;
		return;
	}
	/*-- Check for easy integer numbers -- */
	if (value == (uint32_t)(value)) {
		*num = (uint32_t)value;
		*denom = 1;
		return;
	}
	/*-- Check for too small numbers for "uint32_t" type rationals -- */
	if (value < 1.0 / (double)0xFFFFFFFFU) {
		*num = 0;
		*denom = 0xFFFFFFFFU;
		return;
	}

	/*-- There are two approaches using the Euclidean algorithm, 
	 *   which can accidentially lead to different accuracies just depending on the value.
	 *   Try both and define which one was better.
	 */
	ToRationalEuclideanGCD(value, FALSE, FALSE, &ullNum, &ullDenom);
	ToRationalEuclideanGCD(value, FALSE, TRUE, &ullNum2, &ullDenom2);
	/*-- Double-Check, that returned values fit into ULONG :*/
	if (ullNum > UINT32_MAX || ullDenom > UINT32_MAX || ullNum2 > UINT32_MAX || ullDenom2 > UINT32_MAX)
		assert(0);

	/* Check, which one has higher accuracy and take that. */
	dblDiff = fabs(value - ((double)ullNum / (double)ullDenom));
	dblDiff2 = fabs(value - ((double)ullNum2 / (double)ullDenom2));
	if (dblDiff < dblDiff2) {
		*num = (uint32_t)ullNum;
		*denom = (uint32_t)ullDenom;
	} else {
		*num = (uint32_t)ullNum2;
		*denom = (uint32_t)ullDenom2;
	}


	///*-- Check and adapt for final variable size and return values -- */
	//while (ullNum > UINT32_MAX) {
	//	ullNum = ullNum / 2;
	//	ullDenom = ullDenom / 2;
	//}
	//*num = (uint32_t)ullNum;
	//*denom = (uint32_t)ullDenom;

#ifdef DOUBLE2RAT_DEBUGOUTPUT
	///*-- Generally, this routine has a higher accuracy than the original "direct" method.
	//However, in some cases the "direct" method provides better results. Therefore, check here. */
	uint32_t	num2, denom2;
	DoubleToRational_direct(value, &num2, &denom2);

	/*-- For debugging purposes, check accuracy of this routine -- */
	f_new = ((double)*num / (double)*denom);	/*debugging*/
	f_new2 = ((float)*num / (float)*denom);	/*debugging*/
	f_old = ((double)num2 / (double)denom2);	/*debugging*/
	f_old2 = ((float)num2 / (float)denom2);	/*debugging*/
	dblDiff = fabs(value - ((double)*num / (double)*denom));
	dblDiff2 = fabs(value - ((double)num2 / (double)denom2));
	if (dblDiff > dblDiff2) {
		TIFFErrorExt(0, "TIFFLib: DoubeToRational()", " Old Method is better for %.18f: new dif=%g old-dif=%g.\n", value, dblDiff, dblDiff2);
		*denom = denom2;
		*num = num2;
	}
	else TIFFErrorExt(0, "TIFFLib: DoubeToRational()", " New is better for %.18f: new dif=%g old-dif=%g.\n", value, dblDiff, dblDiff2);
#endif
}  /*-- DoubleToRational2() -------------- */

/**---- DoubleToRational() -----------------------------------------------
* Calculates the rational fractional of a double input value
* for SIGNED rationals,
* using the Euclidean algorithm to find the greatest common divisor (GCD)
------------------------------------------------------------------------*/
void DoubleToSrational2(const double value, int32_t *num, int32_t *denom)
{
	/*---- SIGNED RATIONAL ----*/
	int neg = 1;

	double dblDiff, dblDiff2;
	uint64_t ullNum, ullDenom, ullNum2, ullDenom2;

	/*-- Check for negative values and use then the positive one for internal calculations. */
	auto v = value;
	if (v < 0) {
		neg = -1;
		v = -v;
	}

	/*-- Check for too big numbers (> INT32_MAX) -- */
	if (v > INT32_MAX) {
		*num = 0x7FFFFFFF;
		*denom = 0;
		return;
	}
	/*-- Check for easy numbers -- */
	if (v == (int32_t)(v)) {
		*num = (int32_t)v;
		*denom = 1;
		return;
	}
	/*-- Check for too small numbers for "long" type rationals -- */
	if (v < 1.0 / (double)0x7FFFFFFF) {
		*num = 0;
		*denom = 0x7FFFFFFF;
		return;
	}


	/*-- There are two approaches using the Euclidean algorithm,
	*   which can accidentially lead to different accuracies just depending on the value.
	*   Try both and define which one was better.
	*   Furthermore, set behaviour of ToRationalEuclideanGCD() to the range of signed-long.
	*/
	ToRationalEuclideanGCD(v, TRUE, FALSE, &ullNum, &ullDenom);
	ToRationalEuclideanGCD(v, TRUE, TRUE, &ullNum2, &ullDenom2);
	/*-- Double-Check, that returned values fit into LONG :*/
	if (ullNum > INT32_MAX || ullDenom > INT32_MAX || ullNum2 > INT32_MAX || ullDenom2 > INT32_MAX)
		assert(0);

	/* Check, which one has higher accuracy and take that. */
	dblDiff = fabs(v - ((double)ullNum / (double)ullDenom));
	dblDiff2 = fabs(v - ((double)ullNum2 / (double)ullDenom2));
	if (dblDiff < dblDiff2) {
		*num = (long)(neg * (long)ullNum);
		*denom = (long)ullDenom;
	} else {
		*num = (long)(neg * (long)ullNum2);
		*denom = (long)ullDenom2;
	}




	///*-- Check and adapt for final variable size and return values, especially for reduced signed range,
	// *   because ToRationalEuclideanGCD() returns ULONG -- */
	//while (ullNum > INT32_MAX || ullDenom > INT32_MAX) {
	//	ullNum = ullNum / 2;
	//	ullDenom = ullDenom / 2;
	//}
	//*num = neg * (long)ullNum;
	//*denom = (long)ullDenom;

#ifdef DOUBLE2RAT_DEBUGOUTPUT
	///*-- Generally, this routine has a higher accuracy than the original "direct" method.
	//However, in some cases the "direct" method provides better results. Therefore, check here. */
	int32_t	num2, denom2;
	DoubleToSrational_direct(value, &num2, &denom2);

	/*-- For debugging purposes, check accuracy of this routine -- */
	double f_new, f_old, f_new2, f_old2;	/*debugging*/
	f_new = ((double)*num / (double)*denom);	/*debugging*/
	f_new2 = ((float)*num / (float)*denom);	/*debugging*/
	f_old = ((double)num2 / (double)denom2);	/*debugging*/
	f_old2 = ((float)num2 / (float)denom2);	/*debugging*/
	dblDiff = fabs(value - ((double)*num / (double)*denom));
	dblDiff2 = fabs(value - ((double)num2 / (double)denom2));
	if (dblDiff > dblDiff2) {
		TIFFErrorExt(0, "TIFFLib: DoubeToSRational()", " Old Method is better for %.18f: new dif=%g old-dif=%f .\n", value, dblDiff, dblDiff2);
		*denom = denom2;
		*num = num2;
	}
	else TIFFErrorExt(0, "TIFFLib: DoubeToSRational()", " New is better for %.18f: new dif=%g old-dif=%g .\n", value, dblDiff, dblDiff2);
#endif
}  /*-- DoubleToSrational2() --------------*/






//#define DOUBLE2RAT_DEBUGOUTPUT
/* -----  Double To Rational Conversion ---------------------
* From: http://rosettacode.org/wiki/Convert_decimal_number_to_rational
* Here's another version of best rational approximation of a floating point number.
* Especially for small numbers as needed for EXIF GPS tags latitude and longitude in WGS84.
*/
/* f : number to convert.
* num, denom: returned parts of the rational.
* md: max denominator value.  Note that machine floating point number
*     has a finite resolution (10e-16 ish for 64 bit double), so specifying
*     a "best match with minimal error" is often wrong, because one can
*     always just retrieve the significand and return that divided by
*     2**52, which is in a sense accurate, but generally not very useful:
*     1.0/7.0 would be "2573485501354569/18014398509481984", for example.
*     md=65536 seems to be sufficient for double values and long num/denom
*/
void DoubleToRationalOld(const double value, uint32_t *num, uint32_t *denom)
{
	//---- UN-SIGNED RATIONAL ----
	/*  a: continued fraction coefficients. */
	//-- Internally, the integer variables can be bigger than the external ones,
	//   as long as the result will fit into the external variable size.
	uint64_t a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
	uint64_t x, d, n = 1;
	int i, neg = 1;
	uint64_t nMax = ((9223372036854775807i64 - 1) / 2);		// for long long nMax = ((9223372036854775807-1)/2); for long nMax = ((2147483647-1)/2);
	double fMax = (double)((9223372036854775807i64 - 1) / 2); 					// fMax has to be smaller than max value of "d" /2	

	uint32_t md = UINT32_MAX;	// this guarantees that denominator stays within size of long variables.

	//-- if md would be a parameter to the subroutine, then the following check is necessary:
	//if (md <= 1) { *denom = 1; *num = (long) value; return; }

	//-- Check for negative values. If so it is an error.
    auto v = value;
	if (v < 0) {
		neg = -1;
		v = -v;
		*num = *denom = 0;
		TIFFErrorExt(0, "TIFFLib: DoubeToRational()", " Negative Value for Unsigned Rational given.");
		return;
	}

	//-- Check for too big numbers (> INT32_MAX) --
	if (v > UINT32_MAX) {
		*num = 0xFFFFFFFF;
		*denom = 0;
		return;
	}
	//-- Check for easy numbers --
	if (v == (int32_t)(v)) {
		*num = (int32_t)v;
		*denom = 1;
		return;
	}
	//-- Check for too small numbers for "long" type rationals --
	if (v < 1.0 / 0xFFFFFFFF) {
		*num = 0;
		*denom = 0xFFFFFFFF;
		return;
	}

	//-- Generate Integer value from double with fractional.
	//   d = big numinator of value without fraction (or cut residual fraction)
	//   n = big denominator of value
	//-- Break-criteria so that cast to "d" introduces no error and n has no overflow.
	while ((v != floor(v)) && (v < fMax) && (n < nMax)) { 
		n <<= 1; 
		v *= 2; 
	}
	d = (uint64_t)v;

	/* continued fraction and check denominator each step */
	for (i = 0; i < 64; i++) {
		a = n ? d / n : 0;						// if n is not zero, calculate integer part of original number.
		if (i && !a) break;						// if n is zero, exit loop

		x = d; d = n; n = x % n;				// set n to reminder of d/n and  d to previous denominator n.

		x = a;
		if (k[1] * a + k[0] >= md) {			// calculate next denominator and check for its given maximum
			x = (md - k[0]) / k[1];
			if (x * 2 >= a || k[1] >= md)
				i = 65;
			else
				break;
		}
		h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];  // calculate next numerator to h2 and save previous one to h0; h1 just copy of h2.
		k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];  // calculate next denominator to k2 and save previous one to k0; k1 just copy of k2.
	}
	//-- Check and adapt for final variable size and return values --
	while (h[1] > UINT32_MAX) {
		h[1] = h[1] / 2;
		k[1] = k[1] / 2;
	}
	*denom = (uint32_t)k[1];
	*num = (uint32_t)h[1];

#ifdef DOUBLE2RAT_DEBUGOUTPUT
	//-- For check of better accuracy of "direct" method.
    uint32_t num2, denom2;

	///*-- Generally, this routine has a higher accuracy than the original "direct" method.
	//However, in some cases the "direct" method provides better results. Therefore, check here. */
	DoubleToRational_direct(value, &num2, &denom2);

	//-- For debugging purposes, check accuracy of this routine --
	double dblDiff, dblDiff2;

	dblDiff = fabs(value - ((double)*num / (double)*denom));	//debugging
	dblDiff2 = fabs(value - ((double)num2 / (double)denom2)); //debugging
	if (fabs(value - ((double)*num / (double)*denom)) > fabs(value - ((double)num2 / (double)denom2))) {
		TIFFErrorExt(0, "TIFFLib: DoubeToRational()", " Old Method is better for %f: new dif=%f old-dif=%f .\n", value, dblDiff, dblDiff2);
		*denom = denom2;
		*num = num2;
	}
	else TIFFErrorExt(0, "TIFFLib: DoubeToSrational()", " New is better for %f: new dif=%f old-dif=%f .\n", value, dblDiff, dblDiff2);
#endif
}  //-- DoubleToRationalOld() --------------


void DoubleToSrationalOld(const double value, int32_t *num, int32_t *denom)
{
	//---- SIGNED RATIONAL ----
	/*  a: continued fraction coefficients. */
	//-- Internally, the integer variables can be bigger than the external ones,
	//   as long as the result will fit into the external variable size.
	uint64_t a, h[3] = { 0, 1, 0 }, k[3] = { 1, 0, 0 };
	uint64_t x, d, n = 1;
	int i, neg = 1;
	uint64_t nMax = ((9223372036854775807 - 1) / 2);		// for long long nMax = ((9223372036854775807-1)/2); for long nMax = ((2147483647-1)/2);
	double fMax = (double)((9223372036854775807i64 - 1) / 2); 					// fMax has to be smaller than max value of "d" /2	

																				//-- For check of better accuracy of "direct" method.
	int32_t	num2, denom2;

	//-- For debugging purposes, check accuracy of this routine --
#ifdef DOUBLE2RAT_DEBUGOUTPUT
	double dblDiff, dblDiff2;
#endif

	int32_t md = INT32_MAX;	// this guarantees that denominator stays within size of long variables.
						//-- if md would be a parameter to the subroutine, then the following check is necessary:
						//if (md <= 1) { *denom = 1; *num = (long) f; return; }

	//-- Check for negative values
	auto v = value;
	if (v < 0) {
		neg = -1;
		v = -v;
	}

	//-- Check for too big numbers (> INT32_MAX) --
	if (v > INT32_MAX) {
		*num = 0x7FFFFFFF;
		*denom = 0;
		return;
	}
	//-- Check for easy numbers --
	if (v == (int32_t)(v)) {
		*num = (int32_t)v;
		*denom = 1;
		return;
	}
	//-- Check for too small numbers for "long" type rationals --
	if (v < 1.0 / 0x7FFFFFFF) {
		*num = 0;
		*denom = 0x7FFFFFFF;
		return;
	}

	//-- Generate Integer value from double with fractional.
	//   d = big numinator of value without fraction (or cut residual fraction)
	//   n = big denominator of value
	//-- Break-criteria so that cast to "d" introduces no error and n has no overflow.
	while ((v != floor(v)) && (v < fMax) && (n < nMax)) {
		n <<= 1;
		v *= 2;
	}
	d = (uint64_t)v;

	/* continued fraction and check denominator each step */
	for (i = 0; i < 64; i++) {
		a = n ? d / n : 0;						// if n is not zero, calculate integer part of original number.
		if (i && !a) break;						// if n is zero, exit loop

		x = d; d = n; n = x % n;				// set n to reminder of d/n and  d to previous denominator n.

		x = a;
		if (k[1] * a + k[0] >= md) {			// calculate next denominator and check for its given maximum
			x = (md - k[0]) / k[1];
			if (x * 2 >= a || k[1] >= md)
				i = 65;
			else
				break;
		}
		h[2] = x * h[1] + h[0]; h[0] = h[1]; h[1] = h[2];  // calculate next numerator to h2 and save previous one to h0; h1 just copy of h2.
		k[2] = x * k[1] + k[0]; k[0] = k[1]; k[1] = k[2];  // calculate next denominator to k2 and save previous one to k0; k1 just copy of k2.
	}
	//-- Check and adapt for final variable size and return values --
	while (h[1] > INT32_MAX) {
		h[1] = h[1] / 2;
		k[1] = k[1] / 2;
	}
	*denom = (long)k[1];
	*num = neg * (long)h[1];

	//-- Generally, this routine has a higher accuracy than the original "direct" method.
	//   However, in some cases the "direct" method provides better results. Therefore, check here.
	DoubleToSrational_direct(value, &num2, &denom2);
	if (fabs(value - ((double)*num / (double)*denom)) > fabs(value - ((double)num2 / (double)denom2))) {
		*denom = denom2;
		*num = num2;
	}

#ifdef DOUBLE2RAT_DEBUGOUTPUT
	dblDiff = fabs(value - ((double)*num / (double)*denom));	//debugging
	dblDiff2 = fabs(value - ((double)num2 / (double)denom2)); //debugging
	if (fabs(value - ((double)*num / (double)*denom)) > fabs(value - ((double)num2 / (double)denom2))) {
		TIFFErrorExt(0, "TIFFLib: DoubeToRational()", " Old Method is better for %f: new dif=%f old-dif=%f .\n", value, dblDiff, dblDiff2);
		*denom = denom2;
		*num = num2;
	}
	else TIFFErrorExt(0, "TIFFLib: DoubeToSrational()", " New is better for %f: new dif=%f old-dif=%f .\n", value, dblDiff, dblDiff2);
#endif
}  //-- DoubleToSrationalOld() --------------


void DoubleToRational_direct(const double value, uint32_t *num, uint32_t *denom)
{
	/*--- OLD Code for debugging and comparison  ---- */
	/* code merged from TIFFWriteDirectoryTagCheckedRationalArray() and TIFFWriteDirectoryTagCheckedRational() */

	/* First check for zero and also check for negative numbers (which are illegal for RATIONAL)
	* and also check for "not-a-number". In each case just set this to zero to support also rational-arrays.
	*/
	if (value <= 0.0 || value != value)
	{
		*num = 0;
		*denom = 1;
	}
	else if (value <= 0xFFFFFFFFU && (value == (double)(uint32_t)(value)))	/* check for integer values */
	{
		*num = (uint32_t)(value);
		*denom = 1;
	}
	else if (value<1.0)
	{
		*num = (uint32_t)((value) * (double)0xFFFFFFFFU);
		*denom = 0xFFFFFFFFU;
	}
	else
	{
		*num = 0xFFFFFFFFU;
		*denom = (uint32_t)((double)0xFFFFFFFFU / (value));
	}
}  /*-- DoubleToRational_direct() -------------- */

void DoubleToSrational_direct(const double value, int32_t *num, int32_t *denom)
{
	/*--- OLD Code for debugging and comparison -- SIGNED-version ----*/
	/*  code was amended from original TIFFWriteDirectoryTagCheckedSrationalArray() */

	/* First check for zero and also check for negative numbers (which are illegal for RATIONAL)
	* and also check for "not-a-number". In each case just set this to zero to support also rational-arrays.
	*/
	if (value < 0.0)
	{
		if (value == (int32_t)(value))
		{
			*num = (int32_t)(value);
			*denom = 1;
		}
		else if (value>-1.0)
		{
			*num = -(int32_t)((-value) * (double)0x7FFFFFFF);
			*denom = 0x7FFFFFFF;
		}
		else
		{
			*num = -0x7FFFFFFF;
			*denom = (int32_t)((double)0x7FFFFFFF / (-value));
		}
	}
	else
	{
		if (value == (int32_t)(value))
		{
			*num = (int32_t)(value);
			*denom = 1;
		}
		else if (value<1.0)
		{
			*num = (int32_t)((value)  * (double)0x7FFFFFFF);
			*denom = 0x7FFFFFFF;
		}
		else
		{
			*num = 0x7FFFFFFF;
			*denom = (int32_t)((double)0x7FFFFFFF / (value));
		}
	}
}  /*-- DoubleToSrational_direct() --------------*/


