/*
 * $Id: pmodem.cxx,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: pmodem.cxx,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include <ptlib.h>
#include "pmodem.h"
#include "pmodemi.h"

#define new PNEW

///////////////////////////////////////////////////////////////
PseudoModem::PseudoModem(const PString &_tty, const PNotifier &callbackEndPoint)
{
  PRegularExpression reg(ttyPattern(), PRegularExpression::Extended);

  if( _tty.FindRegEx(reg) == 0 ) {
    if( _tty[0] != '/' )
      ttypath = "/dev/" + _tty;
    else
      ttypath = _tty;
    (ptypath = ttypath)[5] = 'p';
    ptyname = &ptypath[5];
    
    body = new PseudoModemBody(*this, callbackEndPoint);
    body->Resume();
  } else {
    body = NULL;
    myPTRACE(3, "PseudoModem::PseudoModem bad on " << _tty);
  }
}

PseudoModem::~PseudoModem()
{
  if( body ) {
    body->SignalStop();
    body->WaitForTermination();
    delete body;
  }
}

PObject::Comparison PseudoModem::Compare(const PObject & obj) const
{
  PAssert(obj.IsDescendant(PseudoModem::Class()), PInvalidCast);
  const PseudoModem & other = (const PseudoModem &)obj;
  return modemToken().Compare(other.modemToken());
}

BOOL PseudoModem::IsReady() const
{
  return body && body->IsReady();
}

BOOL PseudoModem::Request(PStringToString &request) const
{
  return body && body->Request(request);
}

BOOL PseudoModem::Attach(T38Engine *t38engine) const
{
  return body && body->Attach(t38engine);
}

void PseudoModem::Detach(T38Engine *t38engine) const
{
  if( body )
    body->Detach(t38engine);
}

const char *PseudoModem::ttyPattern()
{
  return "^(/dev/)?tty[pqrstuvwxyzPQRST][0123456789abcdef]$";
}
///////////////////////////////////////////////////////////////
void PseudoModemQ::Enqueue(PseudoModem *modem)
{
  PWaitAndSignal mutexWait(Mutex);
  _PseudoModemQ::Enqueue(modem);
}

PseudoModem *PseudoModemQ::Dequeue()
{
  PWaitAndSignal mutexWait(Mutex);
  PObject *object;
  
  for( PINDEX i = 0 ; (object = GetAt(i)) != NULL ; i++ ) {
    PAssert(object->IsDescendant(PseudoModem::Class()), PInvalidCast);
    PseudoModem *modem = (PseudoModem *)object;
    if( modem->IsReady() ) {
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
  PseudoModem *modem;
  while( (modem = Dequeue()) != NULL ) {
    delete modem;
  }
}
///////////////////////////////////////////////////////////////

