/*
 * drv_pty.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2006 Vyacheslav Frolov
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
 * $Log: drv_pty.cxx,v $
 * Revision 1.3  2006-10-18 14:54:53  vfrolov
 * Added hPty >= FD_SETSIZE check
 *
 * Revision 1.3  2006/10/18 14:54:53  vfrolov
 * Added hPty >= FD_SETSIZE check
 *
 * Revision 1.2  2004/10/20 14:00:17  vfrolov
 * Fixed race condition with SignalDataReady()/WaitDataReady()
 *
 * Revision 1.1  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 *
 *
 * Log: pty.cxx,v
 *
 * Revision 1.7  2002/12/30 12:49:42  vfrolov
 * Added tracing thread's CPU usage (Linux only)
 *
 * Revision 1.6  2002/12/20 10:13:04  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.5  2002/03/05 12:37:45  vfrolov
 * Some OS specific code moved from pmodem.cxx to pty.cxx
 *
 * Revision 1.4  2002/03/01 08:53:16  vfrolov
 * Added Copyright header
 * Some OS specific code moved from pmodemi.cxx to pty.cxx
 * Added error code string to log
 * Fixed race condition with fast close and open slave tty
 * Some other changes
 *
 * Revision 1.3  2002/01/10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.2  2002/01/01 23:43:57  craigs
 * Added fix from Vyacheslav Frolov
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include <ptlib.h>
#include "drv_pty.h"

#ifdef MODEM_DRIVER_Pty

#include <sys/time.h>

#define new PNEW

///////////////////////////////////////////////////////////////
class UniPty : public ModemThreadChild
{
    PCLASSINFO(UniPty, ModemThreadChild);
  public:
    UniPty(PseudoModemPty &_parent, int _hPty);
  protected:
    PseudoModemPty &Parent() { return (PseudoModemPty &)parent; }
    int hPty;
};
///////////////////////////////////////////////////////////////
class InPty : public UniPty
{
    PCLASSINFO(InPty, UniPty);
  public:
    InPty(PseudoModemPty &_parent, int _hPty);
  protected:
    virtual void Main();
};
///////////////////////////////////////////////////////////////
class OutPty : public UniPty
{
    PCLASSINFO(OutPty, UniPty);
  public:
    OutPty(PseudoModemPty &_parent, int _hPty);
  protected:
    virtual void Main();
};
///////////////////////////////////////////////////////////////
#define PrepareSelect(n, fdset, tv, s)	\
  int n = hPty + 1;			\
  fd_set fdset;				\
  struct timeval tv;			\
  FD_ZERO(&fdset);			\
  FD_SET(hPty, &fdset);			\
  tv.tv_sec = s;			\
  tv.tv_usec = 0;
///////////////////////////////////////////////////////////////
UniPty::UniPty(PseudoModemPty &_parent, int _hPty)
  : ModemThreadChild(_parent),
    hPty(_hPty)
{
}
///////////////////////////////////////////////////////////////
InPty::InPty(PseudoModemPty &_parent, int _hPty)
  : UniPty(_parent, _hPty)
{
}

void InPty::Main()
{
  RenameCurrentThread(Parent().ptyName() + "(i)");
  myPTRACE(1, "--> Started");

  for (;;) {
    PrepareSelect(n, fdset, tv, 5);

    if (stop)
      break;

    ::select(n, &fdset, NULL, NULL, &tv);

    if (FD_ISSET(hPty, &fdset)) {
      char cbuf[1024];
      int len;

      if (stop)
        break;

      len = ::read(hPty, cbuf, 1024);

      if (len < 0) {
        int err = errno;
        myPTRACE(1, "--> read ERROR " << len << " " << strerror(err));
        SignalStop();
        break;
      }

      if (len == 0) {
        SignalStop();
        break;
      }

      if (len > 0) {
        Parent().ToInPtyQ(cbuf, len);
        if (stop)
          break;
      }
    }
  }

  myPTRACE(1, "--> Stopped" << GetThreadTimes(", CPU usage: "));
}
///////////////////////////////////////////////////////////////
OutPty::OutPty(PseudoModemPty &_parent, int _hPty)
  : UniPty(_parent, _hPty)
{
}

void OutPty::Main()
{
  RenameCurrentThread(Parent().ptyName() + "(o)");
  myPTRACE(1, "<-- Started");

  PBYTEArray *buf = NULL;
  PINDEX done = 0;

  for (;;) {
    PrepareSelect(n, fdset, tv, 5);

    while (!buf) {
      if (stop)
        break;
      buf = Parent().FromOutPtyQ();
      if (buf) {
        done = 0;
        break;
      }
      WaitDataReady();
    }

    if (stop)
      break;

    ::select(n, NULL, &fdset, NULL, &tv);

    if (FD_ISSET(hPty, &fdset)) {
      int len;

      if (stop)
        break;

      len = ::write(hPty, (const BYTE *)*buf + done, buf->GetSize() - done);

      if (len < 0) {
        int err = errno;
        myPTRACE(1, "<-- write ERROR " << len << " " << strerror(err));
        SignalStop();
        break;
      }

      done += len;
      if (buf->GetSize() <= done) {
        if (buf->GetSize() < done) {
          myPTRACE(1, "<-- " << buf->GetSize() << "(size) < (done)" << done << " " << len);
        }
        delete buf;
        buf = NULL;
      }
    }
  }

  if (buf) {
    if (buf->GetSize() != done)
      myPTRACE(1, "<-- Not sent " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, buf->GetSize() - done)));
    delete buf;
  }

  myPTRACE(1, "<-- Stopped" << GetThreadTimes(", CPU usage: "));
}
///////////////////////////////////////////////////////////////
PseudoModemPty::PseudoModemPty(const PString &_tty, const PString &_route, const PNotifier &_callbackEndPoint)
  : PseudoModemBody(_route, _callbackEndPoint),
    hPty(-1),
    inPty(NULL),
    outPty(NULL)
{
  if (CheckTty(_tty)) {
    if (_tty[0] != '/')
      ttypath = "/dev/" + _tty;
    else
      ttypath = _tty;

    (ptypath = ttypath)[5] = 'p';
    ptyname = &ptypath[5];

    valid = TRUE;
  } else {
    myPTRACE(1, "PseudoModemPty::PseudoModemPty bad on " << _tty);
    valid = FALSE;
  }
}

PseudoModemPty::~PseudoModemPty()
{
  StopAll();
  ClosePty();
}

inline const char *ttyPattern()
{
#if defined(P_LINUX)
  #define TTY_PATTERN "^(/dev/)?tty[pqrstuvwxyzabcde][0123456789abcdef]$"
#endif
#if defined(P_FREEBSD)
  #define TTY_PATTERN "^(/dev/)?tty[pqrsPQRS][0123456789abcdefghijklmnopqrstuv]$"
#endif
#ifndef TTY_PATTERN
  #define TTY_PATTERN "^(/dev/)?tty..$"
#endif

  return TTY_PATTERN;
}

BOOL PseudoModemPty::CheckTty(const PString &_tty)
{
  PRegularExpression reg(ttyPattern(), PRegularExpression::Extended);

  return _tty.FindRegEx(reg) == 0;
}

PStringArray PseudoModemPty::Description()
{
  PStringArray description;

  description.Append(new PString("Uses pseudo-tty (pty) devices to communicate with fax application."));
  description.Append(new PString(PString("The tty should match to regexp '") + ttyPattern() + "'."));

  return description;
}

const PString &PseudoModemPty::ttyPath() const
{
  return ttypath;
}

ModemThreadChild *PseudoModemPty::GetPtyNotifier()
{
  return outPty;
}

BOOL PseudoModemPty::StartAll()
{
  if (IsOpenPty()
     && (inPty = new InPty(*this, hPty))
     && (outPty = new OutPty(*this, hPty))
     && (PseudoModemBody::StartAll())
     ) {
    inPty->Resume();
    outPty->Resume();
    return TRUE;
  }
  StopAll();
  ClosePty();
  return FALSE;
}

void PseudoModemPty::StopAll()
{
  if (inPty) {
    inPty->SignalStop();
    inPty->WaitForTermination();
    PWaitAndSignal mutexWait(Mutex);
    delete inPty;
    inPty = NULL;
  }
  if (outPty) {
    outPty->SignalStop();
    outPty->WaitForTermination();
    PWaitAndSignal mutexWait(Mutex);
    delete outPty;
    outPty = NULL;
  }
  PseudoModemBody::StopAll();
}

BOOL PseudoModemPty::OpenPty()
{
  if (IsOpenPty()) {
    // check hPty health

    PrepareSelect(n, fdset, tv, 0);

    ::select(n, &fdset, NULL, NULL, &tv);

    if (FD_ISSET(hPty, &fdset)) {
      char cbuf[1];
      int len;

      len = ::read(hPty, cbuf, 1);

      if (len < 0) {
        // hPty is ill (slave tty was closed and still was not opened)
        int err = errno;
        // close hPty ASAP
        ClosePty();
        myPTRACE(1, "PseudoModemPty::OpenPty read ERROR " << len << " " << strerror(err));
      } else if (len > 0) {
        PBYTEArray *buf = new PBYTEArray((const BYTE *)cbuf, len);
        myPTRACE(3, "PseudoModemPty::OpenPty read " << PRTHEX(*buf));
        ToInPtyQ(buf);
      }
    }
    if (IsOpenPty()) {
      // hPty is good (slave tty was not closed or was opened after closing)
      return TRUE;
    }
  }

  int delay = 0;

  while ((hPty = ::open(ptypath, O_RDWR | O_NOCTTY)) < 0) {
    int err = errno;
    myPTRACE(delay + 1, "PseudoModemPty::OpenPty open " << ptypath << " ERROR:" << strerror(err));
    if (err == ENOENT)
      return FALSE;
    myPTRACE(delay + 1, "PseudoModemPty::OpenPty will try again to open " << ptypath);
    if (delay > 0)
      PThread::Sleep(delay * 1000);
    if (++delay > 5)
      delay = 5;
  }

#ifdef FD_SETSIZE
  if (hPty >= FD_SETSIZE) {
    myPTRACE(1, "PseudoModemPty::OpenPty ERROR: hPty(" << hPty << ") >= FD_SETSIZE(" << FD_SETSIZE << ")");
    ClosePty();
    return FALSE;
  }
  #if PTRACING
  if (myCanTrace(2)) {
    static int hPtyWarn = (FD_SETSIZE*3)/4;

    if (hPty > hPtyWarn) {
      hPtyWarn = hPty;
      myPTRACE(2, "PseudoModemPty::OpenPty WARNING: hPty=" << hPty << ", FD_SETSIZE=" << FD_SETSIZE);
    }
  }
  #endif
#else
  #warning FD_SETSIZE not defined!
#endif

  struct termios Termios;

  if (::tcgetattr(hPty, &Termios) != 0) {
    int err = errno;
    myPTRACE(1, "PseudoModemPty::OpenPty tcgetattr " << ptypath << " ERROR:" << strerror(err));
    ClosePty();
    return FALSE;
  }
  Termios.c_lflag &= ~(ICANON | ISIG | ECHO | ECHOCTL | ECHOE | ECHOK | ECHOKE | ECHONL | ECHOPRT);
  Termios.c_iflag |= IGNBRK;
  //Termios.c_iflag &= ~IGNBRK;
  //Termios.c_iflag |= BRKINT;
  Termios.c_cc[VMIN] = 1;
  Termios.c_cc[VTIME] = 0;
  if (::tcsetattr(hPty, TCSANOW, &Termios) != 0) {
    int err = errno;
    myPTRACE(1, "PseudoModemPty::OpenPty tcsetattr " << ptypath << " ERROR:" << strerror(err));
    ClosePty();
    return FALSE;
  }
  return TRUE;
}

void PseudoModemPty::ClosePty()
{
  if (!IsOpenPty())
    return;

  if (::close(hPty) != 0) {
    int err = errno;
    myPTRACE(1, "PseudoModemPty::ClosePty close " << ptypath << " ERROR:" << strerror(err));
  }

  hPty = -1;
}

void PseudoModemPty::MainLoop()
{
  if (AddModem()) {
    while (!stop && OpenPty() && StartAll()) {
      while (!stop && !childstop) {
        WaitDataReady();
      }
      StopAll();
    }
    ClosePty();
  }
}
///////////////////////////////////////////////////////////////

#endif // MODEM_DRIVER_Pty

