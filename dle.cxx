/*
 * dle.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2007 Vyacheslav Frolov
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
 * $Log: dle.cxx,v $
 * Revision 1.9  2007-03-22 16:26:04  vfrolov
 * Fixed compiler warnings
 *
 * Revision 1.9  2007/03/22 16:26:04  vfrolov
 * Fixed compiler warnings
 *
 * Revision 1.8  2004/10/27 13:18:03  vfrolov
 * Fixed compiler warning
 *
 * Revision 1.7  2004/10/22 13:34:20  vfrolov
 * Fixed buffer overflow
 *
 * Revision 1.6  2004/07/06 16:07:24  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.5  2003/12/04 13:22:22  vfrolov
 * Removed ambiguous isEof()
 * Improved memory usage in DataStream
 * Fixed myPTRACE
 *
 * Revision 1.4  2003/01/08 16:37:22  vfrolov
 * Changed class DataStream:
 *   members moved to private section and added isEof()
 *   added threshold and isFull()
 *
 * Revision 1.3  2002/04/19 14:29:26  vfrolov
 * Added Copyright header
 *
 * Revision 1.2  2002/01/10 06:10:02  craigs
 * Added MPL header
 *
 * 
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 * 
 */

#include <ptlib.h>
#include "dle.h"

///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
static BYTE BitRevTable[256];

static BOOL initBitRevTable()
{
  for( unsigned i = 0 ; i < sizeof(BitRevTable) ; i++ ) {
    unsigned in = i, out = 0;
    for( int j = 0 ; j < 8 ; j++ ) {
      out <<= 1;
      out += in & 1;
      in >>= 1;
    }
    BitRevTable[i] = (BYTE)out;
  }
  return TRUE;
}

static const BOOL ___InitBitRevTable = initBitRevTable();
///////////////////////////////////////////////////////////////
enum {
  ETX = 0x03,
  DLE = 0x10,
};
///////////////////////////////////////////////////////////////
int DLEData::PutDleData(const void *pBuf, PINDEX count)
{
  if (PutData(NULL, 0) < 0)
    return -1;

  PINDEX cRest = count;
  const BYTE *p = (const BYTE *)pBuf;
  
  while( cRest > 0 ) {
    const BYTE *pScan = p;
    PINDEX cScan = cRest;
    
    if( dle ) {
      dle = FALSE;
      if( *p == ETX ) {
        PutEof();
        cRest--;
        break;
      }
      pScan++;
      cScan--;
    }
    
    const BYTE *pDle = (const BYTE *)memchr(pScan, DLE, cScan);
    
    PINDEX cPut;
    PINDEX cDone;
  
    if( pDle ) {
      dle = TRUE;
      cPut = PINDEX(pDle - p);
      cDone = cPut + 1;	// skip DLE
    } else {
      cDone = cPut = cRest;
    }
  
    if( cPut ) {
      if( bitRev ) {
        BYTE tmp[1024];
        while( cPut ) {
          PINDEX cTmp = cPut > 1024 ? 1024 : cPut;
          for( PINDEX i = 0 ; i < cTmp ; i++ ) {
            tmp[i] = BitRevTable[p[i]];
          }
          PutData(tmp, cTmp);
          cPut -= cTmp;
        }
      } else {
        PutData(p, cPut);
      }
    }
    p += cDone;
    cRest -= cDone;
  }
  
  return count - cRest;
}

int DLEData::GetDleData(void *pBuf, PINDEX count)
{
  if (recvEtx)
    return -1;

  BYTE *p = (BYTE *)pBuf;
  int done;

  for (done = 0 ; (count - done) >= 4 ; done = int(p - (BYTE *)pBuf)) {
    PINDEX cGet = (count - done - 2) / 2;
    BYTE tmp[1024];

    if (cGet > (PINDEX)sizeof(tmp))
      cGet = sizeof(tmp);

    switch( cGet = GetData(tmp, cGet) ) {
      case -1:
        *p++ = DLE;
        *p++ = ETX;
        recvEtx = TRUE;
        return int(p - (BYTE *)pBuf);
      case 0:
        return int(p - (BYTE *)pBuf);
      default:
        for( PINDEX i = 0 ; i < cGet ; i++ ) {
          BYTE b = bitRev ? BitRevTable[tmp[i]] : tmp[i];
          if( b == DLE )
            *p++ = DLE;
          *p++ = b;
        }
    }
  }
  return done;
}
///////////////////////////////////////////////////////////////

