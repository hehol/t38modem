/*
 * tone_gen.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2010 Vyacheslav Frolov
 *
 * t38modem Project
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
 * The Original Code is t38modem.
 *
 * The Initial Developer of the Original Code is Vyacheslav Frolov
 *
 * Contributor(s):
 *
 * $Log: tone_gen.cxx,v $
 * Revision 1.1  2010-10-12 16:43:00  vfrolov
 * Initial revision
 *
 * Revision 1.1  2010/10/12 16:43:00  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>
#include <math.h>
#include "pmutils.h"
#include "tone_gen.h"

///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
#define CNG_HZ                    1100
#define CNG_MAX_DIV               100
#define CNG_AMPLITUDE             5000
#define CNG_ON_MSEC               500
#define CNG_OFF_MSEC              3000
///////////////////////////////////////////////////////////////
#define CED_HZ                    2100
#define CED_MAX_DIV               100
#define CED_AMPLITUDE             5000
#define CED_ON_MSEC               3500
///////////////////////////////////////////////////////////////
#define RING_RU_HZ                425
#define RING_RU_MAX_DIV           25
#define RING_RU_AMPLITUDE         5000
#define RING_RU_ON_MSEC           1000
#define RING_RU_OFF_MSEC          4000
///////////////////////////////////////////////////////////////
#define BUSY_RU_HZ                425
#define BUSY_RU_MAX_DIV           25
#define BUSY_RU_AMPLITUDE         5000
#define BUSY_RU_ON_MSEC           350
#define BUSY_RU_OFF_MSEC          350
///////////////////////////////////////////////////////////////
typedef	PInt16                    SIMPLE_TYPE;
#define BYTES_PER_SIMPLE          sizeof(SIMPLE_TYPE)
#define SIMPLES_PER_SEC           8000
///////////////////////////////////////////////////////////////
#define TWO_PI                    (3.1415926535897932384626433832795029L*2)

static PBoolean InitTone(const void *, BYTE *pTone, size_t amount, unsigned hz, SIMPLE_TYPE ampl)
{
  for( size_t i = 0 ; i < amount/BYTES_PER_SIMPLE ; i++ ) {
    double Sin = sin(double((hz*TWO_PI*i)/SIMPLES_PER_SEC));
    ((SIMPLE_TYPE *)pTone)[i] = (SIMPLE_TYPE)(Sin * ampl);
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////
#define MSEC2BYTES(ms)            (((SIMPLES_PER_SEC*(ms))/1000)*BYTES_PER_SIMPLE)
#define MAX_DIV2BYTES(maxdiv)     ((SIMPLES_PER_SEC/(maxdiv))*BYTES_PER_SIMPLE)
///////////////////////////////////////////////////////////////
ToneGenerator::ToneGenerator(ToneGenerator::ToneType tt)
  : type(tt)
  , index(0)
{
  switch(tt) {
    case ttCng: {
      static BYTE toneOn[MAX_DIV2BYTES(CNG_MAX_DIV)];
      static const PBoolean initTone = InitTone(&initTone, toneOn, sizeof(toneOn), CNG_HZ, CNG_AMPLITUDE);

      bytesOn = MSEC2BYTES(CNG_ON_MSEC);
      bytesOff = MSEC2BYTES(CNG_OFF_MSEC);
      pToneOn = toneOn;
      bytesToneOn = sizeof(toneOn);

      // begin with 1 sec silence
      #if CNG_OFF_MSEC < 1000
        #error CNG_OFF_MSEC < 1000
      #endif
      index = bytesOn + bytesOff - MSEC2BYTES(1000);
      break;
    }
    case ttCed: {
      static BYTE toneOn[MAX_DIV2BYTES(CED_MAX_DIV)];
      static const PBoolean initTone = InitTone(&initTone, toneOn, sizeof(toneOn), CED_HZ, CED_AMPLITUDE);

      bytesOn = MSEC2BYTES(CED_ON_MSEC);
      bytesOff = (P_MAX_INDEX/BYTES_PER_SIMPLE)*BYTES_PER_SIMPLE - bytesOn;
      pToneOn = toneOn;
      bytesToneOn = sizeof(toneOn);

      // begin with 200 ms silence
      index = bytesOn + bytesOff - MSEC2BYTES(200);
      break;
    }
    case ttRing: {
      static BYTE toneOn[MAX_DIV2BYTES(RING_RU_MAX_DIV)];
      static const PBoolean initTone = InitTone(&initTone, toneOn, sizeof(toneOn), RING_RU_HZ, RING_RU_AMPLITUDE);

      bytesOn = MSEC2BYTES(RING_RU_ON_MSEC);
      bytesOff = MSEC2BYTES(RING_RU_OFF_MSEC);
      pToneOn = toneOn;
      bytesToneOn = sizeof(toneOn);
      break;
    }
    case ttBusy: {
      static BYTE toneOn[MAX_DIV2BYTES(BUSY_RU_MAX_DIV)];
      static const PBoolean initTone = InitTone(&initTone, toneOn, sizeof(toneOn), BUSY_RU_HZ, BUSY_RU_AMPLITUDE);

      bytesOn = MSEC2BYTES(BUSY_RU_ON_MSEC);
      bytesOff = MSEC2BYTES(BUSY_RU_OFF_MSEC);
      pToneOn = toneOn;
      bytesToneOn = sizeof(toneOn);
      break;
    }
    default:
      bytesOn = 0;
      bytesOff = (P_MAX_INDEX/BYTES_PER_SIMPLE)*BYTES_PER_SIMPLE - bytesOn;
      pToneOn = NULL;
      bytesToneOn = 0;
  }
}

void ToneGenerator::Read(void * buffer, PINDEX amount)
{
  BYTE *pBuf = (BYTE *)buffer;
  PINDEX bytesOnOff = bytesOn + bytesOff;

  while(amount) {
    if (index >= bytesOnOff)
      index = 0;

    PINDEX len;

    if (bytesOn > index) {
      PINDEX i = index % bytesToneOn;

      len = bytesToneOn - i;

      if (len > amount)
        len = amount;

      memcpy(pBuf, pToneOn + i, len);
    } else {
      len = bytesOnOff - index;

      if (len > amount)
        len = amount;

      memset(pBuf, 0, len);
    }

    pBuf += len;
    amount -= len;
    index += len;
  }
}
///////////////////////////////////////////////////////////////

