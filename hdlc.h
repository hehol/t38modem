/*
 * hdlc.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2003 Vyacheslav Frolov
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
 * $Log: hdlc.h,v $
 * Revision 1.1  2003-12-04 13:38:39  vfrolov
 * Initial revision
 *
 * Revision 1.1  2003/12/04 13:38:39  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _HDLC_H
#define _HDLC_H

#include "fcs.h"

///////////////////////////////////////////////////////////////
class HDLC
{
  public:
    HDLC();
    void PutRawData(DataStream *_inData);
    void PutHdlcData(DataStream *_inData);
    void GetRawStart(PINDEX flags = 0);
    void GetHdlcStart(BOOL sync);
    int GetData(void *pBuf, PINDEX count);
    BOOL isFcsOK();
    int getLastChar() { return lastChar; }
    PINDEX getRawCount() { return rawCount; }
    void resetRawCount() { rawCount = 0; }

  private:
    void pack(const void *pBuf, PINDEX count, BOOL flag = FALSE);
    BOOL sync(BYTE b);
    BOOL skipFlag(BYTE b);
    BOOL unpack(BYTE b);
    int GetInData(void *pBuf, PINDEX count);
    int GetRawData(void *pBuf, PINDEX count);
    int GetHdlcData(void *pBuf, PINDEX count);

    int inDataType;
    int outDataType;
    DataStream *inData;
    DataStream outData;
    FCS fcs;
    int lastChar;
    PINDEX rawCount;

    BYTE rawByte;
    int rawByteLen;
    int rawOnes;
    DWORD hdlcChunk;
    int hdlcChunkLen;

    enum {
      stEof,
      stSync,
      stSkipFlags,
      stData,
    } hdlcState;
};
///////////////////////////////////////////////////////////////

#endif  // _HDLC_H

