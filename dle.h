/*
 * dle.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2002 Vyacheslav Frolov
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
 * $Log: dle.h,v $
 * Revision 1.3  2002-04-19 14:29:30  vfrolov
 * Added Copyright header
 *
 * Revision 1.3  2002/04/19 14:29:30  vfrolov
 * Added Copyright header
 *
 * Revision 1.2  2002/01/10 06:10:02  craigs
 * Added MPL header
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _DLE_H
#define _DLE_H

#include "pmutils.h"

///////////////////////////////////////////////////////////////
class DLEData : public DataStream
{
    PCLASSINFO(DLEData, DataStream);
  public:
    DLEData() : dle(FALSE), recvEtx(FALSE), bitRev(FALSE) { }
    
    int PutDleData(const void *pBuf, PINDEX count);
    int GetDleData(void *pBuf, PINDEX count);
    
    void BitRev(BOOL _bitRev) { bitRev = _bitRev; }
    
    virtual void Clean() {
      DataStream::Clean();
      dle = recvEtx = bitRev = FALSE;
    }
  protected:
    
    BOOL dle;
    BOOL recvEtx;
    BOOL bitRev;
};
///////////////////////////////////////////////////////////////

#endif  // _DLE_H

