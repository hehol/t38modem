/*
 * pmodemi.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2005 Vyacheslav Frolov
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
 * Revision 1.12  2005-02-03 11:32:12  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.12  2005/02/03 11:32:12  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.11  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 *
 * Revision 1.10  2004/07/07 07:49:19  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.9  2004/03/01 17:18:46  vfrolov
 * Increased MAX_qBUF
 *
 * Revision 1.8  2003/12/04 12:10:16  vfrolov
 * Tuned max delay for buffer full condition
 *
 * Revision 1.7  2002/12/30 12:49:33  vfrolov
 * Added tracing thread's CPU usage (Linux only)
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

#include <ptlib.h>
#include "pmodemi.h"
#include "pmodeme.h"

#define new PNEW

///////////////////////////////////////////////////////////////
PseudoModemBody::PseudoModemBody(const PString &_route, const PNotifier &_callbackEndPoint)
  : route(_route),
    callbackEndPoint(_callbackEndPoint),
    engine(NULL)
{
}

PseudoModemBody::~PseudoModemBody()
{
  PseudoModemBody::StopAll();
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
    static const PINDEX MAX_qBUF = 1024*2;
    static const int MAX_delay = ((MAX_qBUF/2)*8*1000)/14400;
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
      ModemThreadChild *notify = OutQ ? GetPtyNotifier() : (ModemThreadChild *)engine;
      if (notify == NULL) {
        myPTRACE(1, "PseudoModemBody::ToPtyQ notify == NULL");
        PtyQ.Clean();
        return;
      }
      notify->SignalDataReady();
    }
    if( count == 0 )
      return;

    if (stop)
      break;

    if (delay > MAX_delay) {
      delay = MAX_delay;
      myPTRACE(2, "PseudoModemBody::ToPtyQ(" << (OutQ ? "outPtyQ" : "inPtyQ") << ")"
        << " busy=" << busy << " count=" << count << " delay=" << delay);
    }
    PThread::Sleep(delay);
    if( stop ) break;
  }
}

BOOL PseudoModemBody::StartAll()
{
  if (engine)
    return TRUE;

  if ((engine = new ModemEngine(*this)) != NULL) {
    engine->Resume();
    return TRUE;
  }
  PseudoModemBody::StopAll();
  return FALSE;
}

void PseudoModemBody::StopAll()
{
  if (engine) {
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

BOOL PseudoModemBody::AddModem() const
{
  PStringToString request;
  request.SetAt("modemtoken", modemToken());
  request.SetAt("command", "addmodem");
  callbackEndPoint(request, 10);
  return request("response") == "confirm";
}

void PseudoModemBody::Main()
{
  RenameCurrentThread(ptyName() + "(b)");

  myPTRACE(2, "Started for " << ttyPath() <<
              " (accepts " << (route.IsEmpty() ? PString("all") : route) << ")");

  MainLoop();

  myPTRACE(2, "Stopped " << GetThreadTimes(", CPU usage: "));
}
///////////////////////////////////////////////////////////////

