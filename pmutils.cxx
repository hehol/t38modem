/*
 * $Id: pmutils.cxx,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: pmutils.cxx,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include "pmutils.h"

#define new PNEW

///////////////////////////////////////////////////////////////
ModemThread::ModemThread()
  : PThread(30000,
            NoAutoDeleteThread,
            NormalPriority),
    stop(FALSE),
    childstop(FALSE)
{
}
///////////////////////////////////////////////////////////////
ModemThreadChild::ModemThreadChild(ModemThread &_parent)
  : parent(_parent)
{
}

void ModemThreadChild::SignalStop()
{
  ModemThread::SignalStop();
  parent.SignalChildStop();
}
///////////////////////////////////////////////////////////////
int DataStream::PutData(const void *pBuf, PINDEX count)
{
  if( eof ) return -1;
  data.Concatenate(PBYTEArray((const BYTE *)pBuf, count));
  return count;
}

int DataStream::GetData(void *pBuf, PINDEX count)
{
  if( done >= data.GetSize() ) {
    if( eof )
      return -1;
    else
      return 0;
  }
  
  PINDEX rest = data.GetSize() - done;
  if( count > rest )
    count = rest;
  
  memcpy(pBuf, (const BYTE *)data + done, count);
  
  done += count;
  
  if( done >= data.GetSize() )
    CleanData();
  
  return count;
}
///////////////////////////////////////////////////////////////

