// Elapse Timer 
/** @file ElapseTimer.h
 ** Interface to ElapseTimer facilities.
 ** Implemented ElapseTimer.cpp Code stolen from Scintilla Platform.h and PlatWX.cxx
 **/
// Copyright 1998-2009 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#ifndef ELAPSETIMER_H
#define ELAPSETIMER_H

#include <windows.h>	//for LARGE_INTEGER, and QueryPerformanceFrequency() etc.
#include <time.h>	//for clock()
#include <tchar.h>		//for Tatl strings


/**
 * ElapsedTime class definition
 */
class ElapseTimer {
friend class Tatl;
	long bigBit;
	long littleBit;
public:
	ElapseTimer();
	double Duration(bool reset=false);
};

/**
 * Trace Analysis Tool class definition
 */
class Tatl {
private:
#define TATLSRCFILESTRLEN	300
#define TATLMESSAGESTRLEN	1024
typedef struct {
	TCHAR	srcFile[TATLSRCFILESTRLEN];
	int		srcLine;
	TCHAR	message[TATLMESSAGESTRLEN];
	double	elapsedTime;
} TatlArray_t;

#define TATLARRAYSIZE	100
short	nArrayEntries;
TatlArray_t	tatlArray[TATLARRAYSIZE];

ElapseTimer		tatlTimer;

public:
	Tatl();
	void TatlReset();
	void TatlSetMark(const TCHAR* srcFile, int srcLine, const TCHAR* message);
void Tatl::TatlPrintAnalysis(TCHAR* strReturn, int nStrMax);
}; //-- class Tatl --

//-- Global Trace Analysis object --
extern Tatl	tatlAnalysis;


#endif  //ELAPSETIMER_H