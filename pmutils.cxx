/*
 * pmutils.cxx
 *
 * T38FAX Pseudo Modem
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
 * $Log: pmutils.cxx,v $
 * Revision 1.2  2002-01-10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.2  2002/01/10 06:10:03  craigs
 * Added MPL header
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

