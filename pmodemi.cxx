/*
 * pmodemi.cxx
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
 * $Log: pmodemi.cxx,v $
 * Revision 1.6  2002-12-20 10:12:54  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.6  2002/12/20 10:12:54  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.5  2002/05/15 16:17:55  vfrolov
 * Implemented per modem routing for I/C calls
 *
 * Revision 1.4  2002/03/05 12:40:27  vfrolov
 * Changed class hierarchy
 *   PseudoModem is abstract
 *   PseudoModemBody is child of PseudoModem
 *   Added PseudoModemQ::CreateModem() to create instances
 *
 * Revision 1.3  2002/03/01 08:53:12  vfrolov
 * Added Copyright header
 * Some OS specific code moved from pmodemi.cxx to pty.cxx
 * Added error code string to log
 * Fixed race condition with fast close and open slave tty
 * Some other changes
 *
 * Revision 1.2  2002/01/10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include "pmodemi.h"
#include "pty.h"
#include "pmodeme.h"

#define new PNEW

///////////////////////////////////////////////////////////////
PseudoModemBody::PseudoModemBody(const PString &_tty, const PString &_route, const PNotifier &_callbackEndPoint)
  : PseudoModem(_tty),
    route(_route),
    callbackEndPoint(_callbackEndPoint),
    hPty(-1),
    inPty(NULL),
    outPty(NULL),
    engine(NULL)
{
}

PseudoModemBody::~PseudoModemBody()
{
  StopAll();
  ClosePty();
}

BOOL PseudoModemBody::IsReady() const
{
  PWaitAndSignal mutexWait(Mutex);
  return engine && engine->IsReady();
}

BOOL PseudoModemBody::CheckRoute(const PString &number) const
{
  return route.IsEmpty() || number.Find(route) == 0;
}

BOOL PseudoModemBody::Request(PStringToString &request) const
{
  PWaitAndSignal mutexWait(Mutex);
  return engine && engine->Request(request);
}

BOOL PseudoModemBody::Attach(T38Engine *t38engine) const
{
  PWaitAndSignal mutexWait(Mutex);
  return engine && engine->Attach(t38engine);
}

void PseudoModemBody::Detach(T38Engine *t38engine) const
{
  PWaitAndSignal mutexWait(Mutex);
  if( engine )
    engine->Detach(t38engine);
}

void PseudoModemBody::ToPtyQ(const void *buf, PINDEX count, BOOL OutQ)
{
  if( count == 0 )
    return;
    
  PBYTEArrayQ &PtyQ = OutQ ? outPtyQ : inPtyQ;
  
  for( int delay = 10 ;; delay *= 2 ) {
    const PINDEX MAX_qBUF = 1024;
    PINDEX busy = PtyQ.GetCount();
    if( busy < MAX_qBUF ) {
      PINDEX free = MAX_qBUF - busy;
      PINDEX len = count;
      if( len > free )
        len = free;
      PtyQ.Enqueue(new PBYTEArray((const BYTE *)buf, len));
      buf = (const BYTE *)buf + len;
      count -= len;
    }
    
    {
      PWaitAndSignal mutexWait(Mutex);
      ModemThreadChild *notify = OutQ ? (ModemThreadChild *)outPty : (ModemThreadChild *)engine;
      if( notify == NULL ) {
        myPTRACE(1, "PseudoModemBody::ToPtyQ notify == NULL");
        PtyQ.Clean();
        return;
      }
      notify->SignalDataReady();
    }
    if( count == 0 )
      return;
  
    if( stop ) break;
    if( delay > 1000 ) {
      delay = 1000;
      myPTRACE(1, "PseudoModemBody::ToPtyQ busy=" << busy << " count=" << count);
    }
    PThread::Sleep(delay);
    if( stop ) break;
  }
}

BOOL PseudoModemBody::StartAll()
{
  if (IsOpenPty()
     && (inPty = new InPty(*this))
     && (outPty = new OutPty(*this))
     && (engine = new ModemEngine(*this))
     ) {
    inPty->Resume();
    outPty->Resume();
    engine->Resume();
    return TRUE;
  }
  StopAll();
  ClosePty();
  return FALSE;
}

void PseudoModemBody::StopAll()
{
  if(inPty) {
    inPty->SignalStop();
    inPty->WaitForTermination();
    PWaitAndSignal mutexWait(Mutex);
    delete inPty;
    inPty = NULL;
  }
  if(outPty) {
    outPty->SignalStop();
    outPty->WaitForTermination();
    PWaitAndSignal mutexWait(Mutex);
    delete outPty;
    outPty = NULL;
  }
  if(engine) {
    engine->SignalStop();
    engine->WaitForTermination();
    PWaitAndSignal mutexWait(Mutex);
    delete engine;
    engine = NULL;
  }
  outPtyQ.Clean();
  inPtyQ.Clean();
  childstop = FALSE;
}

void PseudoModemBody::Main()
{
  RenameCurrentThread(ptyName() + "(b)");

  myPTRACE(3, "PseudoModemBody::Main Started on " << ptyPath() << " for " << ttyPath() <<
              " (accepts " << (route.IsEmpty() ? PString("all") : route) << ")");
  
  while( !stop && OpenPty() && StartAll() ) {
    while( !stop && !childstop ) {
      WaitDataReady();
    }
    StopAll();
  }
  ClosePty();

  myPTRACE(3, "PseudoModemBody::Main stop on " << ptyPath());
}
///////////////////////////////////////////////////////////////

