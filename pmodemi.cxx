/*
 * $Id: pmodemi.cxx,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: pmodemi.cxx,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include "pmodem.h"
#include "pmodemi.h"
#include "pty.h"
#include "pmodeme.h"

#define new PNEW

///////////////////////////////////////////////////////////////
PseudoModemBody::PseudoModemBody(const PseudoModem &_parent, const PNotifier &_callbackEndPoint)
  : parent(_parent),
    callbackEndPoint(_callbackEndPoint),
    hPty(-1),
    inPty(NULL),
    outPty(NULL),
    engine(NULL)
{
  SetThreadName(ptyName() + "(b):%0x");
}

PseudoModemBody::~PseudoModemBody()
{
  StopAll();
  ClosePty();
}

const PString &PseudoModemBody::ptyName() const
{
  return parent.ptyName();
}

const PString &PseudoModemBody::ptyPath() const
{
  return parent.ptyPath();
}

const PString &PseudoModemBody::ttyPath() const
{
  return parent.ttyPath();
}

const PString &PseudoModemBody::modemToken() const
{
  return parent.modemToken();
}

BOOL PseudoModemBody::IsReady() const
{
  PWaitAndSignal mutexWait(Mutex);
  return engine && engine->IsReady();
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
  if( OpenPty() &&
         (inPty = new InPty(*this)) &&
         (outPty = new OutPty(*this)) &&
         (engine = new ModemEngine(*this)) &&
         TRUE ) {
    inPty->Resume();
    outPty->Resume();
    engine->Resume();
    return TRUE;
  }
  StopAll();
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
  ClosePty();	// ???
}

BOOL PseudoModemBody::OpenPty()
{
  if( IsOpenPty() ) return TRUE;

  while ((hPty = ::open(ptyPath(), O_RDWR | O_NOCTTY)) < 0) {
    if (errno == ENOENT) {
         myPTRACE(3, "PseudoModemBody::PseudoModemBody bad file name " << ptyPath());
         return FALSE;
    }
    myPTRACE(3, "PseudoModemBody::Main will try again on " << ptyPath());
    PThread::Sleep(5000);
  }
  
  struct termios Termios;
  
  if( ::tcgetattr(hPty, &Termios) != 0 ) {
     myPTRACE(3, "PseudoModemBody::Main tcgetattr error on " << ptyPath());
     ClosePty();
     return FALSE;
  }
  Termios.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOCTL | ECHOE | ECHOK | ECHOKE | ECHONL | ECHOPRT);
  Termios.c_iflag |= IGNBRK;
  //Termios.c_iflag &= ~IGNBRK;
  //Termios.c_iflag |= BRKINT;
  Termios.c_cc[VMIN] = 1;
  Termios.c_cc[VTIME] = 0;
  if( ::tcsetattr(hPty,TCSANOW,&Termios) != 0 ) {
     myPTRACE(3, "PseudoModemBody::Main tcsetattr error on " << ptyPath());
     ClosePty();
     return FALSE;
  }
  return TRUE;
}

void PseudoModemBody::ClosePty()
{
  if( !IsOpenPty() ) return;

  if( ::close(hPty) != 0 ) {
     myPTRACE(3, "PseudoModemBody::ClosePty close error on " << ptyPath());
  }
  
  hPty = -1;
}

void PseudoModemBody::Main()
{
  myPTRACE(3, "PseudoModemBody::Main Started on " << ptyPath() << " for " << ttyPath());
  
  while( !stop && StartAll() ) {
    while( !stop && !childstop ) {
      WaitDataReady();
    }
    StopAll();
  }

  myPTRACE(3, "PseudoModemBody::Main stop on " << ptyPath());
}
///////////////////////////////////////////////////////////////

