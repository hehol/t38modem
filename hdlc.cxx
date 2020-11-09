/*
 * hdlc.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2003-2010 Vyacheslav Frolov
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
 * $Log: hdlc.cxx,v $
 * Revision 1.8  2010-10-08 06:06:39  vfrolov
 * Added diagErrorMask
 *
 * Revision 1.8  2010/10/08 06:06:39  vfrolov
 * Added diagErrorMask
 *
 * Revision 1.7  2009/11/09 19:12:57  vfrolov
 * Changed T38Engine to EngineBase
 *
 * Revision 1.6  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.5  2005/02/03 11:32:11  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.4  2004/07/06 16:07:24  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.3  2004/03/09 12:52:39  vfrolov
 * Fixed compile warning
 *
 * Revision 1.2  2004/02/17 13:23:07  vfrolov
 * Fixed MSVC compile errors
 *
 * Revision 1.1  2003/12/04 13:38:33  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>
#include "hdlc.h"
#include "enginebase.h"

///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
void HDLC::pack(const void *_pBuf, PINDEX count, PBoolean flag)
{
  WORD w = WORD((WORD)rawByte << 8);
  const BYTE *pBuf = (const BYTE *)_pBuf;

  for (PINDEX i = 0 ; i < count ; i++) {
    w |= *(pBuf++) & 0xFF;
    for (PINDEX j = 0 ; j < 8 ; j++) {
      w <<= 1;

      if (++rawByteLen == 8) {
        rawByte = BYTE(w >> 8);
        outData.PutData(&rawByte, 1);
        rawByteLen = 0;
      }

      if (!flag) {
        if (w & 0x100) {
          if (++rawOnes == 5) {
            w = WORD((w & 0xFF00) << 1 | (w & 0xFF));

            if (++rawByteLen == 8) {
              rawByte = BYTE(w >> 8);
              outData.PutData(&rawByte, 1);
              rawByteLen = 0;
            }
            rawOnes = 0;
          }
        }
        else
          rawOnes = 0;
      }
    }
  }
  rawByte = BYTE(w >> 8);
  if (flag)
    rawOnes = 0;
}

PBoolean HDLC::sync(BYTE b)
{
  WORD w = WORD(((WORD)rawByte << 8) | (b & 0xFF));
  PINDEX j = 8 - rawByteLen;

  w <<= j;

  for ( ; j <= 8 ; j++, w <<= 1) {
    if ((w >> 8) == 0x7E) {
      rawByte = BYTE(w >> j);
      rawByteLen = 8 - j;
      return TRUE;
    }
  }
  rawByte = BYTE(w >> j);
  rawByteLen = 7;
  return FALSE;
}

PBoolean HDLC::skipFlag(BYTE b)
{
  WORD w = WORD(((WORD)rawByte << 8) | (b & 0xFF));
  PINDEX j = 8 - rawByteLen;

  w <<= j;
  if ((w >> 8) == 0x7E) {
    rawByte = BYTE(w >> j);
    return TRUE;
  }

  return FALSE;
}

PBoolean HDLC::unpack(BYTE b)
{
  //myPTRACE(1, "T38Modem\tunpack det " << hex << (WORD)b);
  WORD w = WORD(((WORD)rawByte << 8) | (b & 0xFF));
  PINDEX j = 8 - rawByteLen;

  w <<= j;

  for ( ; j <= 8 ; j++, w <<= 1) {
    if ((w >> 8) == 0x7E) {
      rawByte = BYTE(w >> j);
      rawByteLen = 8 - j;
      return FALSE;
    }

    if (w & 0x8000) {
      hdlcChunk <<= 1;
      hdlcChunk |= 1;
      hdlcChunkLen++;
      rawOnes++;
    } else {
      if (rawOnes != 5) {
        hdlcChunk <<= 1;
        hdlcChunkLen++;
      }
      rawOnes = 0;
    }

    if (hdlcChunkLen == 24) {
      BYTE b = BYTE(hdlcChunk >> 16);
      outData.PutData(&b, 1);
      //myPTRACE(1, "T38Modem\tunpack put " << hex << (WORD)hdlcChunk);
      hdlcChunkLen = 16;
    }
  }
  rawByte = BYTE(w >> j);
  rawByteLen = 7;
  return TRUE;
}
///////////////////////////////////////////////////////////////
int HDLC::GetInData(void *pBuf, PINDEX count)
{
  if (!inData)
    return -1;

  int len = inData->GetData(pBuf, count);

  if (len > 0) {
    rawCount += len;
    lastChar = ((BYTE *)pBuf)[len - 1];
  } else
  if (len < 0) {
    inData = NULL;
    return -1;
  }
  return len;
}

int HDLC::GetRawData(void *_pBuf, PINDEX count)
{
  BYTE *pBuf = (BYTE *)_pBuf;

  switch (inDataType) {
  case EngineBase::dtHdlc:
    break;
  case EngineBase::dtRaw:
    return GetInData(pBuf, count);
  default:
    return 0;
  }

  int outLen = outData.GetData(pBuf, count);

  if (outLen < 0)
    return -1;

  int len = 0;

  if (outLen > 0) {
    pBuf += outLen;
    count -= outLen;
    len += outLen;
  }

  do {
    if (inData) {
      BYTE Buf[256];
      int inLen = inData->GetData(Buf, sizeof(Buf));

      if (inLen < 0) {
        Buf[0] = BYTE(fcs >> 8);
        Buf[1] = BYTE(fcs & 0xFF);
        if (inData->GetDiag() & EngineBase::diagErrorMask)
          Buf[0]++;
        pack(Buf, 2);
        pack("\x7e\x7e", 2, TRUE);
        outData.PutEof();
        inData = NULL;
      }
      else
      if (inLen > 0) {
        lastChar = Buf[inLen - 1];
        fcs.build(Buf, inLen);
        pack(Buf, inLen);
      }
      else
        break;
    }

    int outLen = outData.GetData(pBuf, count);

    if (outLen < 0) {
      if (len > 0)
        break;
      return -1;
    }
    else
    if (outLen > 0) {
      pBuf += outLen;
      count -= outLen;
      len += outLen;
    }
  } while (count);

  if (len > 0)
    rawCount += len;

  return len;
}

int HDLC::GetHdlcData(void *_pBuf, PINDEX count)
{
  int len;
  BYTE *pBuf = (BYTE *)_pBuf;

  switch (inDataType) {
  case EngineBase::dtHdlc:
    len = GetInData(pBuf, count);
    if (len > 0)
      fcs.build(pBuf, len);
    return len;
  case EngineBase::dtRaw:
    break;
  default:
    return 0;
  }

  if (!inData || hdlcState == stEof) {
    len = outData.GetData(pBuf, count);
    if (len > 0)
      fcs.build(pBuf, len);
  }
  else
  if (!count && hdlcState == stData) {
    if (outData.GetData(NULL, 0) < 0 || inData->GetData(NULL, 0) < 0)
      return -1;
    return 0;
  } else {
    BYTE b;
    int res;
    len = 0;

    for ( ; (res = inData->GetData(&b, 1)) != 0 ; ) {
      if (res < 0) {
        outData.PutEof();
        inData = NULL;
        hdlcState = stEof;
        //myPTRACE(1, "T38Modem\thdlcState=stEof EOF");
      }
      else {
        lastChar = b;
        rawCount++;
        switch (hdlcState) {
        case stSync:
          if (sync(b)) {
            hdlcState = stSkipFlags;
            //myPTRACE(1, "T38Modem\thdlcState=stSkipFlags " << hex << (int)b);
          }
          break;
        case stSkipFlags:
          if (skipFlag(b))
            break;
          hdlcState = stData;
          //myPTRACE(1, "T38Modem\thdlcState=stData " << hex << (int)b);
        case stData:
          if (!unpack(b)) {
            outData.PutEof();
            hdlcState = stEof;
            //myPTRACE(1, "T38Modem\thdlcState=stEof " << hex << (int)b);
          }
          break;
        default:
          myPTRACE(1, "T38Modem\tHDLC::GetHdlcData(): unexpected hdlcState=" << hdlcState);
        }
      }

      if (hdlcState == stEof || hdlcState == stData) {
        int outLen = outData.GetData(pBuf, count);

        if (outLen < 0) {
          if (len > 0)
            break;
          return -1;
        }
        else
        if (outLen > 0) {
          fcs.build(pBuf, outLen);
          pBuf += outLen;
          count -= outLen;
          len += outLen;
        }
        if (!count || hdlcState == stEof)
          break;
      }
    }
  }

  return len;
}
///////////////////////////////////////////////////////////////
HDLC::HDLC() :
    inDataType(EngineBase::dtNone), outDataType(EngineBase::dtNone),
    inData(NULL), lastChar(-1), rawCount(0),
    rawByteLen(0), rawOnes(0), hdlcState(stEof)
{
}

PBoolean HDLC::isFcsOK()
{
  if (hdlcChunkLen != 16) {
    myPTRACE(1, "T38Modem\tisFcsOK(): hdlcChunkLen(" << hdlcChunkLen << ") != 16");
    return FALSE;
  }

  if ((WORD)hdlcChunk != fcs) {
    myPTRACE(1, "T38Modem\tisFcsOK(): hdlcChunk(" << hex << (WORD)hdlcChunk << ") != fcs(" << (WORD)fcs << ")");
    return FALSE;
  }

  return TRUE;
}

void HDLC::PutRawData(DataStream *_inData)
{
  inDataType = EngineBase::dtRaw;
  inData = _inData;
  lastChar = -1;
}

void HDLC::PutHdlcData(DataStream *_inData)
{
  inDataType = EngineBase::dtHdlc;
  inData = _inData;
  lastChar = -1;
}

void HDLC::GetRawStart(PINDEX flags)
{
  outDataType = EngineBase::dtRaw;
  outData.Clean();
  fcs = FCS();
  if (inDataType == EngineBase::dtHdlc) {
    while (flags--)
      pack("\x7e", 1, TRUE);
  }
}

void HDLC::GetHdlcStart(PBoolean sync)
{
  outDataType = EngineBase::dtHdlc;
  outData.Clean();
  fcs = FCS();
  if (inDataType == EngineBase::dtRaw) {
    hdlcChunkLen = 0;
    rawOnes = 0;
    hdlcState = sync ? stSync : stSkipFlags;
  } else {
    if (!sync)
      rawCount += 4;	// count FCS, flags, zeros
  }
  //myPTRACE(1, "T38Modem\thdlcState=" << (sync ? "stSync" : "stSkipFlags") << " START");
}

int HDLC::GetData(void *pBuf, PINDEX count)
{
  switch (outDataType) {
  case EngineBase::dtHdlc:
    return GetHdlcData(pBuf, count);
  case EngineBase::dtRaw:
    return GetRawData(pBuf, count);
  default:
    myPTRACE(1, "T38Modem\tHDLC::GetData bad outDataType=" << outDataType);
  }
  return -1;
}
///////////////////////////////////////////////////////////////

