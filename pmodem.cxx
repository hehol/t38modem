/*
 * pmodem.cxx
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
 * $Log: pmodem.cxx,v $
 * Revision 1.5  2002-05-15 16:17:44  vfrolov
 * Implemented per modem routing for I/C calls
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
#include "pmodemi.h"

#define new PNEW

///////////////////////////////////////////////////////////////
PseudoModem::PseudoModem(const PString &_tty)
{
  valid = ttySet(_tty);

  if (!valid) {
    myPTRACE(1, "PseudoModem::PseudoModem bad on " << _tty);
  }
}

PObject::Comparison PseudoModem::Compare(const PObject & obj) const
{
  PAssert(obj.IsDescendant(PseudoModem::Class()), PInvalidCast);
  const PseudoModem & other = (const PseudoModem &)obj;
  return modemToken().Compare(other.modemToken());
}
///////////////////////////////////////////////////////////////
BOOL PseudoModemQ::CreateModem(const PString &tty, const PString &route, const PNotifier &callbackEndPoint)
{
  PseudoModem *modem = new PseudoModemBody(tty, route, callbackEndPoint);
  
  if( modem->IsValid() ) {
    if( Find(modem->modemToken()) == NULL ) {
      Enqueue(modem);
    } else {
      myPTRACE(1, "PseudoModemQ::CreateModem can't add " << tty << " to modem queue, delete");
      delete modem;
      return FALSE;
    }
  } else {
    myPTRACE(1, "PseudoModemQ::CreateModem " << tty << " in not valid, delete");
    delete modem;
    return FALSE;
  }
  myPTRACE(3, "PseudoModemQ::CreateModem " << tty << " OK");
  modem->Resume();
  return TRUE;
}

void PseudoModemQ::Enqueue(PseudoModem *modem)
{
  PWaitAndSignal mutexWait(Mutex);
  _PseudoModemQ::Enqueue(modem);
}

PseudoModem *PseudoModemQ::DequeueWithRoute(const PString &number)
{
  PWaitAndSignal mutexWait(Mutex);
  PObject *object;
  
  for( PINDEX i = 0 ; (object = GetAt(i)) != NULL ; i++ ) {
    PAssert(object->IsDescendant(PseudoModem::Class()), PInvalidCast);
    PseudoModem *modem = (PseudoModem *)object;
    if( modem->CheckRoute(number) && modem->IsReady() ) {
      return Remove(modem) ? modem : NULL;;
    }
  }
  return NULL;
}

PseudoModem *PseudoModemQ::Find(const PString &modemToken) const
{
  PWaitAndSignal mutexWait(Mutex);
  PObject *object;
  
  for( PINDEX i = 0 ; (object = GetAt(i)) != NULL ; i++ ) {
    PAssert(object->IsDescendant(PseudoModem::Class()), PInvalidCast);
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
  
  return (modem != NULL && Remove(modem)) ? modem : NULL;
}

void PseudoModemQ::Clean()
{
  PWaitAndSignal mutexWait(Mutex);
  PObject *object;
  while( (object = _PseudoModemQ::Dequeue()) != NULL ) {
    delete object;
  }
}
///////////////////////////////////////////////////////////////

