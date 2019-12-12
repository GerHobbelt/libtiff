// Elapse Timer 
/** @file ElapseTimer.cpp
 ** ElapseTimer facilities.
 ** Implemented ElapseTimer.cpp Code stolen from Scintilla Platform.h and PlatWX.cxx
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

/*-- Example:
 *   ElapseTimer *myTimer = new ElapseTimer();
 *	 // reset timer
 *	myTimer->Duration(true);
 *	TimeDurationActual = (off64_t)(myTimer->Duration() * 1000.);

*/

#include "ElapseTimer.hpp"

//--- Elapse Timer class ---
static bool initialisedET = false;
static bool usePerformanceCounter = false;
static LARGE_INTEGER frequency;

ElapseTimer::ElapseTimer() {
	if (!initialisedET) {
		usePerformanceCounter = ::QueryPerformanceFrequency(&frequency) != 0;
		initialisedET = true;
	}
	if (usePerformanceCounter) {
		LARGE_INTEGER timeVal;
		::QueryPerformanceCounter(&timeVal);
		bigBit = timeVal.HighPart;
		littleBit = timeVal.LowPart;
	} else {
		bigBit = clock();
	}
}

double ElapseTimer::Duration(bool reset) {
	double result;
	long endBigBit;
	long endLittleBit;

	if (usePerformanceCounter) {
		LARGE_INTEGER lEnd;
		::QueryPerformanceCounter(&lEnd);
		endBigBit = lEnd.HighPart;
		endLittleBit = lEnd.LowPart;
		LARGE_INTEGER lBegin;
		lBegin.HighPart = bigBit;
		lBegin.LowPart = littleBit;
		double elapsed = (double)(lEnd.QuadPart - lBegin.QuadPart);
		result = elapsed / static_cast<double>(frequency.QuadPart);
	} else {
		endBigBit = clock();
		endLittleBit = 0;
		double elapsed = endBigBit - bigBit;
		result = elapsed / CLOCKS_PER_SEC;
	}
	if (reset) {
		bigBit = endBigBit;
		littleBit = endLittleBit;
	}
	return result;
}

//--- Trace Analysis Tool class functions ---
Tatl::Tatl() {
	nArrayEntries=0;
	tatlArray[0].elapsedTime = 0.f;
}

void Tatl::TatlReset() {
	nArrayEntries=0;
	tatlTimer.Duration(true);
};

void Tatl::TatlSetMark(const TCHAR* srcFile, int srcLine, const TCHAR* message) {
	if (nArrayEntries < TATLARRAYSIZE) {
		nArrayEntries++;
		tatlArray[nArrayEntries].elapsedTime = tatlTimer.Duration();
		_tcsncpy(tatlArray[nArrayEntries].srcFile, srcFile, TATLSRCFILESTRLEN);
		tatlArray[nArrayEntries].srcLine = srcLine;
		_tcsncpy(tatlArray[nArrayEntries].message, message, TATLSRCFILESTRLEN);
	};
};

void Tatl::TatlPrintAnalysis(TCHAR* strReturn, int nStrMax) {
//-- Prints out the Trace Analysis into a string variable 
	int len;
	int locStrMax = nStrMax;
	for (int i=1; i <= nArrayEntries; i++) {
		//len = _sntprintf(strReturn,locStrMax, TEXT("%s (%s - %d): %12.8f s diff: %.8f s\n"), tatlArray[i].message, tatlArray[i].srcFile, tatlArray[i].srcLine, tatlArray[i].elapsedTime, tatlArray[i].elapsedTime - tatlArray[i-1].elapsedTime);
		len = _sntprintf(strReturn,locStrMax, TEXT("%s%.3d: %14.8f s    diff: %12.8f s : %s \t(%s - %d) \n"), strReturn,i, tatlArray[i].elapsedTime, tatlArray[i].elapsedTime - tatlArray[i-1].elapsedTime, tatlArray[i].message, tatlArray[i].srcFile, tatlArray[i].srcLine);
		if (len > 0) {
			locStrMax = locStrMax - len; 
		} else {
			_tcsncat(strReturn,TEXT("break"),locStrMax);
			break;
		};
	}; //for
};

//-- Global Trace Analysis object --
Tatl	tatlAnalysis;
