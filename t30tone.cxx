/*
 * t30tone.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2002 Vyacheslav Frolov
 *
 * Open H323 Project
 *
 * The contents of this file are subject to the Mozilla Public License
 * Version 1.0 (the "License"); you may not use this file except in
 * compliance with the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS"
 * basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
 * the License for the specific language governing rights and limitations
 * under the License.
 *
 * The Original Code is Open H323 Library.
 *
 * The Initial Developer of the Original Code is Vyacheslav Frolov
 *
 * Contributor(s): Equivalence Pty ltd
 *
 * $Log: t30tone.cxx,v $
 * Revision 1.2  2002-06-07 06:25:15  robertj
 * Added math.h for use of sin() function.
 * Fixed GNU warnings.
 *
 * Revision 1.2  2002/06/07 06:25:15  robertj
 * Added math.h for use of sin() function.
 * Fixed GNU warnings.
 *
 * Revision 1.1  2002/04/30 10:59:07  vfrolov
 * Initial revision
 *
 * 
 */

#include "t30tone.h"
#include <math.h>


///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
typedef	PInt16			SIMPLE_TYPE;

#define TWO_PI			(3.1415926535897932384626433832795029L*2)
#define SIMPLES_PER_SEC		8000
#define BYTES_PER_SIMPLE	sizeof(SIMPLE_TYPE)
///////////////////////////////////////////////////////////////
#define CNG_HZ			1100
#define CNG_AMPLITUDE		5000
#define CNG_ON_MSEC		500
#define CNG_ON_BYTES		(((SIMPLES_PER_SEC*CNG_ON_MSEC)/1000)*BYTES_PER_SIMPLE)
#define CNG_OFF_MSEC		3000
#define CNG_OFF_BYTES		(((SIMPLES_PER_SEC*CNG_OFF_MSEC)/1000)*BYTES_PER_SIMPLE)
#define CNG_SIMPLES_PER_REPEATE	((SIMPLES_PER_SEC*(CNG_HZ/100))/CNG_HZ)

static BYTE CngTone[CNG_SIMPLES_PER_REPEATE*BYTES_PER_SIMPLE];

static BOOL initCngTone()
{
  for( size_t i = 0 ; i < sizeof(CngTone)/BYTES_PER_SIMPLE ; i++ ) {
    double Sin = sin((CNG_HZ*TWO_PI*i)/SIMPLES_PER_SEC);
    ((SIMPLE_TYPE *)CngTone)[i] = (SIMPLE_TYPE)(Sin * CNG_AMPLITUDE);
  }
  return TRUE;
}

static const BOOL ___InitCngTone = initCngTone();
///////////////////////////////////////////////////////////////
T30Tone::T30Tone(T30Tone::Type _type)
{
  type = _type;

  switch(type) {
    case cng:
      // begin with 1 sec silence
      index = ((SIMPLES_PER_SEC*(CNG_ON_MSEC+CNG_OFF_MSEC-1000))/1000)*BYTES_PER_SIMPLE;
      break;
    default:
      index = 0;
  }
}

void T30Tone::Read(void * buffer, PINDEX amount)
{
  BYTE *pBuf = (BYTE *)buffer;

  switch(type) {
    case cng:
      while(amount) {
        if (index >= (PINDEX)(CNG_ON_BYTES + CNG_OFF_BYTES))
          index = 0;

        PINDEX len;

        if ((PINDEX)CNG_ON_BYTES > index) {
          PINDEX i = index % sizeof(CngTone);
          len = sizeof(CngTone) - i;
          if (len > amount)
            len = amount;
          memcpy(pBuf, CngTone + i, len);
        } else {
          len = CNG_ON_BYTES + CNG_OFF_BYTES - index;
          if (len > amount)
            len = amount;
          memset(pBuf, 0, len);
        }
        pBuf += len;
        amount -= len;
        index += len;
      }
      break;
    default:
      memset(pBuf, 0, amount);
      index = (index + amount) % BYTES_PER_SIMPLE;
  }
}
///////////////////////////////////////////////////////////////

