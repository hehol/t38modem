/*
 * pty.cxx
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
 * $Log: pty.cxx,v $
 * Revision 1.6  2002-12-20 10:13:04  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
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

#include <sys/time.h>
#include "pmodemi.h"
#include "pty.h"

#define new PNEW

#define PrepareSelect(n, fdset, tv, s)	\
  int n = hPty + 1;			\
  fd_set fdset;				\
  struct timeval tv;			\
  FD_ZERO(&fdset);			\
  FD_SET(hPty, &fdset);			\
  tv.tv_sec = s;			\
  tv.tv_usec = 0;
///////////////////////////////////////////////////////////////
UniPty::UniPty(PseudoModemBody &_parent)
  : ModemThreadChild(_parent),
    hPty(_parent.handlePty())
{
}
///////////////////////////////////////////////////////////////
InPty::InPty(PseudoModemBody &_parent)
  : UniPty(_parent)
{
}

void InPty::Main()
{
    RenameCurrentThread(Parent().ptyName() + "(i)");
    myPTRACE(1, "--> Started");

    for(;;) {
      PrepareSelect(n, fdset, tv, 5);
      
      if (stop)
        break;
      ::select(n, &fdset, NULL, NULL, &tv);

      if (FD_ISSET(hPty, &fdset)) {
        char cbuf[1024];
        int len;
        
        {
          PWaitAndSignal mutexWait(Parent().ptyMutex);	// ???
          if (stop)
            break;
          len = ::read(hPty, cbuf, 1024);
        }
          
        if (len < 0) {
          int err = errno;
          myPTRACE(1, "--> read ERROR " << len << " " << strerror(err));
          SignalStop();
          break;
        }
        
        if (len > 0) {
          Parent().ToPtyQ(cbuf, len, FALSE);
          if (stop)
            break;
        }
      }
    }
    myPTRACE(1, "--> Stopped");
}
///////////////////////////////////////////////////////////////
OutPty::OutPty(PseudoModemBody &_parent)
  : UniPty(_parent)
{
}

void OutPty::Main()
{
    RenameCurrentThread(Parent().ptyName() + "(o)");
    myPTRACE(1, "<-- Started");

    PBYTEArray *buf = NULL;
    PINDEX done = 0;

    for(;;) {
      PrepareSelect(n, fdset, tv, 5);
      
      while( buf == NULL ) {
        if( stop ) break;
        WaitDataReady();
        if( stop ) break;
        buf = Parent().FromOutPtyQ();
        done = 0;
      }
      
      if( stop ) break;
      ::select(n, NULL, &fdset, NULL, &tv);

      if(FD_ISSET(hPty, &fdset)) {
        int len;
        
        {
          PWaitAndSignal mutexWait(Parent().ptyMutex);	// ???
          if( stop ) break;
          len = ::write(hPty, (const BYTE *)*buf + done, buf->GetSize() - done);
        }
          
        if (len < 0) {
          int err = errno;
          myPTRACE(1, "<-- write ERROR " << len << " " << strerror(err));
          SignalStop();
          break;
        }

        done += len;
        if( buf->GetSize() <= done ) {
          if( buf->GetSize() < done ) {
            myPTRACE(1, "<-- " << buf->GetSize() << "(size) < (done)" << done << " " << len);
          }
          delete buf;
          buf = NULL;
        }
      }
    }
    
    if( buf ) {
      if( buf->GetSize() != done ) {
        PTRACE(1, "<-- Not sent " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, buf->GetSize() - done)));
      }
      delete buf;
    }

    myPTRACE(1, "<-- Stopped");
}
///////////////////////////////////////////////////////////////
BOOL PseudoModemBody::OpenPty()
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
        myPTRACE(1, "PseudoModemBody::OpenPty read ERROR " << len << " " << strerror(err));
      } else if (len > 0) {
        PBYTEArray *buf = new PBYTEArray((const BYTE *)cbuf, len);
        myPTRACE(3, "PseudoModemBody::OpenPty read " << PRTHEX(*buf));
        inPtyQ.Enqueue(buf);
      }
    }
    if (IsOpenPty()) {
      // hPty is good (slave tty was not closed or was opened after closing)
      return TRUE;
    }
  }
  
  int delay = 0;

  while ((hPty = ::open(ptyPath(), O_RDWR | O_NOCTTY)) < 0) {
    int err = errno;
    myPTRACE(delay + 1, "PseudoModemBody::OpenPty open " << ptyPath() << " ERROR:" << strerror(err));
    if (err == ENOENT)
      return FALSE;
    myPTRACE(delay + 1, "PseudoModemBody::OpenPty will try again to open " << ptyPath());
    if (delay > 0)
      PThread::Sleep(delay * 1000);
    if (++delay > 5)
      delay = 5;
  }
  
  struct termios Termios;
  
  if (::tcgetattr(hPty, &Termios) != 0) {
    int err = errno;
    myPTRACE(1, "PseudoModemBody::OpenPty tcgetattr " << ptyPath() << " ERROR:" << strerror(err));
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
    myPTRACE(1, "PseudoModemBody::OpenPty tcsetattr " << ptyPath() << " ERROR:" << strerror(err));
    ClosePty();
    return FALSE;
  }
  return TRUE;
}

void PseudoModemBody::ClosePty()
{
  if (!IsOpenPty()) return;

  if (::close(hPty) != 0) {
    int err = errno;
    myPTRACE(1, "PseudoModemBody::ClosePty close " << ptyPath() << " ERROR:" << strerror(err));
  }
  
  hPty = -1;
}
///////////////////////////////////////////////////////////////
BOOL PseudoModem::ttySet(const PString &_tty)
{
  PRegularExpression reg(ttyPattern(), PRegularExpression::Extended);

  if (_tty.FindRegEx(reg) == 0) {
    if (_tty[0] != '/')
      ttypath = "/dev/" + _tty;
    else
      ttypath = _tty;
    (ptypath = ttypath)[5] = 'p';
    ptyname = &ptypath[5];
    
    return TRUE;
  }
  return FALSE;
}

const char *PseudoModem::ttyPattern()
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
///////////////////////////////////////////////////////////////

