/*
 * Copyright (c) 1988-1997 Sam Leffler
 * Copyright (c) 1991-1997 Silicon Graphics, Inc.
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
 * TIFF Library.
 *
 * Directory Tag Get & Set Routines.
 * (and also some miscellaneous stuff)
 */
#include "tiffiop.h"
#include <float.h>	/*--: for Rational2Double */

/*
 * These are used in the backwards compatibility code...
 */
#define DATATYPE_VOID		0       /* !untyped data */
#define DATATYPE_INT		1       /* !signed integer data */
#define DATATYPE_UINT		2       /* !unsigned integer data */
#define DATATYPE_IEEEFP		3       /* !IEEE floating point data */

static void
setByteArray(void** vpp, void* vp, size_t nmemb, size_t elem_size)
{
	if (*vpp) {
		_TIFFfree(*vpp);
		*vpp = 0;
	}
	if (vp) {
		tmsize_t bytes = _TIFFMultiplySSize(NULL, nmemb, elem_size, NULL);
		if (bytes)
			*vpp = (void*) _TIFFmalloc(bytes);
		if (*vpp)
			_TIFFmemcpy(*vpp, vp, bytes);
	}
}
void _TIFFsetByteArray(void** vpp, void* vp, uint32_t n)
    { setByteArray(vpp, vp, n, 1); }
void _TIFFsetString(char** cpp, char* cp)
    { setByteArray((void**) cpp, (void*) cp, strlen(cp)+1, 1); }
static void _TIFFsetNString(char** cpp, char* cp, uint32_t n)
    { setByteArray((void**) cpp, (void*) cp, n, 1); }
void _TIFFsetShortArray(uint16_t** wpp, uint16_t* wp, uint32_t n)
    { setByteArray((void**) wpp, (void*) wp, n, sizeof (uint16_t)); }
void _TIFFsetLongArray(uint32_t** lpp, uint32_t* lp, uint32_t n)
    { setByteArray((void**) lpp, (void*) lp, n, sizeof (uint32_t)); }
static void _TIFFsetLong8Array(uint64_t** lpp, uint64_t* lp, uint32_t n)
    { setByteArray((void**) lpp, (void*) lp, n, sizeof (uint64_t)); }
void _TIFFsetFloatArray(float** fpp, float* fp, uint32_t n)
    { setByteArray((void**) fpp, (void*) fp, n, sizeof (float)); }
void _TIFFsetDoubleArray(double** dpp, double* dp, uint32_t n)
    { setByteArray((void**) dpp, (void*) dp, n, sizeof (double)); }
/* SetGetRATIONAL_directly: */
void _TIFFsetRationalArray(TIFFRational_t** dpp, TIFFRational_t* dp, uint32_t n)
{
	setByteArray((void**)dpp, (void*)dp, n, sizeof(TIFFRational_t));
}

static void
setDoubleArrayOneValue(double** vpp, double value, size_t nmemb)
{
	if (*vpp)
		_TIFFfree(*vpp);
	*vpp = _TIFFmalloc(nmemb*sizeof(double));
	if (*vpp)
	{
		while (nmemb--)
			((double*)*vpp)[nmemb] = value;
	}
}

static void
_TIFFsetFloatArrayFromRational(float** vpp, TIFFRational_t* vpR, uint32_t n, char isSigned, char fgAllocMemory)
{
	/* SetGetRATIONAL_directly:_CustomTag: 
	 * Return value needs a pointer to a float-array. Get memmory for float-array, if fgAllocMemory is true 
	 * 
	 * ATTENTION: For "SetGetRATIONAL_directly:_CustomTag:"-enhancement: Allocated extra memory for converting rational arrays to float/doble arrays has to be freed by libtiff. This is done by using td->value2.
	 */
	float* pf;
	uint32_t i;
	TIFFRational_t*  pR;
	TIFFSRational_t* psR;
	size_t elem_size = sizeof(float);
	/* check for input pointer */
	if (!vpR) {
		return;
	}
	if (fgAllocMemory) {
		tmsize_t bytes = _TIFFMultiplySSize(NULL, n, elem_size, NULL);
		if (bytes)
			*vpp = (void*)_TIFFmalloc(bytes);
		else
			*vpp = NULL;
	}
	if (*vpp) {
		pf = *vpp;
		/* Now fill array with correct values: */
		if (isSigned) {
			psR = (TIFFSRational_t*)vpR;
			for (i = 0; i < n; i++) {
				*pf = (float)((double)psR->sNum / (double)psR->sDenom);
				pf++;
				psR++;
			}
		}
		else {
			pR = vpR;
			for (i = 0; i < n; i++) {
				*pf = (float)((double)pR->uNum / (double)pR->uDenom);
				pf++;
				pR++;
			}
		}
	}
} /*-- _TIFFsetFloatArrayFromRational() --*/


static void
_TIFFsetDoubleArrayFromRational(double** vpp, TIFFRational_t* vpR, uint32_t n, char isSigned, char fgAllocMemory)
{
	/* SetGetRATIONAL_directly:_CustomTag:
	 * Return value needs a pointer to a float-array. Get memmory for float-array, if fgAllocMemory is true
	 *
	 * ATTENTION: For "SetGetRATIONAL_directly:_CustomTag:"-enhancement: Allocated extra memory for converting rational arrays to float/doble arrays has to be freed by libtiff. This is done by using td->value2.
	 */
	double* pf;
	uint32_t i;
	TIFFRational_t* pR;
	TIFFSRational_t* psR;
	size_t elem_size = sizeof(double);
	/* check for input pointer */
	if (!vpR) {
		return;
	}
	if (fgAllocMemory) {
		tmsize_t bytes = _TIFFMultiplySSize(NULL, n, elem_size, NULL);
		if (bytes)
			*vpp = (void*)_TIFFmalloc(bytes);
		else
			*vpp = NULL;
	}
	if (*vpp) {
		pf = *vpp;
		/* Now fill array with correct values: */
		if (isSigned) {
			psR = (TIFFSRational_t*)vpR;
			for (i = 0; i < n; i++) {
				*pf = ((double)psR->sNum / (double)psR->sDenom);
				pf++;
				psR++;
			}
		}
		else {
			pR = vpR;
			for (i = 0; i < n; i++) {
				*pf = ((double)pR->uNum / (double)pR->uDenom);
				pf++;
				pR++;
			}
		}
	}
} /*-- _TIFFsetDoubleArrayFromRational() --*/




/*
 * Install extra samples information.
 */
static int
setExtraSamples(TIFF* tif, va_list ap, uint32_t* v)
{
/* XXX: Unassociated alpha data == 999 is a known Corel Draw bug, see below */
#define EXTRASAMPLE_COREL_UNASSALPHA 999 

	uint16_t* va;
	uint32_t i;
        TIFFDirectory* td = &tif->tif_dir;
        static const char module[] = "setExtraSamples";

	*v = (uint16_t) va_arg(ap, uint16_vap);
	if ((uint16_t) *v > td->td_samplesperpixel)
		return 0;
	va = va_arg(ap, uint16_t*);
	if (*v > 0 && va == NULL)		/* typically missing param */
		return 0;
	for (i = 0; i < *v; i++) {
		if (va[i] > EXTRASAMPLE_UNASSALPHA) {
			/*
			 * XXX: Corel Draw is known to produce incorrect
			 * ExtraSamples tags which must be patched here if we
			 * want to be able to open some of the damaged TIFF
			 * files: 
			 */
			if (va[i] == EXTRASAMPLE_COREL_UNASSALPHA)
				va[i] = EXTRASAMPLE_UNASSALPHA;
			else
				return 0;
		}
	}

        if ( td->td_transferfunction[0] != NULL && (td->td_samplesperpixel - *v > 1) &&
                !(td->td_samplesperpixel - td->td_extrasamples > 1))
        {
                TIFFWarningExt(tif->tif_clientdata,module,
                    "ExtraSamples tag value is changing, "
                    "but TransferFunction was read with a different value. Canceling it");
                TIFFClrFieldBit(tif,FIELD_TRANSFERFUNCTION);
                _TIFFfree(td->td_transferfunction[0]);
                td->td_transferfunction[0] = NULL;
        }

	td->td_extrasamples = (uint16_t) *v;
	_TIFFsetShortArray(&td->td_sampleinfo, va, td->td_extrasamples);
	return 1;

#undef EXTRASAMPLE_COREL_UNASSALPHA
}

/*
 * Confirm we have "samplesperpixel" ink names separated by \0.  Returns 
 * zero if the ink names are not as expected.
 */
static uint32_t
checkInkNamesString(TIFF* tif, uint32_t slen, const char* s)
{
	TIFFDirectory* td = &tif->tif_dir;
	uint16_t i = td->td_samplesperpixel;

	if (slen > 0) {
		const char* ep = s+slen;
		const char* cp = s;
		for (; i > 0; i--) {
			for (; cp < ep && *cp != '\0'; cp++) {}
			if (cp >= ep)
				goto bad;
			cp++;				/* skip \0 */
		}
		return ((uint32_t)(cp - s));
	}
bad:
	TIFFErrorExt(tif->tif_clientdata, "TIFFSetField",
	    "%s: Invalid InkNames value; expecting %"PRIu16" names, found %"PRIu16,
	    tif->tif_name,
	    td->td_samplesperpixel,
	    (uint16_t)(td->td_samplesperpixel-i));
	return (0);
}

static int
_TIFFVSetField(TIFF* tif, uint32_t tag, va_list ap)
{
	static const char module[] = "_TIFFVSetField";

	TIFFDirectory* td = &tif->tif_dir;
	int status = 1;
	uint32_t v32, i, v;
    double dblval;
	char* s;
	const TIFFField *fip = TIFFFindField(tif, tag, TIFF_ANY);
	uint32_t standard_tag = tag;
	if( fip == NULL ) /* cannot happen since OkToChangeTag() already checks it */
	    return 0;
	/*
	 * We want to force the custom code to be used for custom
	 * fields even if the tag happens to match a well known 
	 * one - important for reinterpreted handling of standard
	 * tag values in custom directories (i.e. EXIF) 
	 */
	if (fip->field_bit == FIELD_CUSTOM) {
		standard_tag = 0;
	}

	switch (standard_tag) {
	case TIFFTAG_SUBFILETYPE:
		td->td_subfiletype = (uint32_t) va_arg(ap, uint32_t);
		break;
	case TIFFTAG_IMAGEWIDTH:
		td->td_imagewidth = (uint32_t) va_arg(ap, uint32_t);
		break;
	case TIFFTAG_IMAGELENGTH:
		td->td_imagelength = (uint32_t) va_arg(ap, uint32_t);
		break;
	case TIFFTAG_BITSPERSAMPLE:
		td->td_bitspersample = (uint16_t) va_arg(ap, uint16_vap);
		/*
		 * If the data require post-decoding processing to byte-swap
		 * samples, set it up here.  Note that since tags are required
		 * to be ordered, compression code can override this behavior
		 * in the setup method if it wants to roll the post decoding
		 * work in with its normal work.
		 */
		if (tif->tif_flags & TIFF_SWAB) {
			if (td->td_bitspersample == 8)
				tif->tif_postdecode = _TIFFNoPostDecode;
			else if (td->td_bitspersample == 16)
				tif->tif_postdecode = _TIFFSwab16BitData;
			else if (td->td_bitspersample == 24)
				tif->tif_postdecode = _TIFFSwab24BitData;
			else if (td->td_bitspersample == 32)
				tif->tif_postdecode = _TIFFSwab32BitData;
			else if (td->td_bitspersample == 64)
				tif->tif_postdecode = _TIFFSwab64BitData;
			else if (td->td_bitspersample == 128) /* two 64's */
				tif->tif_postdecode = _TIFFSwab64BitData;
		}
		break;
	case TIFFTAG_COMPRESSION:
		v = (uint16_t) va_arg(ap, uint16_vap);
		/*
		 * If we're changing the compression scheme, the notify the
		 * previous module so that it can cleanup any state it's
		 * setup.
		 */
		if (TIFFFieldSet(tif, FIELD_COMPRESSION)) {
			if ((uint32_t)td->td_compression == v)
				break;
			(*tif->tif_cleanup)(tif);
			tif->tif_flags &= ~TIFF_CODERSETUP;
		}
		/*
		 * Setup new compression routine state.
		 */
		if( (status = TIFFSetCompressionScheme(tif, v)) != 0 )
		    td->td_compression = (uint16_t) v;
		else
		    status = 0;
		break;
	case TIFFTAG_PHOTOMETRIC:
		td->td_photometric = (uint16_t) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_THRESHHOLDING:
		td->td_threshholding = (uint16_t) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_FILLORDER:
		v = (uint16_t) va_arg(ap, uint16_vap);
		if (v != FILLORDER_LSB2MSB && v != FILLORDER_MSB2LSB)
			goto badvalue;
		td->td_fillorder = (uint16_t) v;
		break;
	case TIFFTAG_ORIENTATION:
		v = (uint16_t) va_arg(ap, uint16_vap);
		if (v < ORIENTATION_TOPLEFT || ORIENTATION_LEFTBOT < v)
			goto badvalue;
		else
			td->td_orientation = (uint16_t) v;
		break;
	case TIFFTAG_SAMPLESPERPIXEL:
		v = (uint16_t) va_arg(ap, uint16_vap);
		if (v == 0)
			goto badvalue;
        if( v != td->td_samplesperpixel )
        {
            /* See http://bugzilla.maptools.org/show_bug.cgi?id=2500 */
            if( td->td_sminsamplevalue != NULL )
            {
                TIFFWarningExt(tif->tif_clientdata,module,
                    "SamplesPerPixel tag value is changing, "
                    "but SMinSampleValue tag was read with a different value. Canceling it");
                TIFFClrFieldBit(tif,FIELD_SMINSAMPLEVALUE);
                _TIFFfree(td->td_sminsamplevalue);
                td->td_sminsamplevalue = NULL;
            }
            if( td->td_smaxsamplevalue != NULL )
            {
                TIFFWarningExt(tif->tif_clientdata,module,
                    "SamplesPerPixel tag value is changing, "
                    "but SMaxSampleValue tag was read with a different value. Canceling it");
                TIFFClrFieldBit(tif,FIELD_SMAXSAMPLEVALUE);
                _TIFFfree(td->td_smaxsamplevalue);
                td->td_smaxsamplevalue = NULL;
            }
            /* Test if 3 transfer functions instead of just one are now needed
               See http://bugzilla.maptools.org/show_bug.cgi?id=2820 */
            if( td->td_transferfunction[0] != NULL && (v - td->td_extrasamples > 1) &&
                !(td->td_samplesperpixel - td->td_extrasamples > 1))
            {
                    TIFFWarningExt(tif->tif_clientdata,module,
                        "SamplesPerPixel tag value is changing, "
                        "but TransferFunction was read with a different value. Canceling it");
                    TIFFClrFieldBit(tif,FIELD_TRANSFERFUNCTION);
                    _TIFFfree(td->td_transferfunction[0]);
                    td->td_transferfunction[0] = NULL;
            }
        }
		td->td_samplesperpixel = (uint16_t) v;
		break;
	case TIFFTAG_ROWSPERSTRIP:
		v32 = (uint32_t) va_arg(ap, uint32_t);
		if (v32 == 0)
			goto badvalue32;
		td->td_rowsperstrip = v32;
		if (!TIFFFieldSet(tif, FIELD_TILEDIMENSIONS)) {
			td->td_tilelength = v32;
			td->td_tilewidth = td->td_imagewidth;
		}
		break;
	case TIFFTAG_MINSAMPLEVALUE:
		td->td_minsamplevalue = (uint16_t) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_MAXSAMPLEVALUE:
		td->td_maxsamplevalue = (uint16_t) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_SMINSAMPLEVALUE:
		if (tif->tif_flags & TIFF_PERSAMPLE)
			_TIFFsetDoubleArray(&td->td_sminsamplevalue, va_arg(ap, double*), td->td_samplesperpixel);
		else
			setDoubleArrayOneValue(&td->td_sminsamplevalue, va_arg(ap, double), td->td_samplesperpixel);
		break;
	case TIFFTAG_SMAXSAMPLEVALUE:
		if (tif->tif_flags & TIFF_PERSAMPLE)
			_TIFFsetDoubleArray(&td->td_smaxsamplevalue, va_arg(ap, double*), td->td_samplesperpixel);
		else
			setDoubleArrayOneValue(&td->td_smaxsamplevalue, va_arg(ap, double), td->td_samplesperpixel);
		break;
	case TIFFTAG_XRESOLUTION:
        dblval = va_arg(ap, double);
        if( dblval < 0 )
            goto badvaluedouble;
		/* SetGetRATIONAL_directly: */
		DoubleToRational(dblval, &td->td_xresolution.uNum, &td->td_xresolution.uDenom);
		break;
	case TIFFTAG_YRESOLUTION:
        dblval = va_arg(ap, double);
        if( dblval < 0 )
            goto badvaluedouble;
		/* SetGetRATIONAL_directly: */
		DoubleToRational(dblval, &td->td_yresolution.uNum, &td->td_yresolution.uDenom);
		break;
	case TIFFTAG_PLANARCONFIG:
		v = (uint16_t) va_arg(ap, uint16_vap);
		if (v != PLANARCONFIG_CONTIG && v != PLANARCONFIG_SEPARATE)
			goto badvalue;
		td->td_planarconfig = (uint16_t) v;
		break;
	case TIFFTAG_XPOSITION:
		/* SetGetRATIONAL_directly: */
		DoubleToRational(va_arg(ap, double), &td->td_xposition.uNum, &td->td_xposition.uDenom);
		break;
	case TIFFTAG_YPOSITION:
		/* SetGetRATIONAL_directly: */
		DoubleToRational(va_arg(ap, double), &td->td_yposition.uNum, &td->td_yposition.uDenom);
		break;
	case TIFFTAG_RESOLUTIONUNIT:
		v = (uint16_t) va_arg(ap, uint16_vap);
		if (v < RESUNIT_NONE || RESUNIT_CENTIMETER < v)
			goto badvalue;
		td->td_resolutionunit = (uint16_t) v;
		break;
	case TIFFTAG_PAGENUMBER:
		td->td_pagenumber[0] = (uint16_t) va_arg(ap, uint16_vap);
		td->td_pagenumber[1] = (uint16_t) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_HALFTONEHINTS:
		td->td_halftonehints[0] = (uint16_t) va_arg(ap, uint16_vap);
		td->td_halftonehints[1] = (uint16_t) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_COLORMAP:
		v32 = (uint32_t)(1L << td->td_bitspersample);
		_TIFFsetShortArray(&td->td_colormap[0], va_arg(ap, uint16_t*), v32);
		_TIFFsetShortArray(&td->td_colormap[1], va_arg(ap, uint16_t*), v32);
		_TIFFsetShortArray(&td->td_colormap[2], va_arg(ap, uint16_t*), v32);
		break;
	case TIFFTAG_EXTRASAMPLES:
		if (!setExtraSamples(tif, ap, &v))
			goto badvalue;
		break;
	case TIFFTAG_MATTEING:
		td->td_extrasamples =  (((uint16_t) va_arg(ap, uint16_vap)) != 0);
		if (td->td_extrasamples) {
			uint16_t sv = EXTRASAMPLE_ASSOCALPHA;
			_TIFFsetShortArray(&td->td_sampleinfo, &sv, 1);
		}
		break;
	case TIFFTAG_TILEWIDTH:
		v32 = (uint32_t) va_arg(ap, uint32_t);
		if (v32 % 16) {
			if (tif->tif_mode != O_RDONLY)
				goto badvalue32;
			TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
				"Nonstandard tile width %"PRIu32", convert file", v32);
		}
		td->td_tilewidth = v32;
		tif->tif_flags |= TIFF_ISTILED;
		break;
	case TIFFTAG_TILELENGTH:
		v32 = (uint32_t) va_arg(ap, uint32_t);
		if (v32 % 16) {
			if (tif->tif_mode != O_RDONLY)
				goto badvalue32;
			TIFFWarningExt(tif->tif_clientdata, tif->tif_name,
			    "Nonstandard tile length %"PRIu32", convert file", v32);
		}
		td->td_tilelength = v32;
		tif->tif_flags |= TIFF_ISTILED;
		break;
	case TIFFTAG_TILEDEPTH:
		v32 = (uint32_t) va_arg(ap, uint32_t);
		if (v32 == 0)
			goto badvalue32;
		td->td_tiledepth = v32;
		break;
	case TIFFTAG_DATATYPE:
		v = (uint16_t) va_arg(ap, uint16_vap);
		switch (v) {
		case DATATYPE_VOID:	v = SAMPLEFORMAT_VOID;	break;
		case DATATYPE_INT:	v = SAMPLEFORMAT_INT;	break;
		case DATATYPE_UINT:	v = SAMPLEFORMAT_UINT;	break;
		case DATATYPE_IEEEFP:	v = SAMPLEFORMAT_IEEEFP;break;
		default:		goto badvalue;
		}
		td->td_sampleformat = (uint16_t) v;
		break;
	case TIFFTAG_SAMPLEFORMAT:
		v = (uint16_t) va_arg(ap, uint16_vap);
		if (v < SAMPLEFORMAT_UINT || SAMPLEFORMAT_COMPLEXIEEEFP < v)
			goto badvalue;
		td->td_sampleformat = (uint16_t) v;

		/*  Try to fix up the SWAB function for complex data. */
		if( td->td_sampleformat == SAMPLEFORMAT_COMPLEXINT
		    && td->td_bitspersample == 32
		    && tif->tif_postdecode == _TIFFSwab32BitData )
		    tif->tif_postdecode = _TIFFSwab16BitData;
		else if( (td->td_sampleformat == SAMPLEFORMAT_COMPLEXINT
			  || td->td_sampleformat == SAMPLEFORMAT_COMPLEXIEEEFP)
			 && td->td_bitspersample == 64
			 && tif->tif_postdecode == _TIFFSwab64BitData )
		    tif->tif_postdecode = _TIFFSwab32BitData;
		break;
	case TIFFTAG_IMAGEDEPTH:
		td->td_imagedepth = (uint32_t) va_arg(ap, uint32_t);
		break;
	case TIFFTAG_SUBIFD:
		if ((tif->tif_flags & TIFF_INSUBIFD) == 0) {
			td->td_nsubifd = (uint16_t) va_arg(ap, uint16_vap);
			_TIFFsetLong8Array(&td->td_subifd, (uint64_t*) va_arg(ap, uint64_t*),
			    (uint32_t) td->td_nsubifd);
		} else {
			TIFFErrorExt(tif->tif_clientdata, module,
				     "%s: Sorry, cannot nest SubIFDs",
				     tif->tif_name);
			status = 0;
		}
		break;
	case TIFFTAG_YCBCRPOSITIONING:
		td->td_ycbcrpositioning = (uint16_t) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_YCBCRSUBSAMPLING:
		td->td_ycbcrsubsampling[0] = (uint16_t) va_arg(ap, uint16_vap);
		td->td_ycbcrsubsampling[1] = (uint16_t) va_arg(ap, uint16_vap);
		break;
	case TIFFTAG_TRANSFERFUNCTION:
		v = (td->td_samplesperpixel - td->td_extrasamples) > 1 ? 3 : 1;
		for (i = 0; i < v; i++)
			_TIFFsetShortArray(&td->td_transferfunction[i],
                               va_arg(ap, uint16_t*), 1U << td->td_bitspersample);
		break;
	case TIFFTAG_REFERENCEBLACKWHITE:
		/* XXX should check for null range */
		{
			/* SetGetRATIONAL_directly: */
			float* pf;
			TIFFRational_t* pR;
			pf = va_arg(ap, float*);
			_TIFFsetRationalArray(&td->td_refblackwhite, (TIFFRational_t*)&pf, 6);
			/* Now fill array with correct values: */
			pR = td->td_refblackwhite;
			if (pR != NULL) {
				for (int i = 0; i < 6; i++) {
					DoubleToRational(*pf, &pR->uNum, &pR->uDenom);
					pf++;
					pR++;
				}
			}
		}
		break;
	case TIFFTAG_INKNAMES:
		v = (uint16_t) va_arg(ap, uint16_vap);
		s = va_arg(ap, char*);
		v = checkInkNamesString(tif, v, s);
		status = v > 0;
		if( v > 0 ) {
			_TIFFsetNString(&td->td_inknames, s, v);
			td->td_inknameslen = v;
		}
		break;
	case TIFFTAG_PERSAMPLE:
		v = (uint16_t) va_arg(ap, uint16_vap);
		if( v == PERSAMPLE_MULTI )
			tif->tif_flags |= TIFF_PERSAMPLE;
		else
			tif->tif_flags &= ~TIFF_PERSAMPLE;
		break;
	default: {
		TIFFTagValue *tv;
		int tv_size, iCustom;

		/*
		 * This can happen if multiple images are open with different
		 * codecs which have private tags.  The global tag information
		 * table may then have tags that are valid for one file but not
		 * the other. If the client tries to set a tag that is not valid
		 * for the image's codec then we'll arrive here.  This
		 * happens, for example, when tiffcp is used to convert between
		 * compression schemes and codec-specific tags are blindly copied.
		 */
		if(fip->field_bit != FIELD_CUSTOM) {
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Invalid %stag \"%s\" (not supported by codec)",
			    tif->tif_name, isPseudoTag(tag) ? "pseudo-" : "",
			    fip->field_name);
			status = 0;
			break;
		}

		/*
		 * Find the existing entry for this custom value.
		 */
		tv = NULL;
		for (iCustom = 0; iCustom < td->td_customValueCount; iCustom++) {
			if (td->td_customValues[iCustom].info->field_tag == tag) {
				tv = td->td_customValues + iCustom;
				if (tv->value != NULL) {
					_TIFFfree(tv->value);
					tv->value = NULL;
				}
				/*--: SetGetRATIONAL_directly:_CustomTag: */
				if (tv->value2 != NULL) {
					_TIFFfree(tv->value2);
					tv->value2 = NULL;
				}
				break;
			}
		}

		/*
		 * Grow the custom list if the entry was not found.
		 */
		if(tv == NULL) {
			TIFFTagValue *new_customValues;

			td->td_customValueCount++;
			new_customValues = (TIFFTagValue *)
			    _TIFFrealloc(td->td_customValues,
			    sizeof(TIFFTagValue) * td->td_customValueCount);
			if (!new_customValues) {
				TIFFErrorExt(tif->tif_clientdata, module,
				    "%s: Failed to allocate space for list of custom values",
				    tif->tif_name);
				status = 0;
				goto end;
			}

			td->td_customValues = new_customValues;

			tv = td->td_customValues + (td->td_customValueCount - 1);
			tv->info = fip;
			tv->value = NULL;
			tv->value2 = NULL; /*--: SetGetRATIONAL_directly:_CustomTag: */
			tv->count = 0;
		}

		/*
		 * Set custom value ... save a copy of the custom tag value.
		 */
		/*--: Rational2Double: For Rationals evaluate "set_field_type" to determine internal storage size. */
		//tv_size = _TIFFDataSize(fip->field_type);
		/*--: SetGetRATIONAL_directly:_CustomTag:
		*   Because the internal storage size of RATIONAL is now the same size than the size of the tag,
		*   the size can be retrieved by TIFFDataWidth() and the additional function _TIFFDataSize() can be deleted.
		*   ATTENTION: tv_size holds the size of the internal storage size but NOT the size of the passed API data parameter!
		*/
		tv_size = TIFFDataWidth(fip->field_type);
		if (tv_size == 0) {
			status = 0;
			TIFFErrorExt(tif->tif_clientdata, module,
			    "%s: Bad field type %d for \"%s\"",
			    tif->tif_name, fip->field_type,
			    fip->field_name);
			goto end;
		}

		if (fip->field_type == TIFF_ASCII)
		{
			uint32_t ma;
			char* mb;
			if (fip->field_passcount)
			{
				assert(fip->field_writecount==TIFF_VARIABLE2);
				ma=(uint32_t)va_arg(ap, uint32_t);
				mb=(char*)va_arg(ap,char*);
			}
			else
			{
				mb=(char*)va_arg(ap,char*);
				ma=(uint32_t)(strlen(mb) + 1);
			}
			tv->count=ma;
			setByteArray(&tv->value,mb,ma,1);
		}
		else
		{
			if (fip->field_passcount) {
				if (fip->field_writecount == TIFF_VARIABLE2)
					tv->count = (uint32_t) va_arg(ap, uint32_t);
				else
					tv->count = (int) va_arg(ap, int);
			} else if (fip->field_writecount == TIFF_VARIABLE
			   || fip->field_writecount == TIFF_VARIABLE2)
				tv->count = 1;
			else if (fip->field_writecount == TIFF_SPP)
				tv->count = td->td_samplesperpixel;
			else
				tv->count = fip->field_writecount;

			if (tv->count == 0) {
				status = 0;
				TIFFErrorExt(tif->tif_clientdata, module,
					     "%s: Null count for \"%s\" (type "
					     "%d, writecount %d, passcount %d)",
					     tif->tif_name,
					     fip->field_name,
					     fip->field_type,
					     fip->field_writecount,
					     fip->field_passcount);
				goto end;
			}

			tv->value = _TIFFCheckMalloc(tif, tv->count, tv_size,
			    "custom tag binary object");
			if (!tv->value) {
				status = 0;
				goto end;
			}

			if (fip->field_tag == TIFFTAG_DOTRANGE 
			    && strcmp(fip->field_name,"DotRange") == 0) {
				/* TODO: This is an evil exception and should not have been
				   handled this way ... likely best if we move it into
				   the directory structure with an explicit field in 
				   libtiff 4.1 and assign it a FIELD_ value */
				uint16_t v2[2];
				v2[0] = (uint16_t)va_arg(ap, int);
				v2[1] = (uint16_t)va_arg(ap, int);
				_TIFFmemcpy(tv->value, &v2, 4);
			}

			else if (fip->field_passcount
				  || fip->field_writecount == TIFF_VARIABLE
				  || fip->field_writecount == TIFF_VARIABLE2
				  || fip->field_writecount == TIFF_SPP
				  || tv->count > 1) {
				/*--: Rational2Double: For Rationals tv_size is set above to 4 or 8 according to fip->set_field_type! */
				/*--: SetGetRATIONAL_directly:_CustomTag:
				*   ATTENTION: tv_size holds the size of the internal storage size but NOT the size of the passed API data parameter!
				*              Therefore, that API parameter size has to be evaluated using fip->set_field_type information.
				*   For RATIONAL arrays, a special treatment is needed.
				*/
				if (fip->field_type != TIFF_RATIONAL && fip->field_type != TIFF_SRATIONAL) {
					_TIFFmemcpy(tv->value, va_arg(ap, void *), tv->count * tv_size);
				}
				else {
					/* Float or Double Array input for RATIONAL storage */
					int i;
					int tv_set_get_size = _TIFFSetGetFieldSize(fip->set_field_type);
					if (tv_set_get_size == 4) {
						float* pf;
						pf = va_arg(ap, float*);
						if (fip->field_type == TIFF_RATIONAL) {
							TIFFRational_t* pR;
							/* Now fill array with correct values: */
							pR = tv->value;
							for (i = 0; i < tv->count; i++) {
								DoubleToRational(*pf, &pR->uNum, &pR->uDenom);
								pf++;
								pR++;
							}
						}
						else {
							TIFFSRational_t* psR;
							/* Now fill array with correct values: */
							psR = tv->value;
							for (i = 0; i < tv->count; i++) {
								DoubleToSrational(*pf, &psR->sNum, &psR->sDenom);
								pf++;
								psR++;
							}
						}
					} /*-- if (tv_set_get_size == 4)  --*/
					else if (tv_set_get_size == 8) {
						double* pd;
						pd = va_arg(ap, double*);
						if (fip->field_type == TIFF_RATIONAL) {
							TIFFRational_t* pR;
							/* Now fill array with correct values: */
							pR = tv->value;
							for (i = 0; i < tv->count; i++) {
								DoubleToRational(*pd, &pR->uNum, &pR->uDenom);
								pd++;
								pR++;
							}
						}
						else {
							TIFFSRational_t* psR;
							/* Now fill array with correct values: */
							psR = tv->value;
							for (i = 0; i < tv->count; i++) {
								DoubleToSrational(*pd, &psR->sNum, &psR->sDenom);
								pd++;
								psR++;
							}
						}
					} /*-- if (tv_set_get_size == 8)  --*/
					else {
						TIFFErrorExt(tif->tif_clientdata, module,
							"%s: Input parameter size %d not expected for RATIONAL",
							tif->tif_name, tv_set_get_size);
					}
				} /*-- if (fip->field_type != TIFF_RATIONAL && fip->field_type != TIFF_SRATIONAL)  --*/
			} else {
				char *val = (char *)tv->value;
				assert( tv->count == 1 );

				switch (fip->field_type) {
				case TIFF_BYTE:
				case TIFF_UNDEFINED:
					{
						uint8_t v2 = (uint8_t)va_arg(ap, int);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SBYTE:
					{
						int8_t v2 = (int8_t)va_arg(ap, int);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SHORT:
					{
						uint16_t v2 = (uint16_t)va_arg(ap, int);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SSHORT:
					{
						int16_t v2 = (int16_t)va_arg(ap, int);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_LONG:
				case TIFF_IFD:
					{
						uint32_t v2 = va_arg(ap, uint32_t);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SLONG:
					{
						int32_t v2 = va_arg(ap, int32_t);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_LONG8:
				case TIFF_IFD8:
					{
						uint64_t v2 = va_arg(ap, uint64_t);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_SLONG8:
					{
						int64_t v2 = va_arg(ap, int64_t);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_RATIONAL:
					/*-- Rational2Double: For Rationals tv_size is set above to 4 or 8 according to fip->set_field_type! */
					{
						/*-- SetGetRATIONAL_directly:_CustomTag:  Here, tv_size should be always =8.
						*    The interface is allways double and the internal storage is allways 2 * uint32_t;
						*    However, we have to distinguish between unsigned and signed rationals.
						--*/
						TIFFRational_t* r;
						double v2;
						assert(tv_size == 8);
						r = (TIFFRational_t*)val;
						v2 = va_arg(ap, double);
						DoubleToRational(v2, &r->uNum, &r->uDenom);
					}
					break;
				case TIFF_SRATIONAL:
					{
						/*-- SetGetRATIONAL_directly:_CustomTag:  Here, tv_size should be always =8.
						*    The interface is allways double and the internal storage is allways 2 * uint32_t;
						*    However, we have to distinguish between unsigned and signed rationals.
						--*/
						TIFFSRational_t* r;
						double v2;
						assert(tv_size == 8);
						r = (TIFFSRational_t*)val;
						v2 = va_arg(ap, double);
						DoubleToSrational(v2, &r->sNum, &r->sDenom);
					}
					break;
				case TIFF_FLOAT:
					{
						float v2 = _TIFFClampDoubleToFloat(va_arg(ap, double));
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				case TIFF_DOUBLE:
					{
						double v2 = va_arg(ap, double);
						_TIFFmemcpy(val, &v2, tv_size);
					}
					break;
				default:
					_TIFFmemset(val, 0, tv_size);
					status = 0;
					break;
				}
			}
		}
	} /* default: of switch(standard_tag) */
	} /* switch(standard_tag) */
	if (status) {
		const TIFFField* fip2=TIFFFieldWithTag(tif,tag);
		if (fip2)                
			TIFFSetFieldBit(tif, fip2->field_bit);
		tif->tif_flags |= TIFF_DIRTYDIRECT;
	}

end:
	va_end(ap);
	return (status);
badvalue:
        {
		const TIFFField* fip2=TIFFFieldWithTag(tif,tag);
		TIFFErrorExt(tif->tif_clientdata, module,
		     "%s: Bad value %"PRIu32" for \"%s\" tag",
		     tif->tif_name, v,
		     fip2 ? fip2->field_name : "Unknown");
		va_end(ap);
        }
	return (0);
badvalue32:
        {
		const TIFFField* fip2=TIFFFieldWithTag(tif,tag);
		TIFFErrorExt(tif->tif_clientdata, module,
		     "%s: Bad value %"PRIu32" for \"%s\" tag",
		     tif->tif_name, v32,
		     fip2 ? fip2->field_name : "Unknown");
		va_end(ap);
        }
	return (0);
badvaluedouble:
        {
        const TIFFField* fip2=TIFFFieldWithTag(tif,tag);
        TIFFErrorExt(tif->tif_clientdata, module,
             "%s: Bad value %f for \"%s\" tag",
             tif->tif_name, dblval,
             fip2 ? fip2->field_name : "Unknown");
        va_end(ap);
        }
    return (0);
} /* _TIFFVSetField() */


static int
_TIFFVSetFieldRational(TIFF* tif, uint32_t tag, va_list ap)
{
	/* SetGetRATIONAL_directly: 
	 * _TIFFVSetFieldRational() is necessary to distinguish the va_list parameters for directly setting rational values
	 * from the standard case, where a float value is provided.
	 * 
	 * This routine expects as va_list: Numerator , Denominator as uint_32 (or int_32 for signed rationals).
	 */
	static const char module[] = "_TIFFVSetFieldRational";

	TIFFDirectory* td = &tif->tif_dir;
	int status = 1;
	TIFFRational_t* uR;

	const TIFFField* fip = TIFFFindField(tif, tag, TIFF_ANY);
	uint32_t standard_tag = tag;
	if (fip == NULL) /* cannot happen since OkToChangeTag() already checks it */
		return 0;
	
	/* Check for error if it is not a RATIONAL-tag. */
	if (fip->field_type != TIFF_RATIONAL && fip->field_type != TIFF_SRATIONAL) {
		TIFFErrorExt(tif->tif_clientdata, module, "This function accepts only rational tif-tag!");
		return 0;
	}
	
	/*
	 * We want to force the custom code to be used for custom
	 * fields even if the tag happens to match a well known
	 * one - important for reinterpreted handling of standard
	 * tag values in custom directories (i.e. EXIF)
	 */
	if (fip->field_bit == FIELD_CUSTOM) {
		standard_tag = 0;
	}

	switch (standard_tag) {
	/* hereafter, only rational tags are handled */
		case TIFFTAG_XRESOLUTION:
			td->td_xresolution.uNum = va_arg(ap, uint32_t);
			td->td_xresolution.uDenom = va_arg(ap, uint32_t);
			break;
		case TIFFTAG_YRESOLUTION:
			td->td_yresolution.uNum = va_arg(ap, uint32_t);
			td->td_yresolution.uDenom = va_arg(ap, uint32_t);
			break;
		case TIFFTAG_XPOSITION:
			td->td_xposition.uNum = va_arg(ap, uint32_t);
			td->td_xposition.uDenom = va_arg(ap, uint32_t);
			break;
		case TIFFTAG_YPOSITION:
			td->td_yposition.uNum = va_arg(ap, uint32_t);
			td->td_yposition.uDenom = va_arg(ap, uint32_t);
			break;
		case TIFFTAG_REFERENCEBLACKWHITE:
			/* a rational array with 6 entries is expected */
			_TIFFsetRationalArray(&td->td_refblackwhite, va_arg(ap,TIFFRational_t*), 6);
		break;

		default: {
			TIFFTagValue* tv;
			int tv_size, iCustom;

			/*
			 * This can happen if multiple images are open with different
			 * codecs which have private tags.  The global tag information
			 * table may then have tags that are valid for one file but not
			 * the other. If the client tries to set a tag that is not valid
			 * for the image's codec then we'll arrive here.  This
			 * happens, for example, when tiffcp is used to convert between
			 * compression schemes and codec-specific tags are blindly copied.
			 */
			if (fip->field_bit != FIELD_CUSTOM) {
				TIFFErrorExt(tif->tif_clientdata, module,
					"%s: Invalid %stag \"%s\" (not supported by codec)",
					tif->tif_name, isPseudoTag(tag) ? "pseudo-" : "",
					fip->field_name);
				status = 0;
				break;
			}

			/*
			 * Find the existing entry for this custom value.
			 */
			tv = NULL;
			for (iCustom = 0; iCustom < td->td_customValueCount; iCustom++) {
				if (td->td_customValues[iCustom].info->field_tag == tag) {
					tv = td->td_customValues + iCustom;
					if (tv->value != NULL) {
						_TIFFfree(tv->value);
						tv->value = NULL;
					}
					/*--: SetGetRATIONAL_directly:_CustomTag: */
					if (tv->value2 != NULL) {
						_TIFFfree(tv->value2);
						tv->value2 = NULL;
					}
					break;
				}
			}

			/*
			 * Grow the custom list if the entry was not found.
			 */
			if (tv == NULL) {
				TIFFTagValue* new_customValues;

				td->td_customValueCount++;
				new_customValues = (TIFFTagValue*)
					_TIFFrealloc(td->td_customValues,
						sizeof(TIFFTagValue) * td->td_customValueCount);
				if (!new_customValues) {
					TIFFErrorExt(tif->tif_clientdata, module,
						"%s: Failed to allocate space for list of custom values",
						tif->tif_name);
					status = 0;
					goto end;
				}

				td->td_customValues = new_customValues;

				tv = td->td_customValues + (td->td_customValueCount - 1);
				tv->info = fip;
				tv->value = NULL;
				tv->value2 = NULL; /*--: SetGetRATIONAL_directly:_CustomTag: */
				tv->count = 0;
			}

			/*
			 * SetGetRATIONAL_directly:_CustomTag: 
			 * -- Here, only a unsigned or signed rational is expected for a FIELD_CUSTOM --
			 * Therefore, tv_size is allways =8
			*/
			tv_size = 8;

			/*-- Check for rationals and quit if its not. --*/
			if (fip->field_type != TIFF_RATIONAL && fip->field_type != TIFF_SRATIONAL) {
				TIFFErrorExt(tif->tif_clientdata, module,
					"%s: Custom tag field type is not RATIONAL.",
					tif->tif_name);
				status = 0;
				goto end;
			}

			/*
			 * Set custom value ... save a copy of the custom tag value.
			*/
			if (fip->field_passcount) {
				if (fip->field_writecount == TIFF_VARIABLE2)
					tv->count = (uint32_t)va_arg(ap, uint32_t);
				else
					tv->count = (int)va_arg(ap, int);
			}
			else if (fip->field_writecount == TIFF_VARIABLE	|| fip->field_writecount == TIFF_VARIABLE2) {
				tv->count = 1;
			}
			else if (fip->field_writecount == TIFF_SPP) {
				tv->count = td->td_samplesperpixel;
			}
			else {
				tv->count = fip->field_writecount;
			}

			if (tv->count == 0) {
				status = 0;
				TIFFErrorExt(tif->tif_clientdata, module,
					"%s: Null count for \"%s\" (type "
					"%d, writecount %d, passcount %d)",
					tif->tif_name,
					fip->field_name,
					fip->field_type,
					fip->field_writecount,
					fip->field_passcount);
				goto end;
			}

			tv->value = _TIFFCheckMalloc(tif, tv->count, tv_size,
				"custom tag binary object");
			if (!tv->value) {
				status = 0;
				goto end;
			}

			if (fip->field_passcount
				|| fip->field_writecount == TIFF_VARIABLE
				|| fip->field_writecount == TIFF_VARIABLE2
				|| fip->field_writecount == TIFF_SPP
				|| tv->count > 1) {
				/*--: copy rational array back to calling function --*/
				_TIFFmemcpy(tv->value, va_arg(ap, void*), tv->count * tv_size);
			}
			else {
				char* val = (char*)tv->value;
				assert(tv->count == 1);

				/* Single rational value. Not necessary here to distinguish between signed and unsigned. */
				switch (fip->field_type) {
					case TIFF_RATIONAL:
					case TIFF_SRATIONAL:
						uR = (TIFFRational_t*)val;
						uR->uNum = va_arg(ap, uint32_t);
						uR->uDenom = va_arg(ap, uint32_t);
						break;
					default:
						_TIFFmemset(val, 0, tv_size);
						status = 0;
						break;
				} /* switch(fip->fieldtype */
			} /* else for single rational value */
		} /* default: of switch(standard_tag) */
	} /*-- switch(standard_tag) --*/

	if (status) {
		const TIFFField* fip2 = TIFFFieldWithTag(tif, tag);
		if (fip2)
			TIFFSetFieldBit(tif, fip2->field_bit);
		tif->tif_flags |= TIFF_DIRTYDIRECT;
	}

end:
	va_end(ap);
	return (status);
} /* _TIFFVSetFieldRational() */




/*
 * Return 1/0 according to whether or not
 * it is permissible to set the tag's value.
 * Note that we allow ImageLength to be changed
 * so that we can append and extend to images.
 * Any other tag may not be altered once writing
 * has commenced, unless its value has no effect
 * on the format of the data that is written.
 */
static int
OkToChangeTag(TIFF* tif, uint32_t tag)
{
	const TIFFField* fip = TIFFFindField(tif, tag, TIFF_ANY);
	if (!fip) {			/* unknown tag */
		TIFFErrorExt(tif->tif_clientdata, "TIFFSetField", "%s: Unknown %stag %"PRIu32,
		    tif->tif_name, isPseudoTag(tag) ? "pseudo-" : "", tag);
		return (0);
	}
	if (tag != TIFFTAG_IMAGELENGTH && (tif->tif_flags & TIFF_BEENWRITING) &&
	    !fip->field_oktochange) {
		/*
		 * Consult info table to see if tag can be changed
		 * after we've started writing.  We only allow changes
		 * to those tags that don't/shouldn't affect the
		 * compression and/or format of the data.
		 */
		TIFFErrorExt(tif->tif_clientdata, "TIFFSetField",
		    "%s: Cannot modify tag \"%s\" while writing",
		    tif->tif_name, fip->field_name);
		return (0);
	}
	return (1);
}

/*
 * Record the value of a field in the
 * internal directory structure.  The
 * field will be written to the file
 * when/if the directory structure is
 * updated.
 */
int
TIFFSetField(TIFF* tif, uint32_t tag, ...)
{
	va_list ap;
	int status;

	va_start(ap, tag);
	status = TIFFVSetField(tif, tag, ap);
	va_end(ap);
	return (status);
}

int
TIFFSetFieldRational(TIFF* tif, uint32_t tag, ...)
{   /* SetGetRATIONAL_directly: */
	va_list ap;
	int status;

	va_start(ap, tag);
	status = TIFFVSetFieldRational(tif, tag, ap);
	va_end(ap);
	return (status);
} /* TIFFSetFieldRational() */

/*
 * Clear the contents of the field in the internal structure.
 */
int
TIFFUnsetField(TIFF* tif, uint32_t tag)
{
    const TIFFField *fip =  TIFFFieldWithTag(tif, tag);
    TIFFDirectory* td = &tif->tif_dir;

    if( !fip )
        return 0;

    if( fip->field_bit != FIELD_CUSTOM )
        TIFFClrFieldBit(tif, fip->field_bit);
    else
    {
        TIFFTagValue *tv = NULL;
        int i;

        for (i = 0; i < td->td_customValueCount; i++) {
                
            tv = td->td_customValues + i;
            if( tv->info->field_tag == tag )
                break;
        }

        if( i < td->td_customValueCount )
        {
            _TIFFfree(tv->value);
			_TIFFfree(tv->value2); /*--: SetGetRATIONAL_directly:_CustomTag: */
			for( ; i < td->td_customValueCount-1; i++) {
                td->td_customValues[i] = td->td_customValues[i+1];
            }
            td->td_customValueCount--;
        }
    }
        
    tif->tif_flags |= TIFF_DIRTYDIRECT;

    return (1);
}

/*
 * Like TIFFSetField, but taking a varargs
 * parameter list.  This routine is useful
 * for building higher-level interfaces on
 * top of the library.
 */
int
TIFFVSetField(TIFF* tif, uint32_t tag, va_list ap)
{
	return OkToChangeTag(tif, tag) ?
	    (*tif->tif_tagmethods.vsetfield)(tif, tag, ap) : 0;
}

int
TIFFVSetFieldRational(TIFF* tif, uint32_t tag, va_list ap)
{   /* SetGetRATIONAL_directly: */
	return OkToChangeTag(tif, tag) ?
		(*tif->tif_tagmethods.vsetfieldrational)(tif, tag, ap) : 0;
}

static int
_TIFFVGetField(TIFF* tif, uint32_t tag, va_list ap)
{
	TIFFDirectory* td = &tif->tif_dir;
	int ret_val = 1;
	uint32_t standard_tag = tag;
	const TIFFField* fip = TIFFFindField(tif, tag, TIFF_ANY);
	if( fip == NULL ) /* cannot happen since TIFFGetField() already checks it */
	    return 0;

	/*
	 * We want to force the custom code to be used for custom
	 * fields even if the tag happens to match a well known 
	 * one - important for reinterpreted handling of standard
	 * tag values in custom directories (i.e. EXIF) 
	 */
	if (fip->field_bit == FIELD_CUSTOM) {
		standard_tag = 0;
	}
	
        if( standard_tag == TIFFTAG_NUMBEROFINKS )
        {
            int i;
            for (i = 0; i < td->td_customValueCount; i++) {
                uint16_t val;
                TIFFTagValue *tv = td->td_customValues + i;
                if (tv->info->field_tag != standard_tag)
                    continue;
                if( tv->value == NULL )
                    return 0;
                val = *(uint16_t *)tv->value;
                /* Truncate to SamplesPerPixel, since the */
                /* setting code for INKNAMES assume that there are SamplesPerPixel */
                /* inknames. */
                /* Fixes http://bugzilla.maptools.org/show_bug.cgi?id=2599 */
                if( val > td->td_samplesperpixel )
                {
                    TIFFWarningExt(tif->tif_clientdata,"_TIFFVGetField",
                                   "Truncating NumberOfInks from %u to %"PRIu16,
                                   val, td->td_samplesperpixel);
                    val = td->td_samplesperpixel;
                }
                *va_arg(ap, uint16_t*) = val;
                return 1;
            }
            return 0;
        }

	switch (standard_tag) {
		case TIFFTAG_SUBFILETYPE:
			*va_arg(ap, uint32_t*) = td->td_subfiletype;
			break;
		case TIFFTAG_IMAGEWIDTH:
			*va_arg(ap, uint32_t*) = td->td_imagewidth;
			break;
		case TIFFTAG_IMAGELENGTH:
			*va_arg(ap, uint32_t*) = td->td_imagelength;
			break;
		case TIFFTAG_BITSPERSAMPLE:
			*va_arg(ap, uint16_t*) = td->td_bitspersample;
			break;
		case TIFFTAG_COMPRESSION:
			*va_arg(ap, uint16_t*) = td->td_compression;
			break;
		case TIFFTAG_PHOTOMETRIC:
			*va_arg(ap, uint16_t*) = td->td_photometric;
			break;
		case TIFFTAG_THRESHHOLDING:
			*va_arg(ap, uint16_t*) = td->td_threshholding;
			break;
		case TIFFTAG_FILLORDER:
			*va_arg(ap, uint16_t*) = td->td_fillorder;
			break;
		case TIFFTAG_ORIENTATION:
			*va_arg(ap, uint16_t*) = td->td_orientation;
			break;
		case TIFFTAG_SAMPLESPERPIXEL:
			*va_arg(ap, uint16_t*) = td->td_samplesperpixel;
			break;
		case TIFFTAG_ROWSPERSTRIP:
			*va_arg(ap, uint32_t*) = td->td_rowsperstrip;
			break;
		case TIFFTAG_MINSAMPLEVALUE:
			*va_arg(ap, uint16_t*) = td->td_minsamplevalue;
			break;
		case TIFFTAG_MAXSAMPLEVALUE:
			*va_arg(ap, uint16_t*) = td->td_maxsamplevalue;
			break;
		case TIFFTAG_SMINSAMPLEVALUE:
			if (tif->tif_flags & TIFF_PERSAMPLE) {
				*va_arg(ap, double**) = td->td_sminsamplevalue;
				if (tif->tif_flags & TIFF_GETFIELDRETCNT) {
					ret_val = (int)td->td_samplesperpixel;
				}
			}
			else
			{
				/* libtiff historically treats this as a single value. */
				uint16_t i;
				double v = td->td_sminsamplevalue[0];
				for (i=1; i < td->td_samplesperpixel; ++i)
					if( td->td_sminsamplevalue[i] < v )
						v = td->td_sminsamplevalue[i];
				*va_arg(ap, double*) = v;
			}
			break;
		case TIFFTAG_SMAXSAMPLEVALUE:
			if (tif->tif_flags & TIFF_PERSAMPLE) {
				*va_arg(ap, double**) = td->td_smaxsamplevalue;
				if (tif->tif_flags & TIFF_GETFIELDRETCNT) {
					ret_val = (int)td->td_samplesperpixel;
				}
			}
			else
			{
				/* libtiff historically treats this as a single value. */
				uint16_t i;
				double v = td->td_smaxsamplevalue[0];
				for (i=1; i < td->td_samplesperpixel; ++i)
					if( td->td_smaxsamplevalue[i] > v )
						v = td->td_smaxsamplevalue[i];
				*va_arg(ap, double*) = v;
			}
			break;
		case TIFFTAG_XRESOLUTION:
			*va_arg(ap, float*) = (float)((double)td->td_xresolution.uNum / (double)td->td_xresolution.uDenom);  /* SetGetRATIONAL_directly: */
			break;
		case TIFFTAG_YRESOLUTION:
			*va_arg(ap, float*) = (float)((double)td->td_yresolution.uNum / (double)td->td_yresolution.uDenom);  /* SetGetRATIONAL_directly: */
			break;
		case TIFFTAG_PLANARCONFIG:
			*va_arg(ap, uint16_t*) = td->td_planarconfig;
			break;
		case TIFFTAG_XPOSITION:
			*va_arg(ap, float*) = (float)((double)td->td_xposition.uNum / (double)td->td_xposition.uDenom);  /* SetGetRATIONAL_directly: */
			break;
		case TIFFTAG_YPOSITION:
			*va_arg(ap, float*) = (float)((double)td->td_yposition.uNum / (double)td->td_yposition.uDenom);  /* SetGetRATIONAL_directly: */
			break;
		case TIFFTAG_RESOLUTIONUNIT:
			*va_arg(ap, uint16_t*) = td->td_resolutionunit;
			break;
		case TIFFTAG_PAGENUMBER:
			*va_arg(ap, uint16_t*) = td->td_pagenumber[0];
			*va_arg(ap, uint16_t*) = td->td_pagenumber[1];
			break;
		case TIFFTAG_HALFTONEHINTS:
			*va_arg(ap, uint16_t*) = td->td_halftonehints[0];
			*va_arg(ap, uint16_t*) = td->td_halftonehints[1];
			break;
		case TIFFTAG_COLORMAP:
			*va_arg(ap, const uint16_t**) = td->td_colormap[0];
			*va_arg(ap, const uint16_t**) = td->td_colormap[1];
			*va_arg(ap, const uint16_t**) = td->td_colormap[2];
			break;
		case TIFFTAG_STRIPOFFSETS:
		case TIFFTAG_TILEOFFSETS:
			_TIFFFillStriles( tif );
			*va_arg(ap, const uint64_t**) = td->td_stripoffset_p;
			if (tif->tif_flags & TIFF_GETFIELDRETCNT) {
				ret_val = (int)td->td_nstrips;
			}
			break;
		case TIFFTAG_STRIPBYTECOUNTS:
		case TIFFTAG_TILEBYTECOUNTS:
			_TIFFFillStriles( tif );
			*va_arg(ap, const uint64_t**) = td->td_stripbytecount_p;
			if (tif->tif_flags & TIFF_GETFIELDRETCNT) {
				ret_val = (int)td->td_nstrips;
			}
			break;
		case TIFFTAG_MATTEING:
			*va_arg(ap, uint16_t*) =
			    (td->td_extrasamples == 1 &&
			    td->td_sampleinfo[0] == EXTRASAMPLE_ASSOCALPHA);
			break;
		case TIFFTAG_EXTRASAMPLES:
			*va_arg(ap, uint16_t*) = td->td_extrasamples;
			*va_arg(ap, const uint16_t**) = td->td_sampleinfo;
			break;
		case TIFFTAG_TILEWIDTH:
			*va_arg(ap, uint32_t*) = td->td_tilewidth;
			break;
		case TIFFTAG_TILELENGTH:
			*va_arg(ap, uint32_t*) = td->td_tilelength;
			break;
		case TIFFTAG_TILEDEPTH:
			*va_arg(ap, uint32_t*) = td->td_tiledepth;
			break;
		case TIFFTAG_DATATYPE:
			switch (td->td_sampleformat) {
				case SAMPLEFORMAT_UINT:
					*va_arg(ap, uint16_t*) = DATATYPE_UINT;
					break;
				case SAMPLEFORMAT_INT:
					*va_arg(ap, uint16_t*) = DATATYPE_INT;
					break;
				case SAMPLEFORMAT_IEEEFP:
					*va_arg(ap, uint16_t*) = DATATYPE_IEEEFP;
					break;
				case SAMPLEFORMAT_VOID:
					*va_arg(ap, uint16_t*) = DATATYPE_VOID;
					break;
			}
			break;
		case TIFFTAG_SAMPLEFORMAT:
			*va_arg(ap, uint16_t*) = td->td_sampleformat;
			break;
		case TIFFTAG_IMAGEDEPTH:
			*va_arg(ap, uint32_t*) = td->td_imagedepth;
			break;
		case TIFFTAG_SUBIFD:
			*va_arg(ap, uint16_t*) = td->td_nsubifd;
			*va_arg(ap, const uint64_t**) = td->td_subifd;
			break;
		case TIFFTAG_YCBCRPOSITIONING:
			*va_arg(ap, uint16_t*) = td->td_ycbcrpositioning;
			break;
		case TIFFTAG_YCBCRSUBSAMPLING:
			*va_arg(ap, uint16_t*) = td->td_ycbcrsubsampling[0];
			*va_arg(ap, uint16_t*) = td->td_ycbcrsubsampling[1];
			break;
		case TIFFTAG_TRANSFERFUNCTION:
			*va_arg(ap, const uint16_t**) = td->td_transferfunction[0];
			if (td->td_samplesperpixel - td->td_extrasamples > 1) {
				*va_arg(ap, const uint16_t**) = td->td_transferfunction[1];
				*va_arg(ap, const uint16_t**) = td->td_transferfunction[2];
			} else {
				*va_arg(ap, const uint16_t**) = NULL;
				*va_arg(ap, const uint16_t**) = NULL;
			}
			break;
		case TIFFTAG_REFERENCEBLACKWHITE:
			{
				/* SetGetRATIONAL_directly: Return value needs a pointer to a float-array. Get memmory for float-array: */
				float* pf;
				TIFFRational_t* pR;
				if (td->td_refblackwhite2 != NULL) {
					_TIFFfree(td->td_refblackwhite2);
					td->td_refblackwhite2 = NULL;
				}
				_TIFFsetFloatArray((float **)&td->td_refblackwhite2, (float*)td->td_refblackwhite, 6);
				/* Now fill array with correct values: */
				pf = td->td_refblackwhite2;
				pR = td->td_refblackwhite;
				if (pf != NULL && pR != NULL) {
					for (int i = 0; i < 6; i++) {
						*pf = (float)((double)pR->uNum / (double)pR->uDenom);
						pf++;
						pR++;
					}
				}
				*va_arg(ap, const float**) = td->td_refblackwhite2;
			if (tif->tif_flags & TIFF_GETFIELDRETCNT) {
				ret_val = 6;
			}
			break;
		case TIFFTAG_INKNAMES:
			*va_arg(ap, const char**) = td->td_inknames;
			/* return the length of the buffer so the caller
			 * can avoid buffer overrun */
			ret_val = (int)td->td_inknameslen;
			break;
		default:
			{
				int i;

				/*
				 * This can happen if multiple images are open
				 * with different codecs which have private
				 * tags.  The global tag information table may
				 * then have tags that are valid for one file
				 * but not the other. If the client tries to
				 * get a tag that is not valid for the image's
				 * codec then we'll arrive here.
				 */
				if( fip->field_bit != FIELD_CUSTOM )
				{
					TIFFErrorExt(tif->tif_clientdata, "_TIFFVGetField",
					    "%s: Invalid %stag \"%s\" "
					    "(not supported by codec)",
					    tif->tif_name,
					    isPseudoTag(tag) ? "pseudo-" : "",
					    fip->field_name);
					ret_val = 0;
					break;
				}

				/*
				 * Do we have a custom value?
				 */
				ret_val = 0;
				for (i = 0; i < td->td_customValueCount; i++) {
					int tv_set_get_size;
					TIFFTagValue *tv = td->td_customValues + i;

					if (tv->info->field_tag != tag)
						continue;

					if (fip->field_passcount) {
						if (fip->field_readcount == TIFF_VARIABLE2)
							*va_arg(ap, uint32_t*) = (uint32_t)tv->count;
						else  /* Assume TIFF_VARIABLE */
							*va_arg(ap, uint16_t*) = (uint16_t)tv->count;

						/* SetGetRATIONAL_directly:_CustomTag:  TIFFVGetField() enhanced for variable RATIONAl arrays ----> */
						tv_set_get_size = _TIFFSetGetFieldSize(fip->set_field_type);
						if (fip->field_type == TIFF_RATIONAL) {
							if (tv_set_get_size == 4) {
								_TIFFsetFloatArrayFromRational((float **)&tv->value2, (TIFFRational_t*)tv->value, tv->count, FALSE, TRUE);
							}
							else {
								_TIFFsetDoubleArrayFromRational((double**)&tv->value2, (TIFFRational_t*)tv->value, tv->count, FALSE, TRUE);
							}
							*va_arg(ap, const void**) = tv->value2;
						}
						else if (fip->field_type == TIFF_SRATIONAL) {
							if (tv_set_get_size == 4) {
								_TIFFsetFloatArrayFromRational((float**)&tv->value2, (TIFFRational_t*)tv->value, tv->count, TRUE, TRUE);
							}
							else {
								_TIFFsetDoubleArrayFromRational((double**)&tv->value2, (TIFFRational_t*)tv->value, tv->count, TRUE, TRUE);
							}
							*va_arg(ap, const void**) = tv->value2;
						}
						else {
							*va_arg(ap, const void**) = tv->value;
						}
						ret_val = 1;
					} else if (fip->field_tag == TIFFTAG_DOTRANGE
						   && strcmp(fip->field_name,"DotRange") == 0) {
						/* TODO: This is an evil exception and should not have been
						   handled this way ... likely best if we move it into
						   the directory structure with an explicit field in 
						   libtiff 4.1 and assign it a FIELD_ value */
						*va_arg(ap, uint16_t*) = ((uint16_t *)tv->value)[0];
						*va_arg(ap, uint16_t*) = ((uint16_t *)tv->value)[1];
						ret_val = 1;
					} else {
						if (fip->field_type == TIFF_ASCII
						    || fip->field_readcount == TIFF_VARIABLE
						    || fip->field_readcount == TIFF_VARIABLE2
						    || fip->field_readcount == TIFF_SPP
						    || tv->count > 1) {
							/* SetGetRATIONAL_directly:_CustomTag:  TIFFVGetField() enhance for fixed RATIONAl arrays ----> */
							int tv_set_get_size = _TIFFSetGetFieldSize(fip->set_field_type);
							if (fip->field_type == TIFF_RATIONAL) {
								if (tv_set_get_size == 4) {
									_TIFFsetFloatArrayFromRational((float**)&tv->value2, (TIFFRational_t*)tv->value, tv->count, FALSE, TRUE);
								}
								else {
									_TIFFsetDoubleArrayFromRational((double**)&tv->value2, (TIFFRational_t*)tv->value, tv->count, FALSE, TRUE);
								}
								*va_arg(ap, const void**) = tv->value2;
							}
							else if (fip->field_type == TIFF_SRATIONAL) {
								if (tv_set_get_size == 4) {
									_TIFFsetFloatArrayFromRational((float**)&tv->value2, (TIFFRational_t*)tv->value, tv->count, TRUE, TRUE);
								}
								else {
									_TIFFsetDoubleArrayFromRational((double**)&tv->value2, (TIFFRational_t*)tv->value, tv->count, TRUE, TRUE);
								}
								*va_arg(ap, const void**) = tv->value2;
								ret_val = 1;
							}
							else {
								*va_arg(ap, const void**) = tv->value;
								if (tif->tif_flags & TIFF_GETFIELDRETCNT) {
									ret_val = (int)tv->count;
								} else {
									ret_val = 1;
								}
							}
						} else {
							char *val = (char *)tv->value;
							assert( tv->count == 1 );
							switch (fip->field_type) {
							case TIFF_BYTE:
							case TIFF_UNDEFINED:
								*va_arg(ap, uint8_t*) =
									*(uint8_t *)val;
								ret_val = 1;
								break;
							case TIFF_SBYTE:
								*va_arg(ap, int8_t*) =
									*(int8_t *)val;
								ret_val = 1;
								break;
							case TIFF_SHORT:
								*va_arg(ap, uint16_t*) =
									*(uint16_t *)val;
								ret_val = 1;
								break;
							case TIFF_SSHORT:
								*va_arg(ap, int16_t*) =
									*(int16_t *)val;
								ret_val = 1;
								break;
							case TIFF_LONG:
							case TIFF_IFD:
								*va_arg(ap, uint32_t*) =
									*(uint32_t *)val;
								ret_val = 1;
								break;
							case TIFF_SLONG:
								*va_arg(ap, int32_t*) =
									*(int32_t *)val;
								ret_val = 1;
								break;
							case TIFF_LONG8:
							case TIFF_IFD8:
								*va_arg(ap, uint64_t*) =
									*(uint64_t *)val;
								ret_val = 1;
								break;
							case TIFF_SLONG8:
								*va_arg(ap, int64_t*) =
									*(int64_t *)val;
								ret_val = 1;
								break;
							case TIFF_RATIONAL:
							case TIFF_SRATIONAL:
								{
									/*-- Rational2Double: For Rationals evaluate "set_field_type" to determine API interface type and return value size. */
									int tv_size = _TIFFSetGetFieldSize(fip->set_field_type);
									/*-- SetGetRATIONAL_directly:_CustomTag: */
									double v2;
									TIFFRational_t* uR = (TIFFRational_t*)val;
									TIFFSRational_t* sR = (TIFFSRational_t*)val;
									/* distiguish between unsigned / signed rationals here */
									if (fip->field_type == TIFF_RATIONAL) {
										v2 = ((double)uR->uNum / (double)uR->uDenom);  
									}
									else {
										/* TIFF_SRATIONAL */
										v2 = ((double)sR->sNum / (double)sR->sDenom); 
									}
									/* return value through fitting API interface */
									if (fip->set_field_type == TIFF_SETGET_DOUBLE) {
										//*va_arg(ap, double*) = *(double *)val;
										*va_arg(ap, double*) = v2;
										ret_val = 1;
									} else {
										/*-- default should be tv_size == 4  */
										*va_arg(ap, float*) = (float)v2;
										ret_val = 1;
										/*-- ToDo: After Testing, this should be removed and tv_size==4 should be set as default. */
										if (tv_size != 4) {
											TIFFErrorExt(0,"TIFFLib: _TIFFVGetField()", "Rational2Double: .set_field_type in not 4 but %d", tv_size); 
										}
									}
								}
								break;
							case TIFF_FLOAT:
								*va_arg(ap, float*) =
									*(float *)val;
								ret_val = 1;
								break;
							case TIFF_DOUBLE:
								*va_arg(ap, double*) =
									*(double *)val;
								ret_val = 1;
								break;
							default:
								ret_val = 0;
								break;
							}
						}
					}
					break;
				}
			}
	}
	return(ret_val);
} /*-- _TIFFVGetField() --*/



static int
_TIFFVGetFieldRational(TIFF* tif, uint32_t tag, va_list ap)
{
	/* SetGetRATIONAL_directly:
	 * _TIFFVGetFieldRational() is necessary to distinguish the va_list parameters for directly setting rational values
	 * from the standard case, where a float value is provided.
	 *
	 * This routine expects as va_list: Pointer to Numerator , Denominator as uint_32-pointer (or int_32-pointer for signed rationals),
	 * or returns the pointer of the internal storage onto the rational array.
	 */
	static const char module[] = "_TIFFVGetFieldRational";

	TIFFDirectory* td = &tif->tif_dir;
	int ret_val = 1;
	uint32_t standard_tag = tag;
	const TIFFField* fip = TIFFFindField(tif, tag, TIFF_ANY);
	if (fip == NULL) /* cannot happen since TIFFGetField() already checks it */
		return 0;

	/* Check for error if it is not a RATIONAL-tag. */
	if (fip->field_type != TIFF_RATIONAL && fip->field_type != TIFF_SRATIONAL) {
		TIFFErrorExt(tif->tif_clientdata, module, "This function accepts only rational tif-tag!");
		return 0;
	}

	/*
	 * We want to force the custom code to be used for custom
	 * fields even if the tag happens to match a well known
	 * one - important for reinterpreted handling of standard
	 * tag values in custom directories (i.e. EXIF)
	 */
	if (fip->field_bit == FIELD_CUSTOM) {
		standard_tag = 0;
	}

	switch (standard_tag) {
		/* hereafter, only rational tags are handled */
		case TIFFTAG_XRESOLUTION:
			*va_arg(ap, uint32_t*) = td->td_xresolution.uNum;
			*va_arg(ap, uint32_t*) = td->td_xresolution.uDenom;
			break;
		case TIFFTAG_YRESOLUTION:
			*va_arg(ap, uint32_t*) = td->td_yresolution.uNum;
			*va_arg(ap, uint32_t*) = td->td_yresolution.uDenom;
			break;
		case TIFFTAG_REFERENCEBLACKWHITE:
			/* a pointer to rational array with 6 entries is expected */
			*va_arg(ap, TIFFRational_t**) = td->td_refblackwhite;
			break;


		default:
		{
			int i;

			/*
			 * This can happen if multiple images are open
			 * with different codecs which have private
			 * tags.  The global tag information table may
			 * then have tags that are valid for one file
			 * but not the other. If the client tries to
			 * get a tag that is not valid for the image's
			 * codec then we'll arrive here.
			 */
			if (fip->field_bit != FIELD_CUSTOM) {
				TIFFErrorExt(tif->tif_clientdata, module,
					"%s: Invalid %stag \"%s\" "
					"(not supported by codec)",
					tif->tif_name,
					isPseudoTag(tag) ? "pseudo-" : "",
					fip->field_name);
				ret_val = 0;
				break;
			}

			/*
			 * Do we have a custom value?
			 */
			ret_val = 0;
			for (i = 0; i < td->td_customValueCount; i++) {
				TIFFTagValue* tv = td->td_customValues + i;

				if (tv->info->field_tag != tag)
					continue;

				if (fip->field_passcount) {
					if (fip->field_readcount == TIFF_VARIABLE2)
						*va_arg(ap, uint32_t*) = (uint32_t)tv->count;
					else  /* Assume TIFF_VARIABLE */
						*va_arg(ap, uint16_t*) = (uint16_t)tv->count;
					*va_arg(ap, const void**) = tv->value;
					ret_val = 1;
				} else if (fip->field_tag == TIFFTAG_DOTRANGE
					&& strcmp(fip->field_name, "DotRange") == 0) {
					/* TODO: This is an evil exception and should not have been
					   handled this way ... likely best if we move it into
					   the directory structure with an explicit field in
					   libtiff 4.1 and assign it a FIELD_ value */
					*va_arg(ap, uint16_t*) = ((uint16_t*)tv->value)[0];
					*va_arg(ap, uint16_t*) = ((uint16_t*)tv->value)[1];
					ret_val = 1;
				} else {
					if (fip->field_type == TIFF_ASCII
						|| fip->field_readcount == TIFF_VARIABLE
						|| fip->field_readcount == TIFF_VARIABLE2
						|| fip->field_readcount == TIFF_SPP
						|| tv->count > 1) {
						*va_arg(ap, void**) = tv->value;
						ret_val = 1;
					} else {
						char* val = (char*)tv->value;
						assert(tv->count == 1);
						switch (fip->field_type) {
							/*-- SetGetRATIONAL_directly:_CustomTag: */
							/* distiguish between unsigned / signed rationals here */
							case TIFF_RATIONAL:
							{
								TIFFRational_t* uR = (TIFFRational_t*)val;
								*va_arg(ap, uint32_t*) = uR->uNum;
								*va_arg(ap, uint32_t*) = uR->uDenom;
							}
							break;
							case TIFF_SRATIONAL:
							{
								TIFFSRational_t* sR = (TIFFSRational_t*)val;
								*va_arg(ap, int32_t*) = sR->sNum;
								*va_arg(ap, int32_t*) = sR->sDenom;
							}
							break;
							default:
								TIFFErrorExt(tif->tif_clientdata, module,
									"%s: Field type %d \"%s\" ",
									tif->tif_name,
									fip->field_type,
									fip->field_name);
								ret_val = 0;
								break;
						}
					}
				} /* else if ...*/
				break;   /* break for loop */
			} /*for */
		} /* default: of switch(standard_tag) */
	} /*-- switch(standard_tag) --*/
	return(ret_val);
} /*-- _TIFFVGetFieldRational() --*/


/*
 * Return the value of a field in the
 * internal directory structure.
 */
int
TIFFGetField(TIFF* tif, uint32_t tag, ...)
{
	int status;
	va_list ap;

	va_start(ap, tag);
	status = TIFFVGetField(tif, tag, ap);
	va_end(ap);
	return (status);
}

int
TIFFGetFieldRational(TIFF* tif, uint32_t tag, ...)
{	/* SetGetRATIONAL_directly: */
	int status;
	va_list ap;

	va_start(ap, tag);
	status = TIFFVGetFieldRational(tif, tag, ap);
	va_end(ap);
	return (status);
} /*-- TIFFGetFieldRational() --*/

/*
 * Like TIFFGetField, but taking a varargs
 * parameter list.  This routine is useful
 * for building higher-level interfaces on
 * top of the library.
 */
int
TIFFVGetField(TIFF* tif, uint32_t tag, va_list ap)
{
	const TIFFField* fip = TIFFFindField(tif, tag, TIFF_ANY);
	return (fip && (isPseudoTag(tag) || TIFFFieldSet(tif, fip->field_bit)) ?
	    (*tif->tif_tagmethods.vgetfield)(tif, tag, ap) : 0);
}

int
TIFFVGetFieldRational(TIFF* tif, uint32_t tag, va_list ap)
{	/* SetGetRATIONAL_directly: */
	const TIFFField* fip = TIFFFindField(tif, tag, TIFF_ANY);
	return (fip && (isPseudoTag(tag) || TIFFFieldSet(tif, fip->field_bit)) ?
		(*tif->tif_tagmethods.vgetfieldrational)(tif, tag, ap) : 0);
}

#define	CleanupField(member) {		\
    if (td->member) {			\
	_TIFFfree(td->member);		\
	td->member = 0;			\
    }					\
}

/*
 * Release storage associated with a directory.
 */
void
TIFFFreeDirectory(TIFF* tif)
{
	TIFFDirectory *td = &tif->tif_dir;
	int            i;

	_TIFFmemset(td->td_fieldsset, 0, FIELD_SETLONGS);
	CleanupField(td_sminsamplevalue);
	CleanupField(td_smaxsamplevalue);
	CleanupField(td_colormap[0]);
	CleanupField(td_colormap[1]);
	CleanupField(td_colormap[2]);
	CleanupField(td_sampleinfo);
	CleanupField(td_subifd);
	CleanupField(td_inknames);
	CleanupField(td_refblackwhite);
	CleanupField(td_refblackwhite2); /*-- SetGetRATIONAL_directly: --*/
	CleanupField(td_transferfunction[0]);
	CleanupField(td_transferfunction[1]);
	CleanupField(td_transferfunction[2]);
	CleanupField(td_stripoffset_p);
	CleanupField(td_stripbytecount_p);
        td->td_stripoffsetbyteallocsize = 0;
	TIFFClrFieldBit(tif, FIELD_YCBCRSUBSAMPLING);
	TIFFClrFieldBit(tif, FIELD_YCBCRPOSITIONING);

	/* Cleanup custom tag values */
	for( i = 0; i < td->td_customValueCount; i++ ) {
		if (td->td_customValues[i].value)
			_TIFFfree(td->td_customValues[i].value);
		/*-- SetGetRATIONAL_directly:_CustomTag:  --*/
		if (td->td_customValues[i].value2)
			_TIFFfree(td->td_customValues[i].value2);
	}

	td->td_customValueCount = 0;
	CleanupField(td_customValues);

        _TIFFmemset( &(td->td_stripoffset_entry), 0, sizeof(TIFFDirEntry));
        _TIFFmemset( &(td->td_stripbytecount_entry), 0, sizeof(TIFFDirEntry));
}
#undef CleanupField

/*
 * Client Tag extension support (from Niles Ritter).
 */
static TIFFExtendProc _TIFFextender = (TIFFExtendProc) NULL;

TIFFExtendProc
TIFFSetTagExtender(TIFFExtendProc extender)
{
	TIFFExtendProc prev = _TIFFextender;
	_TIFFextender = extender;
	return (prev);
}

/*
 * Setup for a new directory.  Should we automatically call
 * TIFFWriteDirectory() if the current one is dirty?
 *
 * The newly created directory will not exist on the file till
 * TIFFWriteDirectory(), TIFFFlush() or TIFFClose() is called.
 */
int
TIFFCreateDirectory(TIFF* tif)
{
	TIFFDefaultDirectory(tif);
	tif->tif_diroff = 0;
	tif->tif_nextdiroff = 0;
	tif->tif_curoff = 0;
	tif->tif_row = (uint32_t) -1;
	tif->tif_curstrip = (uint32_t) -1;

	return 0;
}

int
TIFFCreateCustomDirectory(TIFF* tif, const TIFFFieldArray* infoarray)
{
	TIFFDefaultDirectory(tif);

	/*
	 * Reset the field definitions to match the application provided list. 
	 * Hopefully TIFFDefaultDirectory() won't have done anything irreversible
	 * based on it's assumption this is an image directory.
	 */
	_TIFFSetupFields(tif, infoarray);

	tif->tif_diroff = 0;
	tif->tif_nextdiroff = 0;
	tif->tif_curoff = 0;
	tif->tif_row = (uint32_t) -1;
	tif->tif_curstrip = (uint32_t) -1;

	return 0;
}

int
TIFFCreateEXIFDirectory(TIFF* tif)
{
	const TIFFFieldArray* exifFieldArray;
	exifFieldArray = _TIFFGetExifFields();
	return TIFFCreateCustomDirectory(tif, exifFieldArray);
}

/*
 * Creates the EXIF GPS custom directory 
 */
int
TIFFCreateGPSDirectory(TIFF* tif)
{
	const TIFFFieldArray* gpsFieldArray;
	gpsFieldArray = _TIFFGetGpsFields();
	return TIFFCreateCustomDirectory(tif, gpsFieldArray);
}

/*
 * Setup a default directory structure.
 */
int
TIFFDefaultDirectory(TIFF* tif)
{
	register TIFFDirectory* td = &tif->tif_dir;
	const TIFFFieldArray* tiffFieldArray;

	tiffFieldArray = _TIFFGetFields();
	_TIFFSetupFields(tif, tiffFieldArray);   

	_TIFFmemset(td, 0, sizeof (*td));
	td->td_fillorder = FILLORDER_MSB2LSB;
	td->td_bitspersample = 1;
	td->td_threshholding = THRESHHOLD_BILEVEL;
	td->td_orientation = ORIENTATION_TOPLEFT;
	td->td_samplesperpixel = 1;
	td->td_rowsperstrip = (uint32_t) -1;
	td->td_tilewidth = 0;
	td->td_tilelength = 0;
	td->td_tiledepth = 1;
#ifdef STRIPBYTECOUNTSORTED_UNUSED
	td->td_stripbytecountsorted = 1; /* Our own arrays always sorted. */  
#endif
	td->td_resolutionunit = RESUNIT_INCH;
	td->td_sampleformat = SAMPLEFORMAT_UINT;
	td->td_imagedepth = 1;
	td->td_ycbcrsubsampling[0] = 2;
	td->td_ycbcrsubsampling[1] = 2;
	td->td_ycbcrpositioning = YCBCRPOSITION_CENTERED;
	tif->tif_postdecode = _TIFFNoPostDecode;  
	tif->tif_foundfield = NULL;
	tif->tif_tagmethods.vsetfield = _TIFFVSetField;  
	tif->tif_tagmethods.vgetfield = _TIFFVGetField;
	/* SetGetRATIONAL_directly: */
	tif->tif_tagmethods.vsetfieldrational = _TIFFVSetFieldRational;
	tif->tif_tagmethods.vgetfieldrational = _TIFFVGetFieldRational;
	tif->tif_tagmethods.printdir = NULL;
	/*
	 *  Give client code a chance to install their own
	 *  tag extensions & methods, prior to compression overloads,
	 *  but do some prior cleanup first. (http://trac.osgeo.org/gdal/ticket/5054)
	 */
	if (tif->tif_nfieldscompat > 0) {
		uint32_t i;

		for (i = 0; i < tif->tif_nfieldscompat; i++) {
				if (tif->tif_fieldscompat[i].allocated_size)
						_TIFFfree(tif->tif_fieldscompat[i].fields);
		}
		_TIFFfree(tif->tif_fieldscompat);
		tif->tif_nfieldscompat = 0;
		tif->tif_fieldscompat = NULL;
	}
	if (_TIFFextender)
		(*_TIFFextender)(tif);
	(void) TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	/*
	 * NB: The directory is marked dirty as a result of setting
	 * up the default compression scheme.  However, this really
	 * isn't correct -- we want TIFF_DIRTYDIRECT to be set only
	 * if the user does something.  We could just do the setup
	 * by hand, but it seems better to use the normal mechanism
	 * (i.e. TIFFSetField).
	 */
	tif->tif_flags &= ~TIFF_DIRTYDIRECT;

	/*
	 * As per http://bugzilla.remotesensing.org/show_bug.cgi?id=19
	 * we clear the ISTILED flag when setting up a new directory.
	 * Should we also be clearing stuff like INSUBIFD?
	 */
	tif->tif_flags &= ~TIFF_ISTILED;

	return (1);
}

static int
TIFFAdvanceDirectory(TIFF* tif, uint64_t* nextdir, uint64_t* off)
{
	static const char module[] = "TIFFAdvanceDirectory";
	if (isMapped(tif))
	{
		uint64_t poff=*nextdir;
		if (!(tif->tif_flags&TIFF_BIGTIFF))
		{
			tmsize_t poffa,poffb,poffc,poffd;
			uint16_t dircount;
			uint32_t nextdir32;
			poffa=(tmsize_t)poff;
			poffb=poffa+sizeof(uint16_t);
			if (((uint64_t)poffa != poff) || (poffb < poffa) || (poffb < (tmsize_t)sizeof(uint16_t)) || (poffb > tif->tif_size))
			{
				TIFFErrorExt(tif->tif_clientdata,module,"Error fetching directory count");
                                  *nextdir=0;
				return(0);
			}
			_TIFFmemcpy(&dircount,tif->tif_base+poffa,sizeof(uint16_t));
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabShort(&dircount);
			poffc=poffb+dircount*12;
			poffd=poffc+sizeof(uint32_t);
			if ((poffc<poffb) || (poffc<dircount*12) || (poffd<poffc) || (poffd<(tmsize_t)sizeof(uint32_t)) || (poffd > tif->tif_size))
			{
				TIFFErrorExt(tif->tif_clientdata,module,"Error fetching directory link");
				return(0);
			}
			if (off!=NULL)
				*off=(uint64_t)poffc;
			_TIFFmemcpy(&nextdir32,tif->tif_base+poffc,sizeof(uint32_t));
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong(&nextdir32);
			*nextdir=nextdir32;
		}
		else
		{
			tmsize_t poffa,poffb,poffc,poffd;
			uint64_t dircount64;
			uint16_t dircount16;
			poffa=(tmsize_t)poff;
			poffb=poffa+sizeof(uint64_t);
			if (((uint64_t)poffa != poff) || (poffb < poffa) || (poffb < (tmsize_t)sizeof(uint64_t)) || (poffb > tif->tif_size))
			{
				TIFFErrorExt(tif->tif_clientdata,module,"Error fetching directory count");
				return(0);
			}
			_TIFFmemcpy(&dircount64,tif->tif_base+poffa,sizeof(uint64_t));
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong8(&dircount64);
			if (dircount64>0xFFFF)
			{
				TIFFErrorExt(tif->tif_clientdata,module,"Sanity check on directory count failed");
				return(0);
			}
			dircount16=(uint16_t)dircount64;
			poffc=poffb+dircount16*20;
			poffd=poffc+sizeof(uint64_t);
			if ((poffc<poffb) || (poffc<dircount16*20) || (poffd<poffc) || (poffd<(tmsize_t)sizeof(uint64_t)) || (poffd > tif->tif_size))
			{
				TIFFErrorExt(tif->tif_clientdata,module,"Error fetching directory link");
				return(0);
			}
			if (off!=NULL)
				*off=(uint64_t)poffc;
			_TIFFmemcpy(nextdir,tif->tif_base+poffc,sizeof(uint64_t));
			if (tif->tif_flags&TIFF_SWAB)
				TIFFSwabLong8(nextdir);
		}
		return(1);
	}
	else
	{
		if (!(tif->tif_flags&TIFF_BIGTIFF))
		{
			uint16_t dircount;
			uint32_t nextdir32;
			if (!SeekOK(tif, *nextdir) ||
			    !ReadOK(tif, &dircount, sizeof (uint16_t))) {
				TIFFErrorExt(tif->tif_clientdata, module, "%s: Error fetching directory count",
				    tif->tif_name);
				return (0);
			}
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabShort(&dircount);
			if (off != NULL)
				*off = TIFFSeekFile(tif,
				    dircount*12, SEEK_CUR);
			else
				(void) TIFFSeekFile(tif,
				    dircount*12, SEEK_CUR);
			if (!ReadOK(tif, &nextdir32, sizeof (uint32_t))) {
				TIFFErrorExt(tif->tif_clientdata, module, "%s: Error fetching directory link",
				    tif->tif_name);
				return (0);
			}
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabLong(&nextdir32);
			*nextdir=nextdir32;
		}
		else
		{
			uint64_t dircount64;
			uint16_t dircount16;
			if (!SeekOK(tif, *nextdir) ||
			    !ReadOK(tif, &dircount64, sizeof (uint64_t))) {
				TIFFErrorExt(tif->tif_clientdata, module, "%s: Error fetching directory count",
				    tif->tif_name);
				return (0);
			}
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabLong8(&dircount64);
			if (dircount64>0xFFFF)
			{
				TIFFErrorExt(tif->tif_clientdata, module, "Error fetching directory count");
				return(0);
			}
			dircount16 = (uint16_t)dircount64;
			if (off != NULL)
				*off = TIFFSeekFile(tif,
				    dircount16*20, SEEK_CUR);
			else
				(void) TIFFSeekFile(tif,
				    dircount16*20, SEEK_CUR);
			if (!ReadOK(tif, nextdir, sizeof (uint64_t))) {
				TIFFErrorExt(tif->tif_clientdata, module,
                                             "%s: Error fetching directory link",
				    tif->tif_name);
				return (0);
			}
			if (tif->tif_flags & TIFF_SWAB)
				TIFFSwabLong8(nextdir);
		}
		return (1);
	}
}

/*
 * Count the number of directories in a file.
 */
uint16_t
TIFFNumberOfDirectories(TIFF* tif)
{
	static const char module[] = "TIFFNumberOfDirectories";
	uint64_t nextdir;
	uint16_t n;
	if (!(tif->tif_flags&TIFF_BIGTIFF))
		nextdir = tif->tif_header.classic.tiff_diroff;
	else
		nextdir = tif->tif_header.big.tiff_diroff;
	n = 0;
	while (nextdir != 0 && TIFFAdvanceDirectory(tif, &nextdir, NULL))
        {
                if (n != 65535) {
                        ++n;
                }
		else
                {
                        TIFFErrorExt(tif->tif_clientdata, module,
                                     "Directory count exceeded 65535 limit,"
                                     " giving up on counting.");
                        return (65535);
                }
        }
	return (n);
}

/*
 * Set the n-th directory as the current directory.
 * NB: Directories are numbered starting at 0.
 */
int
TIFFSetDirectory(TIFF* tif, uint16_t dirn)
{
	uint64_t nextdir;
	uint16_t n;

	if (!(tif->tif_flags&TIFF_BIGTIFF))
		nextdir = tif->tif_header.classic.tiff_diroff;
	else
		nextdir = tif->tif_header.big.tiff_diroff;
	for (n = dirn; n > 0 && nextdir != 0; n--)
		if (!TIFFAdvanceDirectory(tif, &nextdir, NULL))
			return (0);
	tif->tif_nextdiroff = nextdir;
	/*
	 * Set curdir to the actual directory index.  The
	 * -1 is because TIFFReadDirectory will increment
	 * tif_curdir after successfully reading the directory.
	 */
	tif->tif_curdir = (dirn - n) - 1;
	/*
	 * Reset tif_dirnumber counter and start new list of seen directories.
	 * We need this to prevent IFD loops.
	 */
	tif->tif_dirnumber = 0;
	return (TIFFReadDirectory(tif));
}

/*
 * Set the current directory to be the directory
 * located at the specified file offset.  This interface
 * is used mainly to access directories linked with
 * the SubIFD tag (e.g. thumbnail images).
 */
int
TIFFSetSubDirectory(TIFF* tif, uint64_t diroff)
{
	tif->tif_nextdiroff = diroff;
	/*
	 * Reset tif_dirnumber counter and start new list of seen directories.
	 * We need this to prevent IFD loops.
	 */
	tif->tif_dirnumber = 0;
	return (TIFFReadDirectory(tif));
}

/*
 * Return file offset of the current directory.
 */
uint64_t
TIFFCurrentDirOffset(TIFF* tif)
{
	return (tif->tif_diroff);
}

/*
 * Return an indication of whether or not we are
 * at the last directory in the file.
 */
int
TIFFLastDirectory(TIFF* tif)
{
	return (tif->tif_nextdiroff == 0);
}

/*
 * Unlink the specified directory from the directory chain.
 */
int
TIFFUnlinkDirectory(TIFF* tif, uint16_t dirn)
{
	static const char module[] = "TIFFUnlinkDirectory";
	uint64_t nextdir;
	uint64_t off;
	uint16_t n;

	if (tif->tif_mode == O_RDONLY) {
		TIFFErrorExt(tif->tif_clientdata, module,
                             "Can not unlink directory in read-only file");
		return (0);
	}
	/*
	 * Go to the directory before the one we want
	 * to unlink and nab the offset of the link
	 * field we'll need to patch.
	 */
	if (!(tif->tif_flags&TIFF_BIGTIFF))
	{
		nextdir = tif->tif_header.classic.tiff_diroff;
		off = 4;
	}
	else
	{
		nextdir = tif->tif_header.big.tiff_diroff;
		off = 8;
	}
	for (n = dirn-1; n > 0; n--) {
		if (nextdir == 0) {
			TIFFErrorExt(tif->tif_clientdata, module, "Directory %"PRIu16" does not exist", dirn);
			return (0);
		}
		if (!TIFFAdvanceDirectory(tif, &nextdir, &off))
			return (0);
	}
	/*
	 * Advance to the directory to be unlinked and fetch
	 * the offset of the directory that follows.
	 */
	if (!TIFFAdvanceDirectory(tif, &nextdir, NULL))
		return (0);
	/*
	 * Go back and patch the link field of the preceding
	 * directory to point to the offset of the directory
	 * that follows.
	 */
	(void) TIFFSeekFile(tif, off, SEEK_SET);
	if (!(tif->tif_flags&TIFF_BIGTIFF))
	{
		uint32_t nextdir32;
		nextdir32=(uint32_t)nextdir;
		assert((uint64_t)nextdir32 == nextdir);
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong(&nextdir32);
		if (!WriteOK(tif, &nextdir32, sizeof (uint32_t))) {
			TIFFErrorExt(tif->tif_clientdata, module, "Error writing directory link");
			return (0);
		}
	}
	else
	{
		if (tif->tif_flags & TIFF_SWAB)
			TIFFSwabLong8(&nextdir);
		if (!WriteOK(tif, &nextdir, sizeof (uint64_t))) {
			TIFFErrorExt(tif->tif_clientdata, module, "Error writing directory link");
			return (0);
		}
	}
	/*
	 * Leave directory state setup safely.  We don't have
	 * facilities for doing inserting and removing directories,
	 * so it's safest to just invalidate everything.  This
	 * means that the caller can only append to the directory
	 * chain.
	 */
	(*tif->tif_cleanup)(tif);
	if ((tif->tif_flags & TIFF_MYBUFFER) && tif->tif_rawdata) {
		_TIFFfree(tif->tif_rawdata);
		tif->tif_rawdata = NULL;
		tif->tif_rawcc = 0;
                tif->tif_rawdataoff = 0;
                tif->tif_rawdataloaded = 0;
	}
	tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP|TIFF_POSTENCODE|TIFF_BUF4WRITE);
	TIFFFreeDirectory(tif);
	TIFFDefaultDirectory(tif);
	tif->tif_diroff = 0;			/* force link on next write */
	tif->tif_nextdiroff = 0;		/* next write must be at end */
	tif->tif_curoff = 0;
	tif->tif_row = (uint32_t) -1;
	tif->tif_curstrip = (uint32_t) -1;
	return (1);
}

/* vim: set ts=8 sts=8 sw=8 noet: */
/*
 * Local Variables:
 * mode: c
 * c-basic-offset: 8
 * fill-column: 78
 * End:
 */
