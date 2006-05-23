/*
 * pmodem.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2004 Vyacheslav Frolov
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
 * $Log: pmodem.cxx,v $
 * Revision 1.8  2006-05-23 14:48:54  vfrolov
 * Fixed race condition (reported by Tamas)
 *
 * Revision 1.8  2006/05/23 14:48:54  vfrolov
 * Fixed race condition (reported by Tamas)
 *
 * Revision 1.7  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 *
 * Revision 1.6  2004/05/09 07:46:11  csoutheren
 * Updated to compile with new PIsDescendant function
 *
 * Revision 1.5  2002/05/15 16:17:44  vfrolov
 * Implemented per modem routing for I/C calls
 *
 * Revision 1.4  2002/03/05 12:35:52  vfrolov
 * Added Copyright header
 * Changed class hierarchy
 *   PseudoModem is abstract
 *   PseudoModemBody is child of PseudoModem
 *   Added PseudoModemQ::CreateModem() to create instances
 * Some OS specific code moved from pmodem.cxx to pty.cxx
 *
 * Revision 1.3  2002/02/08 12:58:22  vfrolov
 * Defined Linux and FreeBSD patterns in ttyPattern().
 *
 * Revision 1.2  2002/01/10 06:10:02  craigs
 * Added MPL header
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include <ptlib.h>
#include "pmodem.h"
#include "drivers.h"

#define new PNEW

///////////////////////////////////////////////////////////////
PLIST(_PseudoModemList, PseudoModem);

class PseudoModemList : protected _PseudoModemList
{
    PCLASSINFO(PseudoModemList, _PseudoModemList);
  public:
    PINDEX Append(PseudoModem *modem);
    PseudoModem *Find(const PString &modemToken) const;
  protected:
    PseudoModem *_Find(const PString &modemToken) const;
    PMutex Mutex;
};

PINDEX PseudoModemList::Append(PseudoModem *modem)
{
  PWaitAndSignal mutexWait(Mutex);

  if (_Find(modem->modemToken())) {
    myPTRACE(1, "PseudoModemList::Append can't add " << modem->ptyName() << " to modem list");
    delete modem;
    return P_MAX_INDEX;
  }

  PINDEX i = _PseudoModemList::Append(modem);

  myPTRACE(3, "PseudoModemList::Append " << modem->ptyName() << " (" << i << ") OK");

  return i;
}

PseudoModem *PseudoModemList::Find(const PString &modemToken) const
{
  PWaitAndSignal mutexWait(Mutex);
  return _Find(modemToken);
}

PseudoModem *PseudoModemList::_Find(const PString &modemToken) const
{
  PObject *object;

  for (PINDEX i = 0 ; (object = GetAt(i)) != NULL ; i++) {
    PAssert(PIsDescendant(object, PseudoModem), PInvalidCast);
    PseudoModem *modem = (PseudoModem *)object;
    if (modem->modemToken() == modemToken)
      return modem;
  }
  return NULL;
}
///////////////////////////////////////////////////////////////
PObject::Comparison PseudoModem::Compare(const PObject & obj) const
{
  PAssert(PIsDescendant(&obj, PseudoModem), PInvalidCast);
  const PseudoModem & other = (const PseudoModem &)obj;
  return modemToken().Compare(other.modemToken());
}
///////////////////////////////////////////////////////////////
PseudoModemQ::PseudoModemQ()
{
  pmodem_list = new PseudoModemList();
}

PseudoModemQ::~PseudoModemQ()
{
  delete pmodem_list;
}

BOOL PseudoModemQ::CreateModem(
    const PString &tty,
    const PString &route,
    const PNotifier &callbackEndPoint
)
{
  PseudoModem *modem = PseudoModemDrivers::CreateModem(tty, route, callbackEndPoint);

  if (!modem)
      return FALSE;

  if (pmodem_list->Append(modem) == P_MAX_INDEX)
      return FALSE;

  modem->Resume();

  return TRUE;
}

void PseudoModemQ::Enqueue(PseudoModem *modem)
{
  myPTRACE(3, "PseudoModemQ::Enqueue "
    << ((modem != NULL) ? modem->ptyName() : "BAD"));

  PWaitAndSignal mutexWait(Mutex);
  _PseudoModemQ::Enqueue(modem);
}

BOOL PseudoModemQ::Enqueue(const PString &modemToken)
{
  PseudoModem *modem = pmodem_list->Find(modemToken);

  if (!modem) {
      myPTRACE(1, "PseudoModemQ::Enqueue BAD token " << modemToken);
      return FALSE;
  }

  Enqueue(modem);

  return TRUE;
}

PseudoModem *PseudoModemQ::DequeueWithRoute(const PString &number)
{
  PWaitAndSignal mutexWait(Mutex);
  PObject *object;
  
  for( PINDEX i = 0 ; (object = GetAt(i)) != NULL ; i++ ) {
    PAssert(PIsDescendant(object, PseudoModem), PInvalidCast);
    PseudoModem *modem = (PseudoModem *)object;
    if (modem->CheckRoute(number) && modem->IsReady()) {
      if (!Remove(modem))
        modem = NULL;
      myPTRACE(3, "PseudoModemQ::DequeueWithRoute "
        << ((modem != NULL) ? modem->ptyName() : "BAD"));
      return modem;
    }
  }
  return NULL;
}

PseudoModem *PseudoModemQ::Find(const PString &modemToken) const
{
  PObject *object;

  for( PINDEX i = 0 ; (object = GetAt(i)) != NULL ; i++ ) {
    PAssert(PIsDescendant(object, PseudoModem), PInvalidCast);
    PseudoModem *modem = (PseudoModem *)object;
    if( modem->modemToken() == modemToken ) {
      return modem;
    }
  }
  return NULL;
}

PseudoModem *PseudoModemQ::Dequeue(const PString &modemToken)
{
  PWaitAndSignal mutexWait(Mutex);
  PseudoModem *modem = Find(modemToken);
  if (modem != NULL && !Remove(modem))
    modem = NULL;
  myPTRACE(1, "PseudoModemQ::Dequeue "
    << ((modem != NULL) ? modem->ptyName() : "BAD"));
  return modem;
}
///////////////////////////////////////////////////////////////

