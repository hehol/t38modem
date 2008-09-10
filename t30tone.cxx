/*
 * t30tone.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2002-2008 Vyacheslav Frolov
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
 * Revision 1.6  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.6  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.5  2007/03/23 10:14:36  vfrolov
 * Implemented voice mode functionality
 *
 * Revision 1.4  2005/02/03 11:32:12  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.3  2004/07/06 16:07:24  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.2  2002/06/07 06:25:15  robertj
 * Added math.h for use of sin() function.
 * Fixed GNU warnings.
 *
 * Revision 1.1  2002/04/30 10:59:07  vfrolov
 * Initial revision
 * 
 */

#include <ptlib.h>
#include "pmutils.h"
#include "t30tone.h"
#include <math.h>


///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
typedef	PInt16			SIMPLE_TYPE;

#define TWO_PI			(3.1415926535897932384626433832795029L*2)
#define BYTES_PER_SIMPLE	sizeof(SIMPLE_TYPE)
///////////////////////////////////////////////////////////////
#define CNG_MAX_DIV		100
#define CNG_AMPLITUDE		5000
#define CNG_ON_MSEC		500
#define CNG_ON_BYTES		(((SIMPLES_PER_SEC*CNG_ON_MSEC)/1000)*BYTES_PER_SIMPLE)
#define CNG_OFF_MSEC		3000
#define CNG_OFF_BYTES		(((SIMPLES_PER_SEC*CNG_OFF_MSEC)/1000)*BYTES_PER_SIMPLE)
#define CNG_SIMPLES_PER_REPEATE	(SIMPLES_PER_SEC/CNG_MAX_DIV)

static BYTE CngTone[CNG_SIMPLES_PER_REPEATE*BYTES_PER_SIMPLE];

static PBoolean initCngTone()
{
  for( size_t i = 0 ; i < sizeof(CngTone)/BYTES_PER_SIMPLE ; i++ ) {
    double Sin = sin(double((CNG_HZ*TWO_PI*i)/SIMPLES_PER_SEC));
    ((SIMPLE_TYPE *)CngTone)[i] = (SIMPLE_TYPE)(Sin * CNG_AMPLITUDE);
  }
  return TRUE;
}

static const PBoolean ___InitCngTone = initCngTone();
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
#define tone_trace(cng_filter_buf, power, cng_power, cng_power_norm)

#ifndef tone_trace
static void tone_trace(const long *cng_filter_buf, long power, long cng_power, long cng_power_norm)
{
  PString str;

  for (PINDEX i = 0 ; i < CNG_FILTER_BUF_LEN ; i++) {
    str += PString(cng_filter_buf[i]);
    str += " ";
  }

  myPTRACE(1, str << " pw=" << power << " cng_pw=" << cng_power << " norm=" << cng_power_norm);
}
#endif

enum {
  cng_phase_off_head,
  cng_phase_on,
  cng_phase_off_tail,
};

T30ToneDetect::T30ToneDetect()
{
  memset(cng_filter_buf, 0, sizeof(cng_filter_buf));
  index = 0;
  power = 0;

  cng_on_count = 0;
  cng_off_count = 0;
  cng_phase = cng_phase_off_head;
}

#define CNG_FILTER_CHUNK_LEN	20
#define CNG_ON_CHUNKS_MIN	(((CNG_HZ*CNG_ON_MSEC)*60)/((CNG_FILTER_CHUNK_LEN*1000)*100))
#define CNG_ON_CHUNKS_MAX	(((CNG_HZ*CNG_ON_MSEC)*140)/((CNG_FILTER_CHUNK_LEN*1000)*100))
#define CNG_OFF_CHUNKS_MIN	(((CNG_HZ*CNG_OFF_MSEC)*30)/((CNG_FILTER_CHUNK_LEN*1000)*100))

PBoolean T30ToneDetect::Write(const void * buffer, PINDEX len)
{
  PBoolean detected = FALSE;

  SIMPLE_TYPE *pBuf = (SIMPLE_TYPE *)buffer;
  len /= BYTES_PER_SIMPLE;

  while (len) {
    PINDEX iBuf = 0;

    // filter CNG and calculate total power

    PINDEX index0 = index;

    while (index < CNG_FILTER_BUF_LEN*CNG_FILTER_CHUNK_LEN) {
      iBuf = ((index - index0)*SIMPLES_PER_SEC)/(CNG_FILTER_BUF_LEN*CNG_HZ);

      if (iBuf >= len)
        return detected;

      SIMPLE_TYPE pw = pBuf[iBuf];

      cng_filter_buf[index%CNG_FILTER_BUF_LEN] += pw;

      if (pw < 0)
        power -= pw;
      else
        power += pw;

      index++;
    }

    // calculate CNG power

    PINDEX i;
    long cng_power_cur = 0;

    for (i = 0 ; i < CNG_FILTER_BUF_LEN/2 ; i++)
      cng_power_cur += cng_filter_buf[i];

    for (; i < CNG_FILTER_BUF_LEN ; i++)
      cng_power_cur -= cng_filter_buf[i];

    long cng_power;

    if (cng_power_cur < 0)
      cng_power = -cng_power_cur;
    else
      cng_power = cng_power_cur;

    for (i = 1 ; i < CNG_FILTER_BUF_LEN/2 ; i++) {
      cng_power_cur -= cng_filter_buf[i]*2;
      cng_power_cur += cng_filter_buf[i + CNG_FILTER_BUF_LEN/2]*2;

      if (cng_power_cur < 0) {
        if (cng_power < -cng_power_cur)
          cng_power = -cng_power_cur;
      } else {
        if (cng_power < cng_power_cur)
          cng_power = cng_power_cur;
      }
    }

    long cng_power_norm = cng_power * 1000;

    if (power > 1)
      cng_power_norm /= power;

    if (cng_on_count > 1)
      cng_power_norm = (cng_power_norm*140)/100;
    if (cng_off_count > 1)
      cng_power_norm = (cng_power_norm*80)/100;

    if (cng_power_norm > 500) {
      cng_on_count++;

      switch (cng_phase) {
        case cng_phase_off_head:
          if (cng_off_count >= CNG_OFF_CHUNKS_MIN)
            cng_phase = cng_phase_on;
          else
            cng_phase = cng_phase_off_head;
          break;

        case cng_phase_on:
          break;

        case cng_phase_off_tail:
          cng_phase = cng_phase_off_head;
          break;

        default:
          cng_phase = cng_phase_off_head;
      }

      if (cng_off_count) {
        myPTRACE(2, "cng_off_count=" << cng_off_count);
        cng_off_count = 0;
      }

      tone_trace(cng_filter_buf, power, cng_power, cng_power_norm);
    } else {
      cng_off_count++;

      switch (cng_phase) {
        case cng_phase_off_head:
          break;

        case cng_phase_on:
          if (cng_on_count >= CNG_ON_CHUNKS_MIN && cng_on_count <= CNG_ON_CHUNKS_MAX)
            cng_phase = cng_phase_off_tail;
          else
            cng_phase = cng_phase_off_head;
          break;

        case cng_phase_off_tail:
          if (cng_off_count >= CNG_OFF_CHUNKS_MIN) {
            myPTRACE(1, "Detected CNG");
            cng_phase = cng_phase_off_head;
            detected = TRUE;
          }
          break;

        default:
          cng_phase = cng_phase_off_head;
      }

      if (cng_on_count) {
        myPTRACE(2, "cng_on_count=" << cng_on_count);
        tone_trace(cng_filter_buf, power, cng_power, cng_power_norm);

        cng_on_count = 0;
      }
    }

    // reatart filter

    memset(cng_filter_buf, 0, sizeof(cng_filter_buf));
    index = 0;
    power = 0;

    // next chunk

    iBuf++;
    pBuf += iBuf;
    len -= iBuf;
  }

  return detected;
}
///////////////////////////////////////////////////////////////

