/*
		スペアナくん１号

		オーバーレイしてよい部分。

		By Ken/ichiro(OPA)
*/

#include "stdincl.h"
#pragma hdrstop
#include "opa.h"
#include "ovrstr.h"
#include "speana.h"

#pragma option -zEOSPEANA2_TEXT -zFCODE

#include "o\speanag.dmp"

void drawBackground()
{
	uchar
		pal[sizeof pdata];

	memcpy(pal,FarObj(pdata),sizeof pal);
	gv.setPal(pal);

	ExtMem<uchar>
		grph;

	Far2extMemCopy(grph,gdata);

	while(BitMap::restore(grph))
		;
}

