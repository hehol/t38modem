/*
 * $Id: dle.h,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: dle.h,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
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

