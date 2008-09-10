/*
 * drv_pty.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2008 Vyacheslav Frolov
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
 * Revision 1.8  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.8  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.7  2007/07/17 10:03:22  vfrolov
 * Added Unix98 PTY support
 *
 * Revision 1.6  2007/02/21 08:04:20  vfrolov
 * Added printing message to stdout if no pty device
 *
 * Revision 1.5  2007/01/29 12:44:41  vfrolov
 * Added ability to put args to drivers
 *
 * Revision 1.4  2006/10/19 10:44:15  vfrolov
 * Fixed big file descriptors problem (replaced select() by poll())
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

#include <sys/poll.h>

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
    pollfd pollfd;

    pollfd.fd = hPty;
    pollfd.events = POLLIN;

    if (stop)
      break;

    ::poll(&pollfd, 1, 5000);

    if (pollfd.revents) {
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
    pollfd pollfd;

    pollfd.fd = hPty;
    pollfd.events = POLLOUT;

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

    ::poll(&pollfd, 1, 5000);

    if (pollfd.revents) {
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
#ifdef USE_LEGACY_PTY
static const char *ttyPatternLegacy()
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
#undef TTY_PATTERN
}

static PBoolean ttyCheckLegacy(const PString &_tty)
{
  PRegularExpression regLegacy(ttyPatternLegacy(), PRegularExpression::Extended);

  return _tty.FindRegEx(regLegacy) == 0;
}
#endif // USE_LEGACY_PTY
///////////////////////////////////////////////////////////////
#ifdef USE_UNIX98_PTY
static const char *ttyPatternUnix98()
{
  return "^\\+.+$";
}

static PBoolean ttyCheckUnix98(const PString &_tty)
{
  PRegularExpression regUnix98(ttyPatternUnix98(), PRegularExpression::Extended);

  return _tty.FindRegEx(regUnix98) == 0;
}

static const char *ptyPathUnix98()
{
  return "/dev/ptmx";
}

static void ttyUnlinkUnix98(const char *ttypath)
{
    char ptsName[64];

    memset(ptsName, 0, sizeof(ptsName));

    if (readlink(ttypath, ptsName, sizeof(ptsName) - 1) >= 0 && ::unlink(ttypath) == 0)
      myPTRACE(1, "PseudoModemPty::OpenPty removed link " << ttypath << " -> " << ptsName);
}
#endif // USE_UNIX98_PTY
///////////////////////////////////////////////////////////////
PseudoModemPty::PseudoModemPty(
    const PString &_tty,
    const PString &_route,
#ifdef USE_UNIX98_PTY
    const PConfigArgs &args,
#else
    const PConfigArgs &/*args*/,
#endif
    const PNotifier &_callbackEndPoint)

  : PseudoModemBody(_route, _callbackEndPoint),
    hPty(-1),
    inPty(NULL),
    outPty(NULL)
{
  valid = TRUE;

#ifdef USE_LEGACY_PTY
  if (ttyCheckLegacy(_tty)) {
    if (_tty[0] != '/')
      ttypath = "/dev/" + _tty;
    else
      ttypath = _tty;

    (ptypath = ttypath)[5] = 'p';
    ptyname = &ptypath[5];
  }
  else
#endif // USE_LEGACY_PTY
#ifdef USE_UNIX98_PTY
  if (ttyCheckUnix98(_tty)) {
    if (args.HasOption("pts-dir")) {
      ttypath = args.GetOptionString("pts-dir");

      if (!ttypath.IsEmpty() && ttypath.Right(1) != "/")
        ttypath += "/";
    }

    ttypath += _tty.Mid(1);
    ptypath = ptyPathUnix98();

    PINDEX i = ttypath.FindLast('/');

    if (i == P_MAX_INDEX)
      i = 0;
    else
      i++;

    ptyname = ttypath.Mid(i);
  }
  else
#endif // USE_UNIX98_PTY
  {
    myPTRACE(1, "PseudoModemPty::PseudoModemPty bad on " << _tty);
    valid = FALSE;
  }
}

PseudoModemPty::~PseudoModemPty()
{
  StopAll();
  ClosePty();
}

PBoolean PseudoModemPty::CheckTty(const PString &_tty)
{
#ifdef USE_LEGACY_PTY
  if (ttyCheckLegacy(_tty))
    return TRUE;
#endif

#ifdef USE_UNIX98_PTY
  if (ttyCheckUnix98(_tty))
    return TRUE;
#endif

  return FALSE;
}

PString PseudoModemPty::ArgSpec()
{
  return
#ifdef USE_UNIX98_PTY
        "-pts-dir:"
#endif
        "";
}

PStringArray PseudoModemPty::Description()
{
  PStringArray descriptions = PString(
        "Uses pseudo-tty (pty) devices to communicate with a fax application.\n"
#ifdef USE_LEGACY_PTY
        "For legacy ptys the tty should match to the regexp\n"
        "  '" + PString(ttyPatternLegacy()) + "'\n"
#endif
#ifdef USE_UNIX98_PTY
        "For Unix98 ptys the tty should match to the regexp\n"
        "  '" + PString(ttyPatternUnix98()) + "'\n"
        "(the first character '+' will be replaced by a base directory).\n"
        "Options:\n"
        "  --pts-dir dir         : Set a base directory for Unix98 scheme,\n"
        "                          default is empty.\n"
#endif
  ).Lines();

  return descriptions;
}

const PString &PseudoModemPty::ttyPath() const
{
  return ttypath;
}

ModemThreadChild *PseudoModemPty::GetPtyNotifier()
{
  return outPty;
}

PBoolean PseudoModemPty::StartAll()
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

PBoolean PseudoModemPty::OpenPty()
{
  if (IsOpenPty()) {
    // check hPty health

    pollfd pollfd;

    pollfd.fd = hPty;
    pollfd.events = POLLIN;

    ::poll(&pollfd, 1, 0);

    if (pollfd.revents) {
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
    myPTRACE(delay + 1, "PseudoModemPty::OpenPty open " << ptypath << " ERROR: " << strerror(err));
    if (err == ENOENT) {
      cout << "Could not open " << ptypath << ": " << strerror(err) << endl;
      return FALSE;
    }
    myPTRACE(delay + 1, "PseudoModemPty::OpenPty will try again to open " << ptypath);
    if (delay > 0)
      PThread::Sleep(delay * 1000);
    if (++delay > 5)
      delay = 5;
  }

#if defined FD_TRACE_LEVEL && defined PTRACING
  #ifdef FD_SETSIZE
  if (hPty > (FD_SETSIZE*3)/4)
    myPTRACE(FD_TRACE_LEVEL, "PseudoModemPty::OpenPty WARNING: hPty=" << hPty << ", FD_SETSIZE=" << FD_SETSIZE);
  #else
    #warning FD_SETSIZE not defined!
  #endif
#endif

  struct termios Termios;

  if (::tcgetattr(hPty, &Termios) != 0) {
    int err = errno;
    myPTRACE(1, "PseudoModemPty::OpenPty tcgetattr " << ptyname << " ERROR: " << strerror(err));
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
    myPTRACE(1, "PseudoModemPty::OpenPty tcsetattr " << ptyname << " ERROR: " << strerror(err));
    ClosePty();
    return FALSE;
  }

#ifdef USE_UNIX98_PTY
  if (ptypath == ptyPathUnix98()) {
    ttyUnlinkUnix98(ttypath);

    if (::unlockpt(hPty) != 0) {
      int err = errno;
      myPTRACE(1, "PseudoModemPty::OpenPty unlockpt " << ptyname << " ERROR: " << strerror(err));
      ClosePty();
      return FALSE;
    }

    char ptsName[64];

    if (::ptsname_r(hPty, ptsName, sizeof(ptsName)) != 0) {
      int err = errno;
      myPTRACE(1, "PseudoModemPty::OpenPty ptsname_r " << ptyname << " ERROR: " << strerror(err));
      ClosePty();
      return FALSE;
    }

    if (::symlink(ptsName, ttypath) != 0) {
      int err = errno;
      myPTRACE(1, "PseudoModemPty::OpenPty symlink " << ttypath << " -> " << ptsName << " ERROR: " << strerror(err));
      ClosePty();
      return FALSE;
    }

    myPTRACE(1, "PseudoModemPty::OpenPty added link " << ttypath << " -> " << ptsName);
  }
#endif // USE_UNIX98_PTY

  return TRUE;
}

void PseudoModemPty::ClosePty()
{
  if (!IsOpenPty())
    return;

#ifdef USE_UNIX98_PTY
  ttyUnlinkUnix98(ttypath);
#endif

  if (::close(hPty) != 0) {
    int err = errno;
    myPTRACE(1, "PseudoModemPty::ClosePty close " << ptyname << " ERROR: " << strerror(err));
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

