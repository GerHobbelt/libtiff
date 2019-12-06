
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
 * Extended and amended version for testing the TIFFSetField() / and TIFFGetField()- interface 
 * for custom fields of type RATIONAL when the TIFFLib internal precision is updated from FLOAT to DOUBLE!
 * The external interface of already defined tags schould be kept.
 * This is verified for some of those tags with this test.
 *
 */


#pragma warning( disable : 4101)

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



int write_test_tiff(TIFF *tif, const char *filenameRead);

#define	SPP	3		/* Samples per pixel */
const uint16	width = 1;
const uint16	length = 1;
const uint16	bps = 8;
const uint16	photometric = PHOTOMETRIC_RGB;
const uint16	rows_per_strip = 1;
const uint16	planarconfig = PLANARCONFIG_CONTIG;


int
main()
{
	static const char filenameClassicTiff[] = "rationalPrecision2Double.tif";
	static const char filenameBigTiff[] = "rationalPrecision2Double_Big.tif";

	TIFF			*tif;
	int				ret;
	int				errorNo;


	/* --- Test with Classic-TIFF ---*/
	/* delete file, if exists */
	ret = unlink(filenameClassicTiff);
	errorNo = errno;
	if (ret != 0 && errno != ENOENT) {
		fprintf(stderr, "Can't delete test TIFF file %s.\n", filenameClassicTiff);
	}

	/* We write the main directory as a simple image. */
	tif = TIFFOpen(filenameClassicTiff, "w+");
	if (!tif) {
		fprintf(stderr, "Can't create test TIFF file %s.\n", filenameClassicTiff);
		return 1;
	}
	fprintf(stderr, "-------- Test with ClassicTIFF started  ----------\n");
	ret = write_test_tiff(tif, filenameClassicTiff);

	/*--- Test with BIG-TIFF ---*/
	/* delete file, if exists */
	ret = unlink(filenameBigTiff);
	if (ret != 0 && errno != ENOENT) {
		fprintf(stderr, "Can't delete test TIFF file %s.\n", filenameBigTiff);
	}

	tif = TIFFOpen(filenameBigTiff, "w8");
	if (!tif) {
		fprintf(stderr, "Can't create test TIFF file %s.\n", filenameBigTiff);
		return 1;
	}
	fprintf(stderr, "\n\n-------- Test with BigTIFF started  ----------\n");
	ret = write_test_tiff(tif, filenameBigTiff);



} /* main() */







int
write_test_tiff(TIFF *tif, const char *filenameRead)
{
	unsigned char	buf[SPP] = { 0, 127, 255 };
	uint64          dir_offset = 0;
	uint64			dir_offset_GPS = 0, dir_offset_EXIF = 0;
	uint64          read_dir_offset = 0;
    /*-- Additional variables --*/
	int				retCode, retCode2;
	unsigned char	exifVersion[4] = {'0','2','3','1'};		/* EXIF 2.31 version is 4 characters of a string! */
	unsigned char	gpsVersion[4] = {2,2,0,1};		/* GPS Version is 4 numbers! */
	unsigned char   *pGpsVersion;
	char			auxStr[200];
	float			auxFloat = 0.0f;
	double			auxDouble = 0.0;
	char			auxChar = 0;
	uint16			auxUint16 = 0;
	uint32			auxUint32 = 0;
	short			auxShort=0;
	long			auxLong = 0;
	void			*pVoid;

	int		i, j;
	long	nTags;

	const TIFFFieldArray*	tFieldArray;
	unsigned long			tTag;
	TIFFDataType			tType;
	short					tWriteCount;
	TIFFSetGetFieldType		tSetFieldType;
	char					*tFieldName;

	char	blnFillGPSManually = 1;

#define STRSIZE 1000
#define N_SIZE  120
#define VARIABLE_ARRAY_SIZE 6

	/* -- Test data for writing -- */
	char			auxCharArrayW[N_SIZE];
	short			auxShortArrayW[N_SIZE];
	long			auxLongArrayW[N_SIZE];
	float			auxFloatArrayW[N_SIZE];
	double			auxDoubleArrayW[N_SIZE];
	char			auxTextArrayW[N_SIZE][STRSIZE];
	double			auxDoubleArrayGPS1[3] = {1.0/7.0, 61.23456789012345, 62.3};
	double			auxDoubleArrayGPS2[3] = {1.0/19.0, 88.34434, 15.0};
	double			auxDoubleArrayGPSTime[3] = {22.0, 17.0, 15.3456789};
	double			auxDoubleGPSAltitude     =  3456.7;
	double			auxDoubleGPSDirection    =  63.7;
	float			auxFloatArrayN1[3] = {1.0f/7.0f, 61.23456789012345f, 62.3f};

	/* -- Variables for reading -- */
	uint16      count16 = 0;
	union  {
		long		Long;
		short		Short1;
		short		Short2[2];
		char		Char[4];
	} auxLongUnion;
	union {
		double	dbl;
		float	flt1;
		float	flt2;
	} auxDblUnion;

	float		*pFloat;
	void		*pVoidArray;
	float		*pFloatArray;
	double		*pDoubleArray;
	char        *pAscii;
	char		auxCharArray[2*STRSIZE];
	short		auxShortArray[2*N_SIZE];
	long		auxLongArray[2*N_SIZE];
	float		auxFloatArray[2*N_SIZE];
	double		auxDoubleArray[2*N_SIZE];
	double		dblDiff, dblDiffLimit;
	float		fltDiff, fltDiffLimit;
#define RATIONAL_EPS (1.0/3000000.0)

	/*-- Fill test data arrays for writing ----------- */
	for (i=0; i<N_SIZE; i++) {
		sprintf(auxTextArrayW[i],"N%d-String-%d_tttttttttttttttttttttttttttttx", i, i);
	}
	for (i=0; i<N_SIZE; i++) {
		auxCharArrayW[i] = (char)(i+1);
	}
	for (i=0; i<N_SIZE; i++) {
		auxShortArrayW[i] = (short)(i+1)*7;
	}
	for (i=0; i<N_SIZE; i++) {
		auxLongArrayW[i] = (i+1)*133;
	}
	for (i=0; i<N_SIZE; i++) {
		auxFloatArrayW[i] = (float)((i+1)*133)/3.3f;
	}
	for (i=0; i<N_SIZE; i++) {
		auxDoubleArrayW[i] = (double)((i+1)*3689)/4.5697;
	}

	/*-- Setup standard tags of a simple tiff file --*/
	if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width)) {
		fprintf (stderr, "Can't set ImageWidth tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, length)) {
		fprintf (stderr, "Can't set ImageLength tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps)) {
		fprintf (stderr, "Can't set BitsPerSample tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, SPP)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip)) {
		fprintf (stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, planarconfig)) {
		fprintf (stderr, "Can't set PlanarConfiguration tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric)) {
		fprintf (stderr, "Can't set PhotometricInterpretation tag.\n");
		goto failure;
	}

	
	/*--- Some additional FIELD_CUSTOM tags to check standard interface ---*/

	/*- TIFFTAG_INKSET is a SHORT parameter (TIFF_SHORT, TIFF_SETGET_UINT16) with field_bit=FIELD_CUSTOM !! -*/
	if (!TIFFSetField(tif, TIFFTAG_INKSET, 34 )) {
		fprintf (stderr, "Can't set TIFFTAG_INKSET tag.\n");
		goto failure;
	}

	/*- TIFFTAG_PIXAR_FOVCOT is a FLOAT parameter ( TIFF_FLOAT, TIFF_SETGET_FLOAT) with field_bit=FIELD_CUSTOM !! -*/
	/*  - can be written with Double but has to be read with float parameter                                       */
#define PIXAR_FOVCOT_VAL	5.123456789123456789
	auxFloat = (float)PIXAR_FOVCOT_VAL;
	/* if (!TIFFSetField(tif, TIFFTAG_PIXAR_FOVCOT, auxFloat )) {
		fprintf (stderr, "Can't set TIFFTAG_PIXAR_FOVCOT tag.\n");
		goto failure;
	}
	*/
	auxDouble = (double)PIXAR_FOVCOT_VAL;
	if (!TIFFSetField(tif, TIFFTAG_PIXAR_FOVCOT, auxDouble)) {
		fprintf(stderr, "Can't set TIFFTAG_PIXAR_FOVCOT tag.\n");
		goto failure;
	}
	/*- TIFFTAG_STONITS is a DOUBLE parameter (TIFF_DOUBLE, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM!
	 *-- Unfortunately, TIFF_SETGET_DOUBLE is used for TIFF_RATIONAL but those have to be read with FLOAT !!!
	 *   Only TIFFTAG_STONITS is a TIFF_DOUBLE, which has to be read as DOUBLE!!
     */
#define STONITS_VAL 6.123456789123456789
	auxDouble = STONITS_VAL;
	auxFloat = (float)auxDouble;
	if (!TIFFSetField(tif, TIFFTAG_STONITS, auxDouble )) {
		fprintf (stderr, "Can't set TIFFTAG_STONITS tag.\n");
		goto failure;
	}


	/*-- Additional tags to check Rational standard tags, which are also defined with field_bit=FIELD_CUSTOM */
	/*
		The following standard tags have field_type = TIFF_RATIONAL  with field_bit=FIELD_CUSTOM:
			TIFFTAG_BASELINENOISE, 1, 1, TIFF_RATIONAL, 0, TIFF_SETGET_DOUBLE
			TIFFTAG_BASELINESHARPNESS, 1, 1, TIFF_RATIONAL, 0, TIFF_SETGET_DOUBLE
			TIFFTAG_LINEARRESPONSELIMIT, 1, 1, TIFF_RATIONAL, 0, TIFF_SETGET_DOUBLE
			TIFFTAG_CHROMABLURRADIUS, 1, 1, TIFF_RATIONAL, 0, TIFF_SETGET_DOUBLE
			TIFFTAG_ANTIALIASSTRENGTH, 1, 1, TIFF_RATIONAL, 0, TIFF_SETGET_DOUBLE
			TIFFTAG_SHADOWSCALE, 1, 1, TIFF_RATIONAL, 0, TIFF_SETGET_DOUBLE
			TIFFTAG_BESTQUALITYSCALE, 1, 1, TIFF_RATIONAL, 0, TIFF_SETGET_DOUBLE
		and with Signed Rational:
			TIFFTAG_BASELINEEXPOSURE, 1, 1, TIFF_SRATIONAL, 0, TIFF_SETGET_DOUBLE
		Due to the fact that TIFFSetField() and TIFFGetField() interface is using va_args, variable promotion is applied,
		which means:
		If the actual argument is of type float, it is promoted to type double when function is to be made.
		- Any signed or unsigned char, short, enumerated type, or bit field is converted to either a signed or an unsigned int
		  using integral promotion.
		- Any argument of class type is passed by value as a data structure; the copy is created by binary copying instead 
		  of by invoking the class’s copy constructor (if one exists).
		So, if your argument types are of float type, you should expect the argument retrieved to be of type double
		and it is char or short, you should expect it to be signed or unsigned int. Otherwise, the code will give you wrong results.
	*/

	/*--- TEST: First tag is written with FLOAT and second tag is written with DOUBLE parameter ---*/
	/*- TIFFTAG_SHADOWSCALE is a Rational parameter (TIFF_RATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! -*/
#define SHADOWSCALE_VAL   15.123456789123456789
	auxFloat = (float)SHADOWSCALE_VAL;
	if (!TIFFSetField(tif, TIFFTAG_SHADOWSCALE, auxFloat)) {
		fprintf(stderr, "Can't set TIFFTAG_SHADOWSCALE tag.\n");
		goto failure;
	}

	/*- TIFFTAG_BESTQUALITYSCALE is a Rational parameter (TIFF_RATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! -*/
#define BESTQUALITYSCALE_VAL   17.123456789123456789
	auxDouble = BESTQUALITYSCALE_VAL;
	if (!TIFFSetField(tif, TIFFTAG_BESTQUALITYSCALE, auxDouble )) {
		fprintf (stderr, "Can't set TIFFTAG_BESTQUALITYSCALE tag.\n");
		goto failure;
	}
	

		/*- TIFFTAG_BASELINEEXPOSURE is a Rational parameter (TIFF_SRATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! -*/
#define BASELINEEXPOSURE_VAL  (3.14159265358979323846) // (-227.123456789123456789)
		auxDouble = BASELINEEXPOSURE_VAL;
	if (!TIFFSetField(tif, TIFFTAG_BASELINEEXPOSURE, auxDouble)) {
		fprintf(stderr, "Can't set TIFFTAG_BASELINEEXPOSURE tag.\n");
		goto failure;
	}


	/*--- For static or variable ARRAYs the case is different ---*/

	/*- Variable Array: TIFFTAG_DECODE is a SRATIONAL parameter TIFF_SETGET_C16_FLOAT type FIELD_CUSTOM with passcount=1 and variable length of array. */
	if (!TIFFSetField(tif, TIFFTAG_DECODE, 3, auxFloatArrayN1)) {		/* for TIFF_SETGET_C16_DOUBLE */
		fprintf (stderr, "Can't set TIFFTAG_DECODE tag.\n");
		goto failure;
	}
	
	/*- Varable Array:  TIFF_RATIONAL, 0, TIFF_SETGET_C16_FLOAT */
	if (!TIFFSetField(tif, TIFFTAG_BLACKLEVEL, 3, auxFloatArrayN1)) {				/* für TIFF_SETGET_C16_FLOAT */
		fprintf (stderr, "Can't set TIFFTAG_BLACKLEVEL tag.\n");
		goto failure;
	}




	/*-- Write dummy pixel data. --*/
	if (!TIFFWriteScanline(tif, buf, 0, 0) < 0) {
		fprintf (stderr, "Can't write image data.\n");
		goto failure;
	}

	/*-- Write directory to file --*/
	/* Always WriteDirectory before using/creating another directory. */
	/* Not necessary before TIFFClose(), however, TIFFClose() uses TIFFReWriteDirectory(), which forces directory to be written at another location. */
	retCode = TIFFWriteDirectory(tif);

	/*-- Write File to disk and close file --*/
	/* TIFFClose() uses TIFFReWriteDirectory(), which forces directory to be written at another location. */
	/* Therefore, better use TIFFWriteDirectory() before. */
	TIFFClose(tif);

	fprintf (stderr, "-------- Continue Test  ---------- reading ...\n");

/*=========================  READING  =============  READING  ========================================*/
	/* Ok, now test whether we can read written values in the EXIF directory. */
	tif = TIFFOpen(filenameRead, "r");
	

	/*-- Read some parameters out of the main directory --*/

	/*-- IMAGEWIDTH and -LENGTH are defined as TIFF_SETGET_UINT32 */
	retCode = TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &auxUint32 );
	if (auxUint32 != width) {
		fprintf (stderr, "Read value of IMAGEWIDTH %d differs from set value %d\n", auxUint32, width);
	}
	retCode = TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &auxUint32 );
	if (auxUint32 != width) {
		fprintf (stderr, "Read value of TIFFTAG_IMAGELENGTH %d differs from set value %d\n", auxUint32, length);
	}


	/*- TIFFTAG_INKSET is a SHORT parameter (TIFF_SHORT, TIFF_SETGET_UINT16) with field_bit=FIELD_CUSTOM !! -*/
	retCode = TIFFGetField(tif, TIFFTAG_INKSET, &auxUint16);
	if (auxUint16 != 34) {
		fprintf(stderr, "Read value of TIFFTAG_PIXAR_FOVCOT %d differs from set value %d\n", auxUint16, TIFFTAG_INKSET);
	}

	/*- TIFFTAG_PIXAR_FOVCOT is a FLOAT parameter ( TIFF_FLOAT, TIFF_SETGET_FLOAT) with field_bit=FIELD_CUSTOM !! -*/
	/*  - was written with Double but has to be read with Float                                                    */
	retCode = TIFFGetField(tif, TIFFTAG_PIXAR_FOVCOT, &auxFloat );
	if (auxFloat != (float)PIXAR_FOVCOT_VAL) {
		fprintf (stderr, "Read value of TIFFTAG_PIXAR_FOVCOT %f differs from set value %f\n", auxFloat, PIXAR_FOVCOT_VAL);
	}

	/*- TIFFTAG_STONITS is a DOUBLE parameter (TIFF_DOUBLE, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM!! -*/
	retCode = TIFFGetField(tif, TIFFTAG_STONITS, &auxDouble);
	if (auxDouble != (double)STONITS_VAL) {
		fprintf(stderr, "Read value of TIFFTAG_STONITS %f differs from set value %f\n", auxDouble, STONITS_VAL);
	}








	/*- TIFFTAG_BESTQUALITYSCALE is a Rational parameter (TIFF_RATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! 
	    and written with double parameter -*/
	/*- First read with Float-parameter  -  which should be correct -*/
	auxFloat = 0;
	retCode = TIFFGetField(tif, TIFFTAG_BESTQUALITYSCALE, &auxFloat );
	dblDiffLimit = RATIONAL_EPS * (double)BESTQUALITYSCALE_VAL;
	dblDiff = auxFloat - (double)BESTQUALITYSCALE_VAL;
	if (fabs(dblDiff) > fabs(dblDiffLimit)) {
		fprintf(stderr, "Float-Read value of TIFFTAG_BESTQUALITYSCALE %.12f differs from set value %.12f\n", auxFloat, BESTQUALITYSCALE_VAL);
	}
	/*- And then read with Double-parameter  -  which should give a wrong value because only a float value is returned -*/
	auxDblUnion.dbl = 0;
	retCode = TIFFGetField(tif, TIFFTAG_BESTQUALITYSCALE, &auxDblUnion.dbl);
	dblDiffLimit = RATIONAL_EPS * (double)BESTQUALITYSCALE_VAL;
	dblDiff = auxDblUnion.dbl - (double)BESTQUALITYSCALE_VAL;
	fltDiff = auxDblUnion.flt1 - (float)BESTQUALITYSCALE_VAL;
	if ( !( (fabs(dblDiff) > fabs(dblDiffLimit)) && !(fabs(fltDiff) > fabs(dblDiffLimit))) ) {
		fprintf(stderr, "Double-Read value of TIFFTAG_BESTQUALITYSCALE %.12f differs from set value %.12f\n", auxDblUnion.dbl, BESTQUALITYSCALE_VAL);
		fprintf(stderr, "First Float part of Double-Read value of TIFFTAG_BESTQUALITYSCALE %.12f differs from set value %.12f\n", auxDblUnion.flt1, BESTQUALITYSCALE_VAL);
	}

	/*--- Now the same for a Signed Rational ---*/
	/*- TIFFTAG_BASELINEEXPOSURE is a Rational parameter (TIFF_SRATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! -
		and written with double parameter - */
	/*- First read with Float-parameter  -  which should be correct -*/
	auxFloat = 0;
	retCode = TIFFGetField(tif, TIFFTAG_BASELINEEXPOSURE, &auxFloat);
	dblDiffLimit = RATIONAL_EPS * (double)BASELINEEXPOSURE_VAL;
	dblDiff = auxFloat - (double)BASELINEEXPOSURE_VAL;
	if (fabs(dblDiff) > fabs(dblDiffLimit)) {
		fprintf(stderr, "Float-Read value of TIFFTAG_BASELINEEXPOSURE %.12f differs from set value %.12f\n", auxFloat, BASELINEEXPOSURE_VAL);
	}
	/*- And then read with Double-parameter  -  which should give a wrong value because only a float value is returned -*/
	auxDblUnion.dbl = 0;
	retCode = TIFFGetField(tif, TIFFTAG_BASELINEEXPOSURE, &auxDblUnion.dbl);
	dblDiffLimit = RATIONAL_EPS * (double)BASELINEEXPOSURE_VAL;
	dblDiff = auxDblUnion.dbl - (double)BASELINEEXPOSURE_VAL;
	float fff = (float)BASELINEEXPOSURE_VAL;
	fltDiff = auxDblUnion.flt1 - (float)BASELINEEXPOSURE_VAL;
	if (!((fabs(dblDiff) > fabs(dblDiffLimit)) && !(fabs(fltDiff) > fabs(dblDiffLimit)))) {
		fprintf(stderr, "Double-Read value of TIFFTAG_BASELINEEXPOSURE %.12f differs from set value %.12f\n", auxDblUnion.dbl, BASELINEEXPOSURE_VAL);
		fprintf(stderr, "First Float part of Double-Read value of TIFFTAG_BASELINEEXPOSURE %.12f differs from set value %.12f\n", auxDblUnion.flt1, BASELINEEXPOSURE_VAL);
	} else {
		fprintf(stderr, "Rational TIFFTAG_BASELINEEXPOSURE tag is written with double-precision value of %.18f\n  (flt=%.18f)\n  but read value has only limited precision of %.18f \n  with fltDiff= %e and dblDiff=%e\n", BASELINEEXPOSURE_VAL, (float)BASELINEEXPOSURE_VAL, auxDblUnion.flt1, fltDiff, auxDblUnion.flt1 - (double)BASELINEEXPOSURE_VAL);
	}

/*
Neue Rational2Double Implementierung:
Rational TIFFTAG_BASELINEEXPOSURE tag is written with double-precision value of -227.123456789123451927
  but read value has only limited precision of -227.123458862304687500 with diff= -2.073181e-06

Alte Implementierung:
Rational TIFFTAG_BASELINEEXPOSURE tag is written with double-precision value of -227.123456789123451927
  but read value has only limited precision of -227.123458862304687500 with diff= -2.073181e-06
*/



	/*- Variable Array: TIFFTAG_DECODE is a SRATIONAL parameter TIFF_SETGET_C16_FLOAT type FIELD_CUSTOM with passcount=1 and variable length of array. */
	retCode = TIFFGetField(tif, TIFFTAG_DECODE, &count16, &pVoidArray );
	retCode = TIFFGetField(tif, TIFFTAG_DECODE, &count16, &pFloatArray );
	/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
	memcpy(&auxFloatArray, pVoidArray,(count16 * sizeof(auxFloatArray[0])));
	for (i=0; i<count16; i++) {
		dblDiffLimit = RATIONAL_EPS*auxFloatArrayN1[i];
		dblDiff = auxFloatArray[i] - auxFloatArrayN1[i];
		if (fabs(dblDiff) > fabs(dblDiffLimit)) {
			fprintf (stderr, "Read value %d of TIFFTAG_DECODE Array %f differs from set value %f\n", i, auxFloatArray[i], auxFloatArrayN1[i]);
		}
	}

	retCode = TIFFGetField(tif, TIFFTAG_BLACKLEVEL, &count16, &pVoidArray);
	/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
	memcpy(&auxFloatArray, pVoidArray, (count16 * sizeof(auxFloatArray[0])));
	for (i = 0; i<count16; i++) {
		dblDiffLimit = RATIONAL_EPS*auxFloatArrayN1[i];
		dblDiff = auxFloatArray[i] - auxFloatArrayN1[i];
		if (fabs(dblDiff) > fabs(dblDiffLimit)) {
			fprintf(stderr, "Read value %d of TIFFTAG_BLACKLEVEL Array %f differs from set value %f\n", i, auxFloatArray[i], auxFloatArrayN1[i]);
		}
	}




	TIFFClose(tif);
	
	/* All tests passed; delete file and exit with success status. */
	//unlink(filenameRead);
	fprintf (stderr, "-------- Test finished  ----------\n");
	return 0;

failure:
	/* 
	 * Something goes wrong; close file and return unsuccessful status.
	 * Do not remove the file for further manual investigation.
	 */
	TIFFClose(tif);
	return 1;
}
