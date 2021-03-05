
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
 * ATTENTION: 
 *	Because some internal functions are used here, which are normally not included in the tiff-library,
 *	The following functions have to be temporarilly added to "libtiff.def":
 * 	_TIFFFillStriles 	_TIFFMergeFields 	TIFFFlushData1 	_TIFFSetGetFieldSize 	_TIFFClampDoubleToFloat 	_TIFFSeekOK
 *
 */


#define FOR_AUTO_TESTING
#ifdef FOR_AUTO_TESTING
/*  Only for automake and CMake infrastructure the test should: 
    a.) delete any written testfiles when test passed (otherwise autotest will fail)
	b.) goto failure, if any failure is detected, which is not necessary when test is initiated manually for debugging
	c.) if RETURN_FOR_PROGRAM_END is defined, the debugging output console is kept open till Return is pressed.
*/
//#define RETURN_FOR_PROGRAM_END
#define GOTOFAILURE  goto failure;
#else
#define GOTOFAILURE
#endif

#ifdef _MSC_VER
#pragma warning( disable : 4101)
#endif

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


//#include "tif_dirwrite.c"


#ifdef _MSC_VER
#define unlink _unlink
#endif

int write_test_tiff(TIFF * tif, const char* filenameRead, int blnAllCustomTags, int blnWriteRationalDirectly);

#define	SPP	3		/* Samples per pixel */
const uint16_t	width = 1;
const uint16_t	length = 1;
const uint16_t	bps = 8;
const uint16_t	photometric = PHOTOMETRIC_RGB;
const uint16_t	rows_per_strip = 1;
const uint16_t	planarconfig = PLANARCONFIG_CONTIG;

/*-- Additional custom TIFF tags for testing of Rational2Double precision --*/
#define TIFFTAG_RATIONAL_DOUBLE			60000
#define TIFFTAG_SRATIONAL_DOUBLE		60001
#define TIFFTAG_RATIONAL_C0_DOUBLE		60002
#define TIFFTAG_SRATIONAL_C16_DOUBLE	60003
#define TIFFTAG_SRATIONAL_C0_FLOAT		60004
#define TIFFTAG_SRATIONAL_C32_FLOAT		60005
#define TIFFTAG_SRATIONAL_C32_DOUBLE	60006
#define TIFFTAG_BYTE	60011
#define TIFFTAG_SHORT	60012
#define TIFFTAG_LONG	60013
#define TIFFTAG_LONG8	60014
#define TIFFTAG_SBYTE	60015
#define TIFFTAG_SSHORT	60016
#define TIFFTAG_SLONG	60017
#define TIFFTAG_SLONG8	60018
#define TIFFTAG_FLOAT	60019
#define TIFFTAG_DOUBLE	60020



/*--- TIFFField Definition ---
	field_tag: the tag number. For instance 277 for the SamplesPerPixel tag. Builtin tags will generally have a #define in tiff.h for each known tag.
	field_readcount: The number of values which should be read. The special value TIFF_VARIABLE (-1) indicates that a variable number of values may be read. The special value TIFFTAG_SPP (-2) indicates that there should be one value for each sample as defined by TIFFTAG_SAMPLESPERPIXEL. The special value TIFF_VARIABLE2 (-3) is presumably similar to TIFF_VARIABLE though I am not sure what the distinction in behaviour is. This field is TIFF_VARIABLE for variable length ascii fields.
	field_writecount: The number of values which should be written. Generally the same as field_readcount. A few built-in exceptions exist, but I haven't analysed why they differ.
	field_type: Type of the field. One of TIFF_BYTE, TIFF_ASCII, TIFF_SHORT, TIFF_LONG, TIFF_RATIONAL, TIFF_SBYTE, TIFF_UNDEFINED, TIFF_SSHORT, TIFF_SLONG, TIFF_SRATIONAL, TIFF_FLOAT, TIFF_DOUBLE or TIFF_IFD. Note that some fields can support more than one type (for instance short and long). These fields should have multiple TIFFFieldInfos.
	reserved:
	set_field_type: TIFF_SETGET_DOUBLE
	get_field_type: - not used -
	field_bit: Built-in tags stored in special fields in the TIFF structure have assigned field numbers to distinguish them (ie. FIELD_SAMPLESPERPIXEL). New tags should generally just use FIELD_CUSTOM indicating they are stored in the generic tag list.
	field_oktochange: TRUE if it is OK to change this tag value while an image is being written. FALSE for stuff that must be set once and then left unchanged (like ImageWidth, or PhotometricInterpretation for instance).
	field_passcount: If TRUE, then the count value must be passed in TIFFSetField(), and TIFFGetField(), otherwise the count is not required. This should generally be TRUE for non-ascii variable count tags unless the count is implicit (such as with the colormap).
	field_name: A name for the tag. Normally mixed case (studly caps) like "StripByteCounts" and relatively short.
*/
/*--: Hint for defining arrays:
 * Arrays with set_field_type = .._Cxx_.. need a special setting for field_readcount(/write-count) and field_passcount:
 *				field_readcount		field_passcount
 * .._C0_..		positive number			0				(fixed array)
 * .._C16_..	-1 = TIFF_VARIABLE		1				(variable array using uint16_t as count value)
 * .._C32_..	-3 = TIFF_VARIABLE2		1				(variable array using uint32_t as count value)
 */

static const TIFFField
tifFieldInfoInitial[] = {
	{ TIFFTAG_RATIONAL_DOUBLE, 1, 1, TIFF_RATIONAL, 0, TIFF_SETGET_DOUBLE, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Rational2Double_U_Double", NULL },
	{ TIFFTAG_SRATIONAL_DOUBLE, 1, 1, TIFF_SRATIONAL, 0, TIFF_SETGET_DOUBLE, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Rational2Double_S_Double", NULL },
	{ TIFFTAG_RATIONAL_C0_DOUBLE, 3, 3, TIFF_RATIONAL, 0, TIFF_SETGET_C0_DOUBLE, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Rational2Double_C0_Dbl", NULL },
	{ TIFFTAG_SRATIONAL_C16_DOUBLE, TIFF_VARIABLE, TIFF_VARIABLE, TIFF_SRATIONAL, 0, TIFF_SETGET_C16_DOUBLE, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 1, "Rational2Double_S_C16_Dbl", NULL },
	{ TIFFTAG_SRATIONAL_C0_FLOAT, 4, 4, TIFF_SRATIONAL, 0, TIFF_SETGET_C0_FLOAT, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Rational2Double_S_C0_Float", NULL },
	{ TIFFTAG_SRATIONAL_C32_FLOAT, TIFF_VARIABLE2, TIFF_VARIABLE2, TIFF_SRATIONAL, 0, TIFF_SETGET_C32_FLOAT, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 1, "Rational2Double_S_C32_Float", NULL },
	{ TIFFTAG_SRATIONAL_C32_DOUBLE, TIFF_VARIABLE2, TIFF_VARIABLE2, TIFF_SRATIONAL, 0, TIFF_SETGET_C32_DOUBLE, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 1, "Rational2Double_S_C32_Dbl", NULL },
	/*--  WriteLongReadRational: Write different field_types and read them lateron as rational. --*/
	{ TIFFTAG_BYTE, 1, 1, TIFF_BYTE, 0, TIFF_SETGET_UINT8, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_Byte", NULL },
	{ TIFFTAG_SHORT, 1, 1, TIFF_SHORT, 0, TIFF_SETGET_UINT16, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_Short", NULL },
	{ TIFFTAG_LONG, 1, 1, TIFF_LONG, 0, TIFF_SETGET_UINT32, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_Long", NULL },
	{ TIFFTAG_LONG8, 1, 1, TIFF_LONG, 0, TIFF_SETGET_UINT64, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_Long8", NULL },
	{ TIFFTAG_SBYTE, 1, 1, TIFF_SBYTE, 0, TIFF_SETGET_SINT8, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_SByte", NULL },
	{ TIFFTAG_SSHORT, 1, 1, TIFF_SSHORT, 0, TIFF_SETGET_SINT16, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_SShort", NULL },
	{ TIFFTAG_SLONG, 1, 1, TIFF_SLONG, 0, TIFF_SETGET_SINT32, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_SLong", NULL },
	{ TIFFTAG_SLONG8, 1, 1, TIFF_SLONG, 0, TIFF_SETGET_SINT64, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_SLong8", NULL },
	{ TIFFTAG_FLOAT, 1, 1, TIFF_FLOAT, 0, TIFF_SETGET_FLOAT, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_Float", NULL },
	{ TIFFTAG_DOUBLE, 1, 1, TIFF_DOUBLE, 0, TIFF_SETGET_DOUBLE, TIFF_SETGET_UNDEFINED, FIELD_CUSTOM, 0, 0, "Write_Double", NULL },
};

#define IDX_START	7
#define IDX_START_SIGNED	11
#define IDX_START_FLOAT		15
#define IDX_LONG8			10
#define IDX_SLONG8			14

#define	N(a)	(sizeof (a) / sizeof (a[0]))
static TIFFField tifFieldInfo[N(tifFieldInfoInitial)];

/*--- Add aditional Rational-Double Tags to TIFF 
	  ref: html\addingtags.html but with new function _TIFFMergeFields(). 
---*/

/* In libtiff 3.6.0 a new mechanism was introduced allowing libtiff to read unrecognised tags automatically. 
    When an unknown tags is encountered, it is automatically internally defined with a default name and a type derived from the tag value in the file. 
	Applications only need to predefine application specific tags if they need to be able to set them in a file, or if particular calling conventions 
	are desired for TIFFSetField() and TIFFGetField().
    When tags are autodefined like this the field_readcount and field_writecount values are always TIFF_VARIABLE. 
	The field_passcount is always TRUE, and the field_bit is FIELD_CUSTOM. The field name will be "Tag %d" where the %d is the tag number.
*/

/*The tags need to be defined for each TIFF file opened - and when reading they should be defined before the tags of the file are read, 
  yet a valid TIFF * is needed to merge the tags against. In order to get them registered at the appropriate part of the setup process, 
  it is necessary to register our merge function as an extender callback with libtiff. This is done with TIFFSetTagExtender(). 
  We also keep track of the previous tag extender (if any) so that we can call it from our extender allowing a chain of customizations to take effect. 
*/
static TIFFExtendProc _ParentExtender = NULL;
static void _XTIFFDefaultDirectory(TIFF *tif);

static
void _XTIFFInitialize(void)
{
    static int first_time=1;
	
    if (! first_time) return; /* Been there. Done that. */
first_time = 0;

/* Grab the inherited method and install */
_ParentExtender = TIFFSetTagExtender(_XTIFFDefaultDirectory);
}

/* The extender callback is looks like this.
It merges in our new fields and then calls the next extender if there is one in effect.
*/
static void
_XTIFFDefaultDirectory(TIFF *tif)
{
	uint32_t n, nadded;

	/* Install the extended Tag field info */
	n = N(tifFieldInfo);
	//_TIFFMergeFields(tif, const TIFFField info[], uint32_t n);
	nadded = _TIFFMergeFields(tif, tifFieldInfo, n);
        (void)nadded;

	/* Since an XTIFF client module may have overridden
	* the default directory method, we call it now to
	* allow it to set up the rest of its own methods.
	*/

	if (_ParentExtender)
		(*_ParentExtender)(tif);
}


int
main()
{
	static const char filenameClassicTiff1[] = "Rational_SetGetDirect_Test1.tif";
	static const char filenameClassicTiff2[] = "Rational_SetGetDirect_Test2.tif";
	static const char filenameClassicTiff3[] = "Rational_SetGetDirect_Test3.tif";
	static const char filenameBigTiff1[] = "Rational_SetGetDirect_Test_Big1.tif";
	static const char filenameBigTiff2[] = "Rational_SetGetDirect_Test_Big2.tif";

	TIFF			*tif;
	int				ret;
	int				errorNo;

	/*-- Initialize TIFF-Extender to add additonal TIFF-Tags --*/
	_XTIFFInitialize();

	/*=== Some special tags are tested first ===*/
	fprintf(stderr, "==== Test if Set()/Get() interface for some custom rational tags behave as before change. ====\n");
	/* --- Test with Classic-TIFF ---*/
	/* delete file, if exists */
	ret = unlink(filenameClassicTiff1);
	errorNo = errno;
	if (ret != 0 && errorNo != ENOENT) {
		fprintf(stderr, "Can't delete test TIFF file %s.\n", filenameClassicTiff1);
	}

	/* Re-Initialize extra tags field and open file. */
	memcpy(tifFieldInfo, tifFieldInfoInitial, sizeof(tifFieldInfo));
	tif = TIFFOpen(filenameClassicTiff1, "w+");
	if (!tif) {
		fprintf(stderr, "Can't create test TIFF file %s.\n", filenameClassicTiff1);
		return 1;
	}
	fprintf(stderr, "-------- Test with ClassicTIFF started  ----------\n");
	ret = write_test_tiff(tif, filenameClassicTiff1, FALSE, FALSE);
	if (ret > 0) return(ret);

	/* We write the main directory as a simple image with --- Rational directly.--- */
	/* delete file, if exists */
	ret = unlink(filenameClassicTiff3);
	errorNo = errno;
	if (ret != 0 && errorNo != ENOENT) {
		fprintf(stderr, "Can't delete test TIFF file %s.\n", filenameClassicTiff3);
	}
	/* Re-Initialize extra tags field and open file. */
	memcpy(tifFieldInfo, tifFieldInfoInitial, sizeof(tifFieldInfo));
	tif = TIFFOpen(filenameClassicTiff3, "w+");
	if (!tif) {
		fprintf(stderr, "Can't create test TIFF file %s.\n", filenameClassicTiff3);
		return 1;
	}
	fprintf(stderr, "\n-------- Test with ClassicTIFF started  ---- Rational Dierctly ------\n");
	ret = write_test_tiff(tif, filenameClassicTiff3, FALSE, TRUE);
	if (ret > 0) return(ret);


	/*====== Test with BIG-TIFF ======*/
	/* delete file, if exists */
	ret = unlink(filenameBigTiff1);
	if (ret != 0 && errorNo != ENOENT) {
		fprintf(stderr, "Can't delete test TIFF file %s.\n", filenameBigTiff1);
	}

	/* Re-Initialize extra tags field and open file. */
	memcpy(tifFieldInfo, tifFieldInfoInitial, sizeof(tifFieldInfo));
	tifFieldInfo[IDX_LONG8].field_type = TIFF_LONG8;
	tifFieldInfo[IDX_SLONG8].field_type = TIFF_SLONG8;
	tif = TIFFOpen(filenameBigTiff1, "w8");
	if (!tif) {
		fprintf(stderr, "Can't create test TIFF file %s.\n", filenameBigTiff1);
		return 1;
	}
	fprintf(stderr, "\n-------- Test with BigTIFF started  ----------\n");
	ret = write_test_tiff(tif, filenameBigTiff1, FALSE, FALSE);
	if (ret > 0) return(ret);


	/*=== Test with ALL Custom tags ===*/
	fprintf(stderr, "\n\n==== Test automatically, if all custom rational tags are written/read correctly. ====\n");
	/* --- Test with Classic-TIFF ---*/
	/* delete file, if exists */
	ret = unlink(filenameClassicTiff2);
	errorNo = errno;
	if (ret != 0 && errorNo != ENOENT) {
		fprintf(stderr, "Can't delete test TIFF file %s.\n", filenameClassicTiff2);
	}

	/* Re-Initialize extra tags field and open file. */
	memcpy(tifFieldInfo, tifFieldInfoInitial, sizeof(tifFieldInfo));
	tif = TIFFOpen(filenameClassicTiff2, "w+");
	if (!tif) {
		fprintf(stderr, "Can't create test TIFF file %s.\n", filenameClassicTiff2);
		return 1;
	}
	fprintf(stderr, "-------- Test with ClassicTIFF started  with all Custom Tags ----------\n");
	ret = write_test_tiff(tif, filenameClassicTiff2, TRUE, FALSE);
	if (ret > 0) return(ret);

	/*--- Test with BIG-TIFF ---*/
	/* delete file, if exists */
	ret = unlink(filenameBigTiff2);
	if (ret != 0 && errno != ENOENT) {
		fprintf(stderr, "Can't delete test TIFF file %s.\n", filenameBigTiff2);
	}

	/* Re-Initialize extra tags field and open file. */
	memcpy(tifFieldInfo, tifFieldInfoInitial, sizeof(tifFieldInfo));
	tif = TIFFOpen(filenameBigTiff2, "w8");
	if (!tif) {
		fprintf(stderr, "Can't create test TIFF file %s.\n", filenameBigTiff2);
		return 1;
	}
	fprintf(stderr, "\n-------- Test with BigTIFF started  with all Custom Tags ----------\n");
	ret = write_test_tiff(tif, filenameBigTiff2, TRUE, FALSE);

#ifndef FOR_AUTO_TESTING
#ifdef RETURN_FOR_PROGRAM_END
	fprintf(stderr, "\n -> press Return to finish program -> ");
	getchar();
#endif
#endif
	return(ret);
} /* main() */




int
write_test_tiff(TIFF* tif, const char* filenameRead, int blnAllCustomTags, int blnWriteRationalDirectly) {
	unsigned char	buf[SPP] = {0, 127, 255};
	/*-- Additional variables --*/
	int				retCode;
	float			auxFloat = 0.0f;
	double			auxDouble = 0.0;
	uint16_t			auxUint16 = 0;
	uint32_t			auxUint32 = 0;
	long			auxLong = 0;
	void* pVoid;
	int				blnIsRational2Double;

	int		i, j;
	long	nTags;

	const TIFFFieldArray* tFieldArray;
	unsigned long			tTag;
	TIFFDataType			tType;
	short					tWriteCount;
	TIFFSetGetFieldType		tSetFieldType;
	unsigned short			tFieldBit;
	const TIFFField			*fip;
	char* tFieldName;


#define STRSIZE 1000
#define N_SIZE  200
#define VARIABLE_ARRAY_SIZE 6

	/* -- Test data for writing -- */
	float			auxFloatArrayW[N_SIZE];
	double			auxDoubleArrayW[N_SIZE];
	char			auxTextArrayW[N_SIZE][STRSIZE];
	float			auxFloatArrayN1[3] = {1.0f / 7.0f, 61.23456789012345f, 62.3f};
	float			auxFloatArrayS1[3] = {-1.0f / 9.0f, 61.23456789012345f, -62.3f};
	float			auxFloatArrayResolutions[5] = {0.313f, 5.456789f, 6.666666f, 0.0033f, 5.0f / 213.0f};
	//TIFFRational_t	auxRationalArrayN3[3] = {{1,2},{3, 4},{5, 6}};
	//float			auxFloatArrayReferenceBlackWhite[6] = {0.314f, 23.5f, 5.0f, 9.8f, 9.9f, 10.0f};
	float			auxFloatArrayReferenceBlackWhite[6] = {1.0f/2.0f, 1.0f/3.0f, 2.0f/3.0f, 1.0f, 8.0f/7.0f, 10.0f};

	/* -- Variables for reading -- */
	uint16_t      count16 = 0;
	uint32_t	  count32 = 0;
	union {
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

	void* pVoidArray;
	//float* pFloatArray;
	float		auxFloatArray[2 * N_SIZE];
	double		auxDoubleArray[2 * N_SIZE];
	double		dblDiff, dblDiffLimit;
	float		fltDiff;
	uint32_t uN, uD, uN2, uD2;
	int32_t  sN, sD;
	TIFFRational_t	auxRationalArray[2 * N_SIZE];
	TIFFSRational_t	auxSRationalArray[2 * N_SIZE];
#define RATIONAL_EPS (1.0/30000.0) /* reduced difference of rational values, approx 3.3e-5 */


	/*-- Fill test data arrays for writing ----------- */
	for (i = 0; i < N_SIZE; i++) {
		sprintf(auxTextArrayW[i], "N%d-String-%d_tttttttttttttttttttttttttttttx", i, i);
	}
	for (i = 0; i < N_SIZE; i++) {
		auxFloatArrayW[i] = (float)((i + 1) * 133) / 3.3f;
	}
	for (i = 0; i < N_SIZE; i++) {
		auxDoubleArrayW[i] = (double)((i + 1) * 3689) / 4.5697;
	}

	/*-- Setup standard tags of a simple tiff file --*/
	if (!TIFFSetField(tif, TIFFTAG_IMAGEWIDTH, width)) {
		fprintf(stderr, "Can't set ImageWidth tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_IMAGELENGTH, length)) {
		fprintf(stderr, "Can't set ImageLength tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_BITSPERSAMPLE, bps)) {
		fprintf(stderr, "Can't set BitsPerSample tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_SAMPLESPERPIXEL, SPP)) {
		fprintf(stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_ROWSPERSTRIP, rows_per_strip)) {
		fprintf(stderr, "Can't set SamplesPerPixel tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PLANARCONFIG, planarconfig)) {
		fprintf(stderr, "Can't set PlanarConfiguration tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_PHOTOMETRIC, photometric)) {
		fprintf(stderr, "Can't set PhotometricInterpretation tag.\n");
		goto failure;
	}

	/*--- Standard tags with TIFF_RATIONAL and TIFF_SETGET_DOUBLE to TIFF_SETGET_FLOAT change. ---
	 *     They can be written either using float or double but have to be read using float.
	-------------------------------------------------------------------------------------------- */
	if (!TIFFSetField(tif, TIFFTAG_XRESOLUTION, auxFloatArrayResolutions[0])) {
			fprintf(stderr, "Can't set TIFFTAG_XRESOLUTION tag.\n");
			goto failure;
	}
	/* Test here the direct rational input possibility */
	DoubleToRational(auxFloatArrayResolutions[0], &uN, &uD);
	if (!TIFFSetFieldRational(tif, TIFFTAG_XRESOLUTION, uN, uD)) {
		fprintf(stderr, "Can't set TIFFTAG_XRESOLUTION tag as RATIONAL.\n");
		goto failure;
	}



	/* Test here the double input possibility */
	if (!TIFFSetField(tif, TIFFTAG_YRESOLUTION, (double)auxFloatArrayResolutions[1])) {
		fprintf(stderr, "Can't set TIFFTAG_YRESOLUTION tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_XPOSITION, auxFloatArrayResolutions[2])) {
		fprintf(stderr, "Can't set TIFFTAG_XPOSITION tag.\n");
		goto failure;
	}
	if (!TIFFSetField(tif, TIFFTAG_YPOSITION, auxFloatArrayResolutions[3])) {
		fprintf(stderr, "Can't set TIFFTAG_YPOSITION tag.\n");
		goto failure;
	}


	if (!blnWriteRationalDirectly) {
		/* Fixed Array of 6 rationals is TIFFTAG_REFERENCEBLACKWHITE */
		if (!TIFFSetField(tif, TIFFTAG_REFERENCEBLACKWHITE, auxFloatArrayReferenceBlackWhite)) {
			fprintf(stderr, "Can't set TIFFTAG_REFERENCEBLACKWHITE tag.\n");
			goto failure;
		}
	}
	else {
		/* Test here the direct rational input possibility for TIFFTAG_REFERENCEBLACKWHITE with a rational-array as input. */
		for (i = 0; i < 6; i++) {
			DoubleToRational(auxFloatArrayReferenceBlackWhite[i], &auxRationalArray[i].uNum, &auxRationalArray[i].uDenom);
		}
		if (!TIFFSetFieldRational(tif, TIFFTAG_REFERENCEBLACKWHITE, auxRationalArray)) {
			fprintf(stderr, "Can't set TIFFTAG_REFERENCEBLACKWHITE tag as RATIONAL.\n");
			goto failure;
		}
	}



	/*--- Some additional FIELD_CUSTOM tags to check standard interface ---*/

	/*- TIFFTAG_INKSET is a SHORT parameter (TIFF_SHORT, TIFF_SETGET_UINT16) with field_bit=FIELD_CUSTOM !! -*/
	if (!TIFFSetField(tif, TIFFTAG_INKSET, 34)) {
		fprintf(stderr, "Can't set TIFFTAG_INKSET tag.\n");
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
	if (!TIFFSetField(tif, TIFFTAG_STONITS, auxDouble)) {
		fprintf(stderr, "Can't set TIFFTAG_STONITS tag.\n");
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

	if (!blnAllCustomTags) {
		/*--- TEST: First tag is written with FLOAT and second tag is written with DOUBLE parameter ---*/
		/*- TIFFTAG_SHADOWSCALE is a Rational parameter (TIFF_RATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! -*/
		#define SHADOWSCALE_VAL   15.123456789123456789
		auxFloat = (float)SHADOWSCALE_VAL;
		if (!TIFFSetField(tif, TIFFTAG_SHADOWSCALE, auxFloat)) {
			fprintf(stderr, "Can't set TIFFTAG_SHADOWSCALE tag.\n");
			goto failure;
		}

		/*- TIFFTAG_BESTQUALITYSCALE is a Rational parameter (TIFF_RATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! -*/
		//#define BESTQUALITYSCALE_VAL   17.123456789123456789
		#define BESTQUALITYSCALE_VAL   (17.0/2.0)
		auxDouble = BESTQUALITYSCALE_VAL;
		if (!TIFFSetField(tif, TIFFTAG_BESTQUALITYSCALE, auxDouble)) {
			fprintf(stderr, "Can't set TIFFTAG_BESTQUALITYSCALE tag.\n");
			goto failure;
		}


		/*- TIFFTAG_BASELINEEXPOSURE is a Rational parameter (TIFF_SRATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! -*/
		//#define BASELINEEXPOSURE_VAL  (-3.14159265358979323846) 
		#define BASELINEEXPOSURE_VAL  (-0.25) 
		/*
		fprintf(stderr, "(-3.14159265358979323846) as float= %.18f, double=%.18f\n", (float)BASELINEEXPOSURE_VAL, (double)BASELINEEXPOSURE_VAL, BASELINEEXPOSURE_VAL);
		fprintf(stderr, "(-3.141592742098056)      as float= %.18f, double=%.18f\n", (float)(-3.141592742098056), (double)(-3.141592742098056));
		*/
		auxDouble = BASELINEEXPOSURE_VAL;
		if (!TIFFSetField(tif, TIFFTAG_BASELINEEXPOSURE, auxDouble)) {
			fprintf(stderr, "Can't set TIFFTAG_BASELINEEXPOSURE tag.\n");
			goto failure;
		}


		/*--- For static or variable ARRAYs the case is different ---*/

		if (!blnWriteRationalDirectly) {
			/*- Variable Array: TIFFTAG_DECODE is a SRATIONAL parameter TIFF_SETGET_C16_FLOAT type FIELD_CUSTOM with passcount=1 and variable length of array. */
			if (!TIFFSetField(tif, TIFFTAG_DECODE, 3, auxFloatArrayS1)) {		/* for TIFF_SETGET_C16_DOUBLE */
				fprintf(stderr, "Can't set TIFFTAG_DECODE tag.\n");
				goto failure;
			}
		}
		else {
			/* Test here the direct rational input possibility for TIFFTAG_DECODE with a rational-array as input. */
			for (i = 0; i < 3; i++) {
				DoubleToSrational(auxFloatArrayS1[i], &auxSRationalArray[i].sNum, &auxSRationalArray[i].sDenom);
			}
			if (!TIFFSetFieldRational(tif, TIFFTAG_DECODE, 3, auxSRationalArray)) {
				fprintf(stderr, "Can't set TIFFTAG_DECODE tag as RATIONAL.\n");
				goto failure;
			}
		}

		if (!blnWriteRationalDirectly) {
			/*- Varable Array:  TIFF_RATIONAL, 0, TIFF_SETGET_C16_FLOAT */
			if (!TIFFSetField(tif, TIFFTAG_BLACKLEVEL, 3, auxFloatArrayN1)) {				/* for TIFF_SETGET_C16_FLOAT */
				fprintf(stderr, "Can't set TIFFTAG_BLACKLEVEL tag.\n");
				goto failure;
			}
		}
		else {
			/* Test here the direct rational input possibility for TIFFTAG_BLACKLEVEL with a rational-array as input. */
			for (i = 0; i < 3; i++) {
				DoubleToRational(auxFloatArrayN1[i], &auxRationalArray[i].uNum, &auxRationalArray[i].uDenom);
			}
			if (!TIFFSetFieldRational(tif, TIFFTAG_BLACKLEVEL, 3, auxRationalArray)) {
				fprintf(stderr, "Can't set TIFFTAG_BLACKLEVEL tag as RATIONAL.\n");
				goto failure;
			}
		}


		/*--  WriteLongReadRational: Write tag as BYTE and read lateron as rational. --*/
		nTags = N(tifFieldInfo);
		for (i = IDX_START; i < nTags; i++) {
			if (i < IDX_START_SIGNED) {
				if (!TIFFSetField(tif, tifFieldInfo[i].field_tag, i)) {
					fprintf(stderr, "Can't set TIFFTAG_xxx tag %d.\n", tifFieldInfo[i].field_tag);
					goto failure;
				}
			} else if (i < IDX_START_FLOAT) {
				if (!TIFFSetField(tif, tifFieldInfo[i].field_tag, (-1*i))) {
					fprintf(stderr, "Can't set TIFFTAG_xxx tag %d.\n", tifFieldInfo[i].field_tag);
					goto failure;
				}
			} else {
				if (!TIFFSetField(tif, tifFieldInfo[i].field_tag, (-2.3 * i))) {
					fprintf(stderr, "Can't set TIFFTAG_xxx tag %d.\n", tifFieldInfo[i].field_tag);
					goto failure;
				}
			}
		}




		/*-- Check, if the TiffLibrary is compiled with the new interface with Rational2Double or still uses the old definitions. */
		/*   tags to check: TIFFTAG_BESTQUALITYSCALE, TIFFTAG_BASELINENOISE, TIFFTAG_BASELINESHARPNESS, */
		fip = TIFFFindField(tif, TIFFTAG_BESTQUALITYSCALE, TIFF_ANY);
		tSetFieldType = fip->set_field_type;
		if (tSetFieldType == TIFF_SETGET_DOUBLE)
			blnIsRational2Double = FALSE;
		else
			blnIsRational2Double = TRUE;

		/*--- Write now additional Rational2Double test tags ---*/
		/*--- However, this additional tags are only written as Double correctly,
			  if blnIsRational2Double  is defined!
		 ------------------------------------------------------*/
		if (blnIsRational2Double) {
			if (!blnWriteRationalDirectly) {
				if (!TIFFSetField(tif, TIFFTAG_RATIONAL_DOUBLE, auxDoubleArrayW[100])) {
					fprintf(stderr, "Can't set TIFFTAG_RATIONAL_DOUBLE tag.\n");
					goto failure;
				}
				/* test for plain integers */
				if (!TIFFSetField(tif, TIFFTAG_SRATIONAL_DOUBLE, (-1.0))) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_DOUBLE tag.\n");
					goto failure;
				}
				if (!TIFFSetField(tif, TIFFTAG_RATIONAL_C0_DOUBLE, &auxDoubleArrayW[110])) {
					fprintf(stderr, "Can't set TIFFTAG_RATIONAL_C0_DOUBLE tag.\n");
					goto failure;
				}
				auxDoubleArrayW[120] = -1.49;
				if (!TIFFSetField(tif, TIFFTAG_SRATIONAL_C16_DOUBLE, 2, &auxDoubleArrayW[120])) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_C16_DOUBLE tag.\n");
					goto failure;
				}
				auxFloatArrayW[120] = -5.88f;
				if (!TIFFSetField(tif, TIFFTAG_SRATIONAL_C0_FLOAT, &auxFloatArrayW[120])) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_C0_FLOAT tag.\n");
					goto failure;
				}
				if (!TIFFSetField(tif, TIFFTAG_SRATIONAL_C32_FLOAT, 2, &auxFloatArrayW[120])) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_C32_FLOAT tag.\n");
					goto failure;
				}
				if (!TIFFSetField(tif, TIFFTAG_SRATIONAL_C32_DOUBLE, 5, &auxDoubleArrayW[120])) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_C32_DOUBLE tag.\n");
					goto failure;
				}
			}
			else {
				/* Test here the direct rational input possibility for TIFFTAG_REFERENCEBLACKWHITE with a rational-array as input. */
				DoubleToRational(auxDoubleArrayW[100], &uN, &uD);
				if (!TIFFSetFieldRational(tif, TIFFTAG_RATIONAL_DOUBLE, uN, uD)) {
					fprintf(stderr, "Can't set TIFFTAG_RATIONAL_DOUBLE tag as RATIONAL.\n");
					goto failure;
				}
				DoubleToSrational((-1.0), &sN, &sD);
				if (!TIFFSetFieldRational(tif, TIFFTAG_SRATIONAL_DOUBLE, sN, sD)) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_DOUBLE tag as RATIONAL.\n");
					goto failure;
				}


				for (i = 0; i < 3; i++) {
					DoubleToRational(auxDoubleArrayW[110 + i], &auxRationalArray[i].uNum, &auxRationalArray[i].uDenom);
				}
				if (!TIFFSetFieldRational(tif, TIFFTAG_RATIONAL_C0_DOUBLE, auxRationalArray)) {
					fprintf(stderr, "Can't set TIFFTAG_RATIONAL_C0_DOUBLE tag as RATIONAL.\n");
					goto failure;
				}
				for (i = 0; i < 3; i++) {
					DoubleToSrational(auxDoubleArrayW[120 + i], &auxSRationalArray[i].sNum, &auxSRationalArray[i].sDenom);
				}
				if (!TIFFSetFieldRational(tif, TIFFTAG_SRATIONAL_C16_DOUBLE, 2, auxSRationalArray)) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_C16_DOUBLE tag as RATIONAL.\n");
					goto failure;
				}
				for (i = 0; i < 4; i++) {
					DoubleToSrational(auxFloatArrayW[120 + i], &auxSRationalArray[i].sNum, &auxSRationalArray[i].sDenom);
				}
				if (!TIFFSetFieldRational(tif, TIFFTAG_SRATIONAL_C0_FLOAT, auxSRationalArray)) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_C0_FLOAT tag as RATIONAL.\n");
					goto failure;
				}
				for (i = 0; i < 3; i++) {
					DoubleToSrational(auxFloatArrayW[120 + i], &auxSRationalArray[i].sNum, &auxSRationalArray[i].sDenom);
				}
				if (!TIFFSetFieldRational(tif, TIFFTAG_SRATIONAL_C32_FLOAT, 2, auxSRationalArray)) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_C32_FLOAT tag as RATIONAL.\n");
					goto failure;
				}
				for (i = 0; i < 6; i++) {
					DoubleToSrational(auxDoubleArrayW[110 + i], &auxSRationalArray[i].sNum, &auxSRationalArray[i].sDenom);
				}
				if (!TIFFSetFieldRational(tif, TIFFTAG_SRATIONAL_C32_DOUBLE, 5, auxSRationalArray)) {
					fprintf(stderr, "Can't set TIFFTAG_SRATIONAL_C32_DOUBLE tag as RATIONAL.\n");
					goto failure;
				}
			}
		}

	} else { /* blnAllCustomTags */
		/*==== Automatically check all custom rational tags == WRITING ===*/
		/*-- Get array, where TIFF tag fields are defined --*/
		tFieldArray = _TIFFGetFields();
		nTags = tFieldArray->count;

		for (i = 0; i < nTags; i++) {
			tTag = tFieldArray->fields[i].field_tag;
			tType = tFieldArray->fields[i].field_type;				/* e.g. TIFF_RATIONAL */
			tWriteCount = tFieldArray->fields[i].field_writecount;
			tSetFieldType = tFieldArray->fields[i].set_field_type;	/* e.g. TIFF_SETGET_C0_FLOAT */
			tFieldBit = tFieldArray->fields[i].field_bit;
			tFieldName = tFieldArray->fields[i].field_name;
			pVoid = NULL;

			if (tType == TIFF_RATIONAL && tFieldBit == FIELD_CUSTOM) {
				/*-- dependent on set_field_type write value --*/
				switch (tSetFieldType) {
					case TIFF_SETGET_FLOAT:
					case TIFF_SETGET_DOUBLE:
						if (tWriteCount == 1) {
							/*-- All single values can be written with float or double parameter. Only value range should be in line. */
							if (!TIFFSetField(tif, tTag, auxDoubleArrayW[i])) {
								fprintf(stderr, "Can't write %s\n", tFieldArray->fields[i].field_name);
								goto failure;
							}
						} else {
							fprintf(stderr, "WriteCount for .set_field_type %d should be 1!  %s\n", tSetFieldType, tFieldArray->fields[i].field_name);
						}
						break;
					case TIFF_SETGET_C0_FLOAT:
					case TIFF_SETGET_C0_DOUBLE:
					case TIFF_SETGET_C16_FLOAT:
					case TIFF_SETGET_C16_DOUBLE:
					case TIFF_SETGET_C32_FLOAT:
					case TIFF_SETGET_C32_DOUBLE:
						/* _Cxx_ just defines the size of the count parameter for the array as C0=char, C16=short or C32=long */
						/*-- Check, if it is a single parameter, a fixed array or a variable array */
						if (tWriteCount == 1) {
							fprintf(stderr, "WriteCount for .set_field_type %d should be -1 or greather than 1!  %s\n", tSetFieldType, tFieldArray->fields[i].field_name);
						} else {
							/*-- Either fix or variable array --*/
							/* For arrays, distinguishing between float or double is essential, even for writing */
							if (tSetFieldType == TIFF_SETGET_C0_FLOAT || tSetFieldType == TIFF_SETGET_C16_FLOAT || tSetFieldType == TIFF_SETGET_C32_FLOAT)
								pVoid = &auxFloatArrayW[i]; else pVoid = &auxDoubleArrayW[i];
							/* Now decide between fixed or variable array */
							if (tWriteCount > 1) {
								/* fixed array with needed arraysize defined in .field_writecount */
								if (!TIFFSetField(tif, tTag, pVoid)) {
									fprintf(stderr, "Can't write %s\n", tFieldArray->fields[i].field_name);
									goto failure;
								}
							} else {
								/* special treatment of variable array */
								/* for test, use always arraysize of VARIABLE_ARRAY_SIZE */
								if (!TIFFSetField(tif, tTag, VARIABLE_ARRAY_SIZE, pVoid)) {
									fprintf(stderr, "Can't write %s\n", tFieldArray->fields[i].field_name);
									goto failure;
								}
							}
						}
						break;
					default:
						fprintf(stderr, "SetFieldType %d not defined within writing switch for %s.\n", tSetFieldType, tFieldName);
				};  /*-- switch() --*/
			} /* if () */
		} /*-- for() --*/
	}	/* blnAllCustomTags */  /*==== END END - Automatically check all custom rational tags  == WRITING END ===*/

	/*-- Write dummy pixel data. --*/
	if (TIFFWriteScanline(tif, buf, 0, 0) < 0) {
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


	/*-- WriteLongReadRational: Change field_type to rational for next DefaultDirectory creation for reading. --*/
	nTags = N(tifFieldInfo);
	for (i = IDX_START; i < nTags; i++) {
		if (i < IDX_START_SIGNED) {
			tifFieldInfo[i].field_type = TIFF_RATIONAL;
			tifFieldInfo[i].set_field_type = TIFF_SETGET_FLOAT;
		} else if (i < IDX_START_FLOAT) {
			tifFieldInfo[i].field_type = TIFF_SRATIONAL;
			tifFieldInfo[i].set_field_type = TIFF_SETGET_FLOAT;
		} else {
			tifFieldInfo[i].field_type = TIFF_SRATIONAL;
			tifFieldInfo[i].set_field_type = TIFF_SETGET_DOUBLE;
		}
	}

	fprintf (stderr, "-------- Continue Test  ---------- reading ...\n");

/*=========================  READING  =============  READING  ========================================*/
	/* Ok, now test whether we can read written values in the EXIF directory. */
	tif = TIFFOpen(filenameRead, "r");

	/*-- Read some parameters out of the main directory --*/

	/*-- IMAGEWIDTH and -LENGTH are defined as TIFF_SETGET_UINT32 */
	retCode = TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &auxUint32 );
	if (auxUint32 != width) {
		fprintf (stderr, "Read value of IMAGEWIDTH %d differs from set value %d\n", auxUint32, width);
		GOTOFAILURE
	}
	retCode = TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &auxUint32 );
	if (auxUint32 != width) {
		fprintf (stderr, "Read value of TIFFTAG_IMAGELENGTH %d differs from set value %d\n", auxUint32, length);
		GOTOFAILURE
	}

	/*--- Standard tags with TIFF_RATIONAL and TIFF_SETGET_DOUBLE to TIFF_SETGET_FLOAT change. ---
	 *     They can be written either using float or double but have to be read using float.
	-------------------------------------------------------------------------------------------- */
	dblDiffLimit = RATIONAL_EPS;
	retCode = TIFFGetField(tif, TIFFTAG_XRESOLUTION, &auxFloat);
	dblDiff = auxFloat - auxFloatArrayResolutions[0];
	if (fabs(dblDiff) > fabs(dblDiffLimit)) {
		fprintf(stderr, "Read value of TIFFTAG_XRESOLUTION %f differs from set value %f\n", auxFloat, auxFloatArrayResolutions[0]);
		GOTOFAILURE
	}
	/* Test here the direct rational input/output possibility */
	retCode = TIFFGetFieldRational(tif, TIFFTAG_XRESOLUTION, &uN2, &uD2);
	auxDouble = (double)uN2 / (double)uD2;
	dblDiff = auxDouble - auxFloatArrayResolutions[0];
	if (fabs(dblDiff) > fabs(dblDiffLimit)) {
		fprintf(stderr, "DirectRational Read value of TIFFTAG_XRESOLUTION %f differs from set value %f\n", auxDouble, auxFloatArrayResolutions[0]);
		GOTOFAILURE
	}



	retCode = TIFFGetField(tif, TIFFTAG_YRESOLUTION, &auxFloat);
	dblDiff = (double)auxFloat - auxFloatArrayResolutions[1];
	if (fabs(dblDiff) > fabs(dblDiffLimit)) {
		fprintf(stderr, "Read value of TIFFTAG_YRESOLUTION %f differs from set value %f\n", auxFloat, auxFloatArrayResolutions[1]);
		GOTOFAILURE
	}
	retCode = TIFFGetField(tif, TIFFTAG_XPOSITION, &auxFloat);
	dblDiff = (double)auxFloat - auxFloatArrayResolutions[2];
	if (fabs(dblDiff) > fabs(dblDiffLimit)) {
		fprintf(stderr, "Read value of TIFFTAG_XPOSITION %f differs from set value %f\n", auxFloat, auxFloatArrayResolutions[2]);
		GOTOFAILURE
	}
	retCode = TIFFGetField(tif, TIFFTAG_YPOSITION, &auxFloat);
	dblDiff = (double)auxFloat - auxFloatArrayResolutions[3];
	if (fabs(dblDiff) > fabs(dblDiffLimit)) {
		fprintf(stderr, "Read value of TIFFTAG_YPOSITION %f differs from set value %f\n", auxFloat, auxFloatArrayResolutions[3]);
		GOTOFAILURE
	}

	/* Fixed Array of 6 rationals is TIFFTAG_REFERENCEBLACKWHITE Read and compare. */
	retCode = TIFFGetField(tif, TIFFTAG_REFERENCEBLACKWHITE, &pVoidArray);
	/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
	if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_REFERENCEBLACKWHITE \n"); GOTOFAILURE }
	else
		memcpy(&auxFloatArray, pVoidArray, (6 * sizeof(auxFloatArray[0])));
	for (i = 0; i < 6; i++) {
		dblDiffLimit = RATIONAL_EPS * auxFloatArrayReferenceBlackWhite[i];
		dblDiff = auxFloatArray[i] - auxFloatArrayReferenceBlackWhite[i];
		if (fabs(dblDiff) > fabs(dblDiffLimit)) {
			fprintf(stderr, "Read value %d of TIFFTAG_REFERENCEBLACKWHITE Array %f differs from set value %f\n", i, auxFloatArray[i], auxFloatArrayReferenceBlackWhite[i]);
			GOTOFAILURE
		}
	}
	/* Test here the direct rational input/output possibility for TIFFTAG_REFERENCEBLACKWHITE with a fixed rational-array. */
	retCode = TIFFGetFieldRational(tif, TIFFTAG_REFERENCEBLACKWHITE, &pVoidArray);
	/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
	if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_REFERENCEBLACKWHITE \n"); GOTOFAILURE }
	else
		memcpy(&auxRationalArray, pVoidArray, (6 * sizeof(TIFFRational_t)));
	for (i = 0; i < 6; i++) {
		dblDiffLimit = RATIONAL_EPS * auxFloatArrayReferenceBlackWhite[i];
		auxDouble = ((double)auxRationalArray[i].uNum / (double)auxRationalArray[i].uDenom);
		dblDiff = auxDouble - auxFloatArrayReferenceBlackWhite[i];
		if (fabs(dblDiff) > fabs(dblDiffLimit)) {
			fprintf(stderr, "Direct Rational Read value %d of TIFFTAG_REFERENCEBLACKWHITE Array %f differs from set value %f\n", i, auxDouble, auxFloatArrayReferenceBlackWhite[i]);
			GOTOFAILURE
		}
	}



	/*- TIFFTAG_INKSET is a SHORT parameter (TIFF_SHORT, TIFF_SETGET_UINT16) with field_bit=FIELD_CUSTOM !! -*/
	retCode = TIFFGetField(tif, TIFFTAG_INKSET, &auxUint16);
	if (auxUint16 != 34) {
		fprintf(stderr, "Read value of TIFFTAG_PIXAR_FOVCOT %d differs from set value %d\n", auxUint16, TIFFTAG_INKSET);
		GOTOFAILURE
	}

	/*- TIFFTAG_PIXAR_FOVCOT is a FLOAT parameter ( TIFF_FLOAT, TIFF_SETGET_FLOAT) with field_bit=FIELD_CUSTOM !! -*/
	/*  - was written with Double but has to be read with Float                                                    */
	retCode = TIFFGetField(tif, TIFFTAG_PIXAR_FOVCOT, &auxFloat );
	if (auxFloat != (float)PIXAR_FOVCOT_VAL) {
		fprintf (stderr, "Read value of TIFFTAG_PIXAR_FOVCOT %f differs from set value %f\n", auxFloat, PIXAR_FOVCOT_VAL);
		GOTOFAILURE
	}

	/*- TIFFTAG_STONITS is a DOUBLE parameter (TIFF_DOUBLE, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM!! -*/
	retCode = TIFFGetField(tif, TIFFTAG_STONITS, &auxDouble);
	if (auxDouble != (double)STONITS_VAL) {
		fprintf(stderr, "Read value of TIFFTAG_STONITS %f differs from set value %f\n", auxDouble, STONITS_VAL);
		GOTOFAILURE
	}




	/*-- Check, if the TiffLibrary is compiled with the new interface with Rational2Double or still uses the old definitions. */
	/*   tags to check: TIFFTAG_BESTQUALITYSCALE, TIFFTAG_BASELINENOISE, TIFFTAG_BASELINESHARPNESS, */
	fip = TIFFFindField(tif, TIFFTAG_BESTQUALITYSCALE, TIFF_ANY);
	tSetFieldType = fip->set_field_type;
	if (tSetFieldType == TIFF_SETGET_DOUBLE)
		blnIsRational2Double = FALSE;
	else
		blnIsRational2Double = TRUE;


	if (!blnAllCustomTags) {

		/*- TIFFTAG_BESTQUALITYSCALE is a Rational parameter (TIFF_RATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM!
			and written with double parameter -*/
		/*- Read into a union to test the correct precision (float  or double) returned.
		 *   Float-parameter should be correct, but double-parameter should give a wrong value
		 */
		auxDblUnion.dbl = 0;
		retCode = TIFFGetField(tif, TIFFTAG_BESTQUALITYSCALE, &auxDblUnion.dbl);
		dblDiffLimit = RATIONAL_EPS * (double)BESTQUALITYSCALE_VAL;
		dblDiff = auxDblUnion.dbl - (double)BESTQUALITYSCALE_VAL;
		fltDiff = auxDblUnion.flt1 - (float)BESTQUALITYSCALE_VAL;
		if (!((fabs(dblDiff) > fabs(dblDiffLimit)) && !(fabs(fltDiff) > fabs(dblDiffLimit)))) {
			fprintf(stderr, "Float-Read value of TIFFTAG_BESTQUALITYSCALE %.12f differs from set value %.12f too much,\n", auxDblUnion.flt1, BESTQUALITYSCALE_VAL);
			fprintf(stderr, "whereas Double-Read value of TIFFTAG_BESTQUALITYSCALE %.12f is nearly equal to set value %.12f\n", auxDblUnion.dbl, BESTQUALITYSCALE_VAL);
			GOTOFAILURE
		}

		/*--- Now the same for a Signed Rational ---*/
		/*- TIFFTAG_BASELINEEXPOSURE is a Rational parameter (TIFF_SRATIONAL, TIFF_SETGET_DOUBLE) with field_bit=FIELD_CUSTOM! -
			and written with double parameter - */
		/*- Read into a union to test the correct precision (float  or double) returned.
		 *   Float-parameter should be correct, but double-parameter should give a wrong value
		 */
		auxDblUnion.dbl = 0;
		retCode = TIFFGetField(tif, TIFFTAG_BASELINEEXPOSURE, &auxDblUnion.dbl);
		dblDiffLimit = RATIONAL_EPS * (double)BASELINEEXPOSURE_VAL;
		dblDiff = auxDblUnion.dbl - (double)BASELINEEXPOSURE_VAL;
		fltDiff = auxDblUnion.flt1 - (float)BASELINEEXPOSURE_VAL;
		if (!((fabs(dblDiff) > fabs(dblDiffLimit)) && !(fabs(fltDiff) > fabs(dblDiffLimit)))) {
			fprintf(stderr, "Float-Read value of TIFFTAG_BASELINEEXPOSURE %.12f differs from set value %.12f too much,\n", auxDblUnion.flt1, BASELINEEXPOSURE_VAL);
			fprintf(stderr, "whereas Double-Read value of TIFFTAG_BESTQUALITYSCALE %.12f is nearly equal to set value %.12f\n", auxDblUnion.dbl, BASELINEEXPOSURE_VAL);
			GOTOFAILURE
		}

		/*- Variable Array: TIFFTAG_DECODE is a SRATIONAL parameter TIFF_SETGET_C16_FLOAT type FIELD_CUSTOM with passcount=1 and variable length of array. */
		retCode = TIFFGetField(tif, TIFFTAG_DECODE, &count16, &pVoidArray);
		/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
		if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_DECODE \n"); GOTOFAILURE }
		else
			memcpy(&auxFloatArray, pVoidArray, (count16 * sizeof(auxFloatArray[0])));
		for (i = 0; i < count16; i++) {
			dblDiffLimit = RATIONAL_EPS * auxFloatArrayS1[i];
			dblDiff = (double)auxFloatArray[i] - auxFloatArrayS1[i];
			if (fabs(dblDiff) > fabs(dblDiffLimit)) {
				fprintf(stderr, "Read value %d of TIFFTAG_DECODE Array %f differs from set value %f\n", i, auxFloatArray[i], auxFloatArrayS1[i]);
				GOTOFAILURE
			}
		}
		/*- Variable Array with Rational directly reading -*/
		retCode = TIFFGetFieldRational(tif, TIFFTAG_DECODE, &count16, &pVoidArray);
		/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
		if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_DECODE \n"); GOTOFAILURE }
		else
			memcpy(&auxSRationalArray, pVoidArray, (count16 * sizeof(TIFFRational_t)));
		for (i = 0; i < count16; i++) {
			dblDiffLimit = RATIONAL_EPS * auxFloatArrayS1[i];
			auxDouble = ((double)auxSRationalArray[i].sNum / (double)auxSRationalArray[i].sDenom);
			dblDiff = auxDouble - auxFloatArrayS1[i];
			if (fabs(dblDiff) > fabs(dblDiffLimit)) {
				fprintf(stderr, "Direct Rational Read value %d of TIFFTAG_DECODE Array %f differs from set value %f\n", i, auxDouble, auxFloatArrayS1[i]);
				GOTOFAILURE
			}
		}


		retCode = TIFFGetField(tif, TIFFTAG_BLACKLEVEL, &count16, &pVoidArray);
		/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
		if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_BLACKLEVEL \n"); GOTOFAILURE }
		else
			memcpy(&auxFloatArray, pVoidArray, (count16 * sizeof(auxFloatArray[0])));
		for (i = 0; i < count16; i++) {
			dblDiffLimit = RATIONAL_EPS * auxFloatArrayN1[i];
			dblDiff = (double)auxFloatArray[i] - auxFloatArrayN1[i];
			if (fabs(dblDiff) > fabs(dblDiffLimit)) {
				fprintf(stderr, "Read value %d of TIFFTAG_BLACKLEVEL Array %f differs from set value %f\n", i, auxFloatArray[i], auxFloatArrayN1[i]);
				GOTOFAILURE
			}
		}
		/*- Variable Array with Rational directly reading -*/
		retCode = TIFFGetFieldRational(tif, TIFFTAG_BLACKLEVEL, &count16, &pVoidArray);
		/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
		if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_BLACKLEVEL \n"); GOTOFAILURE }
		else
			memcpy(&auxRationalArray, pVoidArray, (count16 * sizeof(TIFFRational_t)));
		for (i = 0; i < count16; i++) {
			dblDiffLimit = RATIONAL_EPS * auxFloatArrayN1[i];
			auxDouble = ((double)auxRationalArray[i].uNum / (double)auxRationalArray[i].uDenom);
			dblDiff = auxDouble - auxFloatArrayN1[i];
			if (fabs(dblDiff) > fabs(dblDiffLimit)) {
				fprintf(stderr, "Direct Rational Read value %d of TIFFTAG_BLACKLEVEL Array %f differs from set value %f\n", i, auxDouble, auxFloatArrayN1[i]);
				GOTOFAILURE
			}
		}



		/*--  WriteLongReadRational: Read tags written as BYTE, LONG, FLOAT as rational. --*/
		nTags = N(tifFieldInfo);
		for (i = IDX_START; i < nTags; i++) {
			fip = TIFFFindField(tif, tifFieldInfo[i].field_tag, TIFF_ANY); /* debugging */
			if (i < IDX_START_SIGNED) {
				retCode = TIFFGetField(tif, tifFieldInfo[i].field_tag, &auxFloat);
				if (auxFloat != i) {
					fprintf(stderr, "Read value of TIFFTAG_xxx(%d): %f differs from set value %d\n", tifFieldInfo[i].field_tag, auxFloat, i);
					GOTOFAILURE
				}
			} else if (i < IDX_START_FLOAT) {
				retCode = TIFFGetField(tif, tifFieldInfo[i].field_tag, &auxFloat);
				if (auxFloat != -i) {
					fprintf(stderr, "Read value of TIFFTAG_xxx(%d): %f differs from set value %d\n", tifFieldInfo[i].field_tag, auxFloat, -i);
					GOTOFAILURE
				}
			} else {
				retCode = TIFFGetField(tif, tifFieldInfo[i].field_tag, &auxDouble);
				if (auxDouble != (-2.3 * i)) {
					fprintf(stderr, "Read value of TIFFTAG_xxx(%d): %f differs from set value %f\n", tifFieldInfo[i].field_tag, auxDouble, (-2.3 * i));
					GOTOFAILURE
				}
			}
		}



		/*--- Read now additional Rational2Double test tags ---
		      This should be now with nearly double precision
			  However, this additional tags are only read as Double,
			  if blnIsRational2Double  is defined!
	     ------------------------------------------------------*/
		if (blnIsRational2Double) {
			auxDblUnion.dbl = 0;
			retCode = TIFFGetField(tif, TIFFTAG_RATIONAL_DOUBLE, &auxDblUnion.dbl);
			if (!retCode) { fprintf(stderr, "Can't read %s\n", "TIFFTAG_RATIONAL_DOUBLE"); GOTOFAILURE }
			dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[100];
			dblDiff = auxDblUnion.dbl - auxDoubleArrayW[100];
			if (fabs(dblDiff) > fabs(dblDiffLimit)) {
				fprintf(stderr, "Read value of TIFFTAG_RATIONAL_DOUBLE %f differs from set value %f\n", auxDblUnion.dbl, auxDoubleArrayW[100]);
				GOTOFAILURE
			}
			retCode = TIFFGetFieldRational(tif, TIFFTAG_RATIONAL_DOUBLE, &uN2, &uD2);
			auxDouble = (double)uN2 / (double)uD2;
			dblDiff = auxDouble - auxDoubleArrayW[100];
			if (fabs(dblDiff) > fabs(dblDiffLimit)) {
				fprintf(stderr, "DirectRational Read value of TIFFTAG_RATIONAL_DOUBLE %f differs from set value %f\n", auxDouble, auxDoubleArrayW[100]);
				GOTOFAILURE
			}

			auxDblUnion.dbl = 0;
			retCode = TIFFGetField(tif, TIFFTAG_SRATIONAL_DOUBLE, &auxDblUnion.dbl);
			if (!retCode) { fprintf(stderr, "Can't read %s\n", "TIFFTAG_SRATIONAL_DOUBLE"); GOTOFAILURE }
			auxDouble = -1.0;
			dblDiffLimit = RATIONAL_EPS * auxDouble;
			dblDiff = auxDblUnion.dbl - auxDouble;
			if (fabs(dblDiff) > fabs(dblDiffLimit)) {
				fprintf(stderr, "Read value of TIFFTAG_SRATIONAL_DOUBLE %f differs from set value %f\n", auxDblUnion.dbl, auxDouble);
				GOTOFAILURE
			}
			retCode = TIFFGetFieldRational(tif, TIFFTAG_SRATIONAL_DOUBLE, &sN, &sD);
			auxDouble = (double)sN / (double)sD;
			dblDiff = auxDouble - (-1.0);
			if (fabs(dblDiff) > fabs(dblDiffLimit)) {
				fprintf(stderr, "DirectRational Read value of TIFFTAG_SRATIONAL_DOUBLE %f differs from set value %f\n", auxDouble, (-1.0));
				GOTOFAILURE
			}

			/*- Fixed Array: TIFFTAG_RATIONAL_C0_DOUBLE, 3, 3, TIFF_RATIONAL, 0, TIFF_SETGET_C0_DOUBLE */
			count16 = 3; /* set fixed array length for checking */
			retCode = TIFFGetField(tif, TIFFTAG_RATIONAL_C0_DOUBLE, &pVoidArray);
			/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
			if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_RATIONAL_C0_DOUBLE \n"); GOTOFAILURE }
			else
				memcpy(&auxDoubleArray, pVoidArray, (count16 * sizeof(auxDoubleArray[0])));
			for (i = 0; i < count16; i++) {
				dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[110 + i];
				dblDiff = auxDoubleArray[i] - auxDoubleArrayW[110 + i];
				if (fabs(dblDiff) > fabs(dblDiffLimit)) {
					fprintf(stderr, "Read value %d of TIFFTAG_RATIONAL_C0_DOUBLE Array %f differs from set value %f\n", i, auxDoubleArray[i], auxDoubleArrayW[110 + i]);
					GOTOFAILURE
				}
			}
			retCode = TIFFGetFieldRational(tif, TIFFTAG_RATIONAL_C0_DOUBLE, &pVoidArray);
			/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
			if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_RATIONAL_C0_DOUBLE \n"); GOTOFAILURE }
			else
				memcpy(&auxRationalArray, pVoidArray, (count16 * sizeof(TIFFRational_t)));
			for (i = 0; i < count16; i++) {
				dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[110 + i];
				auxDouble = ((double)auxRationalArray[i].uNum / (double)auxRationalArray[i].uDenom);
				dblDiff = auxDouble - auxDoubleArrayW[110 + i];
				if (fabs(dblDiff) > fabs(dblDiffLimit)) {
					fprintf(stderr, "Direct Rational Read value %d of TIFFTAG_RATIONAL_C0_DOUBLE Array %f differs from set value %f\n", i, auxDouble, auxDoubleArrayW[110 + i]);
					GOTOFAILURE
				}
			}

			/*- Variable Array: TIFFTAG_SRATIONAL_C16_DOUBLE, -1, -1, TIFF_SRATIONAL, 0, TIFF_SETGET_C16_DOUBLE  */
			retCode = TIFFGetField(tif, TIFFTAG_SRATIONAL_C16_DOUBLE, &count16, &pVoidArray);
			/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
			if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_SRATIONAL_C16_DOUBLE \n"); GOTOFAILURE }
			else
				memcpy(&auxDoubleArray, pVoidArray, (count16 * sizeof(auxDoubleArray[0])));
			for (i = 0; i < count16; i++) {
				dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[120 + i];
				dblDiff = auxDoubleArray[i] - auxDoubleArrayW[120 + i];
				if (fabs(dblDiff) > fabs(dblDiffLimit)) {
					fprintf(stderr, "Read value %d of TIFFTAG_SRATIONAL_C16_DOUBLE Array %f differs from set value %f\n", i, auxDoubleArray[i], auxDoubleArrayW[120 + i]);
					GOTOFAILURE
				}
			}
			retCode = TIFFGetFieldRational(tif, TIFFTAG_SRATIONAL_C16_DOUBLE, &count16, &pVoidArray);
			/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
			if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_SRATIONAL_C16_DOUBLE \n"); GOTOFAILURE }
			else
				memcpy(&auxSRationalArray, pVoidArray, (count16 * sizeof(TIFFRational_t)));
			for (i = 0; i < count16; i++) {
				dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[120 + i];
				auxDouble = ((double)auxSRationalArray[i].sNum / (double)auxSRationalArray[i].sDenom);
				dblDiff = auxDouble - auxDoubleArrayW[120 + i];
				if (fabs(dblDiff) > fabs(dblDiffLimit)) {
					fprintf(stderr, "Direct Rational Read value %d of TIFFTAG_SRATIONAL_C16_DOUBLE Array %f differs from set value %f\n", i, auxDouble, auxDoubleArrayW[120 + i]);
					GOTOFAILURE
				}
			}

			/*- Fixed Array: TIFFTAG_SRATIONAL_C0_FLOAT, 4, 4, TIFF_SRATIONAL, 0, TIFF_SETGET_C0_FLOAT */
			count16 = 4; /* set fixed array length for checking */
			retCode = TIFFGetField(tif, TIFFTAG_SRATIONAL_C0_FLOAT, &pVoidArray);
			/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
			if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_SRATIONAL_C0_FLOAT \n"); GOTOFAILURE }
			else
				memcpy(&auxFloatArray, pVoidArray, (count16 * sizeof(auxFloatArray[0])));
			for (i = 0; i < count16; i++) {
				dblDiffLimit = RATIONAL_EPS * auxFloatArrayW[120 + i];
				dblDiff = (double)auxFloatArray[i] - auxFloatArrayW[120 + i];
				if (fabs(dblDiff) > fabs(dblDiffLimit)) {
					fprintf(stderr, "Read value %d of TIFFTAG_SRATIONAL_C0_FLOAT Array %f differs from set value %f\n", i, auxFloatArray[i], auxFloatArrayW[120 + i]);
					GOTOFAILURE
				}
			}
			retCode = TIFFGetFieldRational(tif, TIFFTAG_SRATIONAL_C0_FLOAT, &pVoidArray);
			/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
			if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_SRATIONAL_C0_FLOAT \n"); GOTOFAILURE }
			else
				memcpy(&auxSRationalArray, pVoidArray, (count16 * sizeof(TIFFSRational_t)));
			for (i = 0; i < count16; i++) {
				dblDiffLimit = RATIONAL_EPS * auxFloatArrayW[120 + i];
				auxDouble = ((double)auxSRationalArray[i].sNum / (double)auxSRationalArray[i].sDenom);
				dblDiff = auxDouble - auxFloatArrayW[120 + i];
				if (fabs(dblDiff) > fabs(dblDiffLimit)) {
					fprintf(stderr, "Read value %d of TIFFTAG_SRATIONAL_C0_FLOAT Array %f differs from set value %f\n", i, auxFloatArray[i], auxFloatArrayW[120 + i]);
					GOTOFAILURE
				}
			}

			/*- Variable Array: TIFFTAG_SRATIONAL_C32_FLOAT, -1, -1, TIFF_SRATIONAL, 0, TIFF_SETGET_C32_FLOAT  */
			retCode = TIFFGetField(tif, TIFFTAG_SRATIONAL_C32_FLOAT, &count32, &pVoidArray);
			/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
			if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_SRATIONAL_C32_FLOAT \n"); GOTOFAILURE }
			else
				memcpy(&auxFloatArray, pVoidArray, (count32 * sizeof(auxFloatArray[0])));
			for (i = 0; i < (int)count32; i++) {
				dblDiffLimit = RATIONAL_EPS * auxFloatArrayW[120 + i];
				dblDiff = auxFloatArray[i] - auxFloatArrayW[120 + i];
				if (fabs(dblDiff) > fabs(dblDiffLimit)) {
					fprintf(stderr, "Read value %d of TIFFTAG_SRATIONAL_C32_FLOAT Array %f differs from set value %f\n", i, auxFloatArray[i], auxFloatArrayW[120 + i]);
					GOTOFAILURE
				}
			}
			retCode = TIFFGetFieldRational(tif, TIFFTAG_SRATIONAL_C32_FLOAT, &count32, &pVoidArray);
			/*- pVoidArray points to a Tiff-internal temporary memorypart. Thus, contents needs to be saved. */
			if (!retCode) { fprintf(stderr, "Error TiffGetField for TIFFTAG_SRATIONAL_C32_FLOAT \n"); GOTOFAILURE }
			else
				memcpy(&auxSRationalArray, pVoidArray, (count32 * sizeof(TIFFRational_t)));
			for (i = 0; i < (int)count32; i++) {
				dblDiffLimit = RATIONAL_EPS * auxFloatArrayW[120 + i];
				auxDouble = ((double)auxSRationalArray[i].sNum / (double)auxSRationalArray[i].sDenom);
				dblDiff = auxDouble - auxFloatArrayW[120 + i];
				if (fabs(dblDiff) > fabs(dblDiffLimit)) {
					fprintf(stderr, "Direct Rational Read value %d of TIFFTAG_SRATIONAL_C32_FLOAT Array %f differs from set value %f\n", i, auxDouble, auxFloatArrayW[120 + i]);
					GOTOFAILURE
				}
			}

		} /*-- if (blnIsRational2Double) --*/
	} else {  /* blnAllCustomTags */
		/*==== Automatically check all custom rational tags == READING ===*/

		/*-- Get array, where standard TIFF tag fields are defined --*/
		tFieldArray = _TIFFGetFields();
		nTags = tFieldArray->count;

		for (i = 0; i < nTags; i++) {
			tTag = tFieldArray->fields[i].field_tag;
			tType = tFieldArray->fields[i].field_type;				/* e.g. TIFF_RATIONAL */
			tWriteCount = tFieldArray->fields[i].field_writecount;
			tSetFieldType = tFieldArray->fields[i].set_field_type;	/* e.g. TIFF_SETGET_C0_FLOAT */
			tFieldBit = tFieldArray->fields[i].field_bit;
			tFieldName = tFieldArray->fields[i].field_name;
			pVoid = NULL;
			auxDblUnion.dbl = 0;

			if (tType == TIFF_RATIONAL && tFieldBit == FIELD_CUSTOM) {
				/*-- dependent on set_field_type read value --*/
				switch (tSetFieldType) {
					case TIFF_SETGET_FLOAT:
						if (!TIFFGetField(tif, tTag, &auxFloat)) {
							fprintf(stderr, "Can't read %s\n", tFieldArray->fields[i].field_name);
							GOTOFAILURE
							break;
						}
						/* compare read values with written ones */
						if (tType == TIFF_RATIONAL || tType == TIFF_SRATIONAL) dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[i]; else dblDiffLimit = 1e-6;
						dblDiff = auxFloat - auxDoubleArrayW[i];
						if (fabs(dblDiff) > fabs(dblDiffLimit)) {
							/*--: EXIFTAG_SUBJECTDISTANCE: LibTiff returns value of "-1.0" if numerator equals 4294967295 (0xFFFFFFFF) to indicate infinite distance!
							 * However, there are two other EXIF tags where numerator indicates a special value and six other cases where the denominator indicates special values,
							 * which are not treated within LibTiff!!
							*/
							if (!(tTag == EXIFTAG_SUBJECTDISTANCE && auxFloat == -1.0)) {
								fprintf(stderr, "%d:Read value of %s %f differs from set value %f\n", i, tFieldName, auxFloat, auxDoubleArrayW[i]);
								GOTOFAILURE
							}
						}
						break;
					case TIFF_SETGET_DOUBLE:
						/*-- Unfortunately, TIFF_SETGET_DOUBLE is used for TIFF_RATIONAL but those have to be read with FLOAT !!! */
						/*   Only after update with Rational2Double feature, also TIFF_RATIONAL can be read in double precision!!! */
						/*   Therefore, use a union to avoid overflow in TIFFGetField() return value
						 *   and depending on version check for the right interface here:
						 *   - old interface:  correct value should be here a float
						 *   - new interface:  correct value should be here a double
						 *   Interface version (old/new) is determined above.
						 */
						if (!TIFFGetField(tif, tTag, &auxDblUnion.dbl)) {
							fprintf(stderr, "Can't read %s\n", tFieldArray->fields[i].field_name);
							GOTOFAILURE
							break;
						}
						if (tType == TIFF_RATIONAL || tType == TIFF_SRATIONAL) {
							if (blnIsRational2Double) {
								/* New interface allows also double precision for TIFF_RATIONAL */
								auxDouble = auxDblUnion.dbl;
							}
							else {
								/* Old interface reads TIFF_RATIONAL defined as TIFF_SETGET_DOUBLE alwasy as FLOAT */
								auxDouble = (double)auxDblUnion.flt1;
							}
						} else {
							auxDouble = auxDblUnion.dbl;
						}
						/* compare read values with written ones */
						if (tType == TIFF_RATIONAL || tType == TIFF_SRATIONAL) dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[i]; else dblDiffLimit = 1e-6;
						dblDiff = auxDouble - auxDoubleArrayW[i];
						if (fabs(dblDiff) > fabs(dblDiffLimit)) {
							/*--: EXIFTAG_SUBJECTDISTANCE: LibTiff returns value of "-1.0" if numerator equals 4294967295 (0xFFFFFFFF) to indicate infinite distance! */
							if (!(tTag == EXIFTAG_SUBJECTDISTANCE && auxDouble == -1.0))
								fprintf(stderr, "%d:Read value of %s %f differs from set value %f\n", i, tFieldName, auxDouble, auxDoubleArrayW[i]);
								GOTOFAILURE
						}
						break;

					case TIFF_SETGET_C0_FLOAT:
					case TIFF_SETGET_C0_DOUBLE:
					case TIFF_SETGET_C16_FLOAT:
					case TIFF_SETGET_C16_DOUBLE:
					case TIFF_SETGET_C32_FLOAT:
					case TIFF_SETGET_C32_DOUBLE:
						/* _Cxx_ just defines the size of the count parameter for the array as C0=char, C16=short or C32=long */
						/*-- Check, if it is a single parameter, a fixed array or a variable array */
						if (tWriteCount == 1) {
							fprintf(stderr, "Reading: WriteCount for .set_field_type %d should be -1 or greather than 1!  %s\n", tSetFieldType, tFieldArray->fields[i].field_name);
							GOTOFAILURE
						} else {
							/*-- Either fix or variable array --*/
							/* For arrays, distinguishing between float or double is essential. */
							/* Now decide between fixed or variable array */
							if (tWriteCount > 1) {
								/* fixed array with needed arraysize defined in .field_writecount */
								if (!TIFFGetField(tif, tTag, &pVoidArray)) {
									fprintf(stderr, "Can't read %s\n", tFieldArray->fields[i].field_name);
									GOTOFAILURE
									break;
								}
								/* set tWriteCount to number of read samples for next steps */
								auxLong = tWriteCount;
							} else {
								/* Special treatment of variable array. */
								/* Dependent on Cxx, the count parameter is char, short or long. Therefore use unionLong! */
								if (!TIFFGetField(tif, tTag, &auxLongUnion, &pVoidArray)) {
									fprintf(stderr, "Can't read %s\n", tFieldArray->fields[i].field_name);
									GOTOFAILURE
									break;
								}
								/* set tWriteCount to number of read samples for next steps */
								auxLong = auxLongUnion.Short1;
							}
							/* Save values from temporary array */
							if (tSetFieldType == TIFF_SETGET_C0_FLOAT || tSetFieldType == TIFF_SETGET_C16_FLOAT || tSetFieldType == TIFF_SETGET_C32_FLOAT) {
								memcpy(&auxFloatArray, pVoidArray, (auxLong * sizeof(auxFloatArray[0])));
								/* compare read values with written ones */
								if (tType == TIFF_RATIONAL || tType == TIFF_SRATIONAL) dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[i]; else dblDiffLimit = 1e-6;
								for (j = 0; j < auxLong; j++) {
									dblDiff = auxFloatArray[j] - auxFloatArrayW[i + j];
									if (fabs(dblDiff) > fabs(dblDiffLimit)) {
										/*if (auxFloatArray[j] != (float)auxFloatArrayW[i+j]) { */
										fprintf(stderr, "Read value %d of %s #%d %f differs from set value %f\n", i, tFieldName, j, auxFloatArray[j], auxFloatArrayW[i + j]);
										GOTOFAILURE
									}
								}
							} else {
								memcpy(&auxDoubleArray, pVoidArray, (auxLong * sizeof(auxDoubleArray[0])));
								/* compare read values with written ones */
								if (tType == TIFF_RATIONAL || tType == TIFF_SRATIONAL) dblDiffLimit = RATIONAL_EPS * auxDoubleArrayW[i]; else dblDiffLimit = 1e-6;
								for (j = 0; j < auxLong; j++) {
									dblDiff = auxDoubleArray[j] - auxDoubleArrayW[i + j];
									if (fabs(dblDiff) > fabs(dblDiffLimit)) {
										/*if (auxDoubleArray[j] != auxDoubleArrayW[i+j]) { */
										fprintf(stderr, "Read value %d of %s #%d %f differs from set value %f\n", i, tFieldName, j, auxDoubleArray[j], auxDoubleArrayW[i + j]);
										GOTOFAILURE
									}
								}
							}
						}
						break;
					default:
						fprintf(stderr, "SetFieldType %d not defined within reading switch for %s.\n", tSetFieldType, tFieldName);
				};  /*-- switch() --*/
			} /* if () */
		} /*-- for() --*/


	}	/* blnAllCustomTags */ /*==== END END - Automatically check all custom rational tags == READING  END ===*/



	TIFFClose(tif);
	
	/* All tests passed; delete file and exit with success status. */
#ifdef FOR_AUTO_TESTING
	unlink(filenameRead);
#endif
	fprintf(stderr, "-------- Test finished OK ----------\n");
	return 0;

failure:
	/* 
	 * Something goes wrong; close file and return unsuccessful status.
	 * Do not remove the file for further manual investigation.
	 */
	TIFFClose(tif);
	fprintf(stderr, "-------- Test finished with FAILURE --------\n");
	return 1;
}
