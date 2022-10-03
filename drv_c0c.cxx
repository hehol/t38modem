/*
 * drv_c0c.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2004-2010 Vyacheslav Frolov
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
 * Contributor(s):
 *
 * $Log: drv_c0c.cxx,v $
 * Revision 1.13  2010-12-28 12:27:22  vfrolov
 * Added fTXContinueOnXoff to prevent deadlock
 *
 * Revision 1.13  2010/12/28 12:27:22  vfrolov
 * Added fTXContinueOnXoff to prevent deadlock
 *
 * Revision 1.12  2009/07/08 18:43:44  vfrolov
 * Added PseudoModem::ttyName()
 *
 * Revision 1.11  2009/03/13 09:44:32  vfrolov
 * Fixed Segmentation fault (wrong PString usage)
 *
 * Revision 1.10  2008/09/24 14:41:06  frolov
 * Removed CTS monitoring
 *
 * Revision 1.9  2008/09/11 07:41:48  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.8  2007/03/30 11:01:12  vfrolov
 * Replaced strerror() by FormatMessage()
 *
 * Revision 1.7  2007/03/22 16:22:25  vfrolov
 * Added some changes
 *
 * Revision 1.6  2007/01/29 12:44:41  vfrolov
 * Added ability to put args to drivers
 *
 * Revision 1.5  2005/03/03 16:12:46  vfrolov
 * Fixed potential handle leak
 * Fixed compiler warnings
 *
 * Revision 1.4  2005/02/10 15:04:57  vfrolov
 * Disabled I/C calls for closed ports
 *
 * Revision 1.3  2004/10/20 14:00:16  vfrolov
 * Fixed race condition with SignalDataReady()/WaitDataReady()
 *
 * Revision 1.2  2004/08/30 12:11:33  vfrolov
 * Enabled input XON/XOFF control
 *
 * Revision 1.1  2004/07/07 13:36:46  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>
#include "drv_c0c.h"

#ifdef MODEM_DRIVER_C0C

#define new PNEW

//////////////////////////////////////////////////////////////
class UniC0C : public ModemThreadChild
{
    PCLASSINFO(UniC0C, ModemThreadChild);
  public:
    UniC0C(PseudoModemC0C &_parent, HANDLE _hC0C);
  protected:
    PseudoModemC0C &Parent() { return (PseudoModemC0C &)parent; }
    HANDLE hC0C;
};
///////////////////////////////////////////////////////////////
class InC0C : public UniC0C
{
    PCLASSINFO(InC0C, UniC0C);
  public:
    InC0C(PseudoModemC0C &_parent, HANDLE _hC0C);
  protected:
    virtual void Main();
};
///////////////////////////////////////////////////////////////
class OutC0C : public UniC0C
{
    PCLASSINFO(OutC0C, UniC0C);
  public:
    OutC0C(PseudoModemC0C &_parent, HANDLE _hC0C);
  protected:
    virtual void Main();
};
///////////////////////////////////////////////////////////////
UniC0C::UniC0C(PseudoModemC0C &_parent, HANDLE _hC0C)
  : ModemThreadChild(_parent),
    hC0C(_hC0C)
{
}
///////////////////////////////////////////////////////////////
#if PTRACING
static PString strError(DWORD err)
{
  LPVOID pMsgBuf;

  FormatMessage(
      FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
      NULL,
      err,
      MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
      (LPTSTR) &pMsgBuf,
      0,
      NULL);

  PString str((const char *)pMsgBuf);

  LocalFree(pMsgBuf);

  return str.Trim() + " (" + PString(err) + ")";
}

static void TraceLastError(const PString &head)
{
  DWORD err = ::GetLastError();
  myPTRACE(1, "T38Modem\t" << head << " ERROR " << strError(err));
}
#else
#define TraceLastError(head)
#endif
///////////////////////////////////////////////////////////////
static void CloseEvents(int num, HANDLE *hEvents)
{
  for (int i = 0 ; i < num ; i++) {
    if (hEvents[i]) {
      if (!::CloseHandle(hEvents[i])) {
        TraceLastError("CloseEvents() CloseHandle()");
      }
      hEvents[i] = NULL;
    }
  }
}

static BOOL PrepareEvents(int num, HANDLE *hEvents, OVERLAPPED *overlaps)
{
  memset(hEvents, 0, num * sizeof(HANDLE));
  memset(overlaps, 0, num * sizeof(OVERLAPPED));

  for (int i = 0 ; i < num ; i++) {
    overlaps[i].hEvent = hEvents[i] = ::CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvents[i]) {
      TraceLastError("PrepareEvents() CreateEvent()");
      CloseEvents(i, hEvents);
      return FALSE;
    }
  }
  return TRUE;
}

static BOOL myClearCommError(HANDLE hC0C, DWORD *pErrors)
{
  if (!ClearCommError(hC0C, pErrors, NULL)) {
    TraceLastError("ClearCommError()");
    return FALSE;
  }
  return TRUE;
}

static BOOL myGetCommState(HANDLE hC0C, DCB *dcb)
{
  dcb->DCBlength = sizeof(*dcb);

  if (!GetCommState(hC0C, dcb)) {
    TraceLastError("GetCommState()");
    return FALSE;
  }
  return TRUE;
}

static BOOL mySetCommState(HANDLE hC0C, DCB *dcb)
{
  if (!SetCommState(hC0C, dcb)) {
    TraceLastError("SetCommState()");
    return FALSE;
  }
  return TRUE;
}
///////////////////////////////////////////////////////////////
InC0C::InC0C(PseudoModemC0C &_parent, HANDLE _hC0C)
  : UniC0C(_parent, _hC0C)
{
}

void InC0C::Main()
{
  RenameCurrentThread(Parent().ptyName() + "(i)");
  myPTRACE(1, "T38Modem\t--> Started");

  enum {
    EVENT_READ,
    EVENT_STAT,
    EVENT_NUM
  };

  HANDLE hEvents[EVENT_NUM];
  OVERLAPPED overlaps[EVENT_NUM];

  if (!PrepareEvents(EVENT_NUM, hEvents, overlaps))
    SignalStop();

  if (!SetCommMask(hC0C, EV_DSR|EV_BREAK)) {
    TraceLastError("SetCommMask()");
    SignalStop();
  }

  char cbuf[32];
  DWORD cbufRead = 0;
  BOOL waitingRead = FALSE;
  DWORD maskStat = 0;
  BOOL waitingStat = FALSE;
  DWORD lastErrors;

  if (!myClearCommError(hC0C, &lastErrors))
    SignalStop();

  for (;;) {
    if (stop)
      break;

    if (!waitingRead) {
      DWORD undef;

      if (!ReadFile(hC0C, cbuf, sizeof(cbuf), &undef, &overlaps[EVENT_READ])) {
        DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
          myPTRACE(1, "T38Modem\tReadFile() ERROR " << strError(err));
          SignalStop();
          break;
        }
      }
      waitingRead = TRUE;
    }

    if (!waitingStat) {
      if (!WaitCommEvent(hC0C, &maskStat, &overlaps[EVENT_STAT])) {
        DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
          myPTRACE(1, "T38Modem\tWaitCommEvent() ERROR " << strError(err));
          SignalStop();
          break;
        }
      }
      waitingStat = TRUE;

      DWORD stat;

      if (!GetCommModemStatus(hC0C, &stat)) {
        TraceLastError("GetCommModemStatus()");
        SignalStop();
        break;
      }

      if (!(stat & MS_DSR_ON)) {
        myPTRACE(1, "T38Modem\tDSR is OFF");
        Parent().reset = FALSE;
        SignalStop();
        break;
      }

      DWORD errors;

      if (!myClearCommError(hC0C, &errors)) {
        SignalStop();
        break;
      }

      DWORD changedErrors = lastErrors^errors;

      if (changedErrors & CE_BREAK) {
        if ((errors & CE_BREAK)) {
          myPTRACE(1, "T38Modem\tBREAK is detected");
          SignalStop();
          break;
        }
      }
      lastErrors = errors;
    }

    if (waitingRead && waitingStat) {
      DWORD undef;

      switch (WaitForMultipleObjects(EVENT_NUM, hEvents, FALSE, 5000)) {
      case WAIT_OBJECT_0 + EVENT_READ:
        if (!GetOverlappedResult(hC0C, &overlaps[EVENT_READ], &cbufRead, FALSE)) {
          TraceLastError("GetOverlappedResult(EVENT_READ)");
          SignalStop();
        }
        waitingRead = FALSE;
        break;
      case WAIT_OBJECT_0 + EVENT_STAT:
        if (!GetOverlappedResult(hC0C, &overlaps[EVENT_STAT], &undef, FALSE)) {
          TraceLastError("GetOverlappedResult(EVENT_STAT)");
          SignalStop();
        }
        waitingStat = FALSE;
        myPTRACE(6, "T38Modem\tEVENT_STAT " << hex << maskStat);
        break;
      case WAIT_TIMEOUT:
        break;
      default:
        TraceLastError("WaitForMultipleObjects()");
        SignalStop();
      }
      if (stop)
        break;
    }

    if (!waitingRead && cbufRead) {
      myPTRACE(6, "T38Modem\t--> " << PRTHEX(PBYTEArray((const BYTE *)cbuf, cbufRead)));
      Parent().ToInPtyQ(cbuf, cbufRead);
      cbufRead = 0;
    }
  }

  CancelIo(hC0C);

  CloseEvents(EVENT_NUM, hEvents);

  myPTRACE(1, "T38Modem\t--> Stopped" << GetThreadTimes(", CPU usage: "));
}
///////////////////////////////////////////////////////////////
OutC0C::OutC0C(PseudoModemC0C &_parent, HANDLE _hC0C)
  : UniC0C(_parent, _hC0C)
{
}

void OutC0C::Main()
{
  RenameCurrentThread(Parent().ptyName() + "(o)");
  myPTRACE(1, "T38Modem\t<-- Started");

  enum {
    EVENT_WRITE,
    EVENT_NUM
  };

  HANDLE hEvents[EVENT_NUM];
  OVERLAPPED overlaps[EVENT_NUM];

  if (!PrepareEvents(EVENT_NUM, hEvents, overlaps))
    SignalStop();

  PBYTEArray *buf = NULL;
  PINDEX done = 0;
  DWORD written = 0;
  BOOL waitingWrite = FALSE;

  for(;;) {
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

    if (!waitingWrite) {
      if (!WriteFile(hC0C, (const BYTE *)*buf + done, buf->GetSize() - done, &written, &overlaps[EVENT_WRITE])) {
        DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
          myPTRACE(1, "T38Modem\tWriteFile() ERROR " << strError(err));
          SignalStop();
          break;
        }
        waitingWrite = TRUE;
      }
    }

    if (waitingWrite) {
      switch (WaitForMultipleObjects(EVENT_NUM, hEvents, FALSE, 5000)) {
      case WAIT_OBJECT_0 + EVENT_WRITE:
        if (!GetOverlappedResult(hC0C, &overlaps[EVENT_WRITE], &written, FALSE)) {
          TraceLastError("GetOverlappedResult()");
          SignalStop();
        }
        waitingWrite = FALSE;
        break;
      case WAIT_TIMEOUT:
        myPTRACE(6, "T38Modem\tTIMEOUT");

//#if PTRACING
//        if (PTrace::CanTrace(6)) {
//          DWORD errors;
//          COMSTAT comstat;
//
//          if (ClearCommError(hC0C, &errors, &comstat)) {
//            myPTRACE(1,
//                "T38Modem\terrors="       << hex << errors << dec << " "
//                "fCtsHold="     << comstat.fCtsHold << " "
//                "fDsrHold="     << comstat.fDsrHold << " "
//                "fRlsdHold="    << comstat.fRlsdHold << " "
//                "fXoffHold="    << comstat.fXoffHold << " "
//                "fXoffSent="    << comstat.fXoffSent << " "
//                "fEof="         << comstat.fEof << " "
//                "fTxim="        << comstat.fTxim << " "
//                "cbInQue="      << comstat.cbInQue << " "
//                "cbOutQue="     << comstat.cbOutQue);
//          }
//        }
//#endif

        break;
      default:
        TraceLastError("WaitForMultipleObjects()");
        SignalStop();
      }
      if (stop)
        break;
    }

    if (!waitingWrite && written) {
      myPTRACE(6, "T38Modem\t<-- " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, written)));

      done += written;

      if (buf->GetSize() <= done) {
        if (buf->GetSize() < done) {
          myPTRACE(1, "T38Modem\t<-- " << buf->GetSize() << "(size) < (done)" << done << " " << written);
        }
        delete buf;
        buf = NULL;
      }
      written = 0;
    }
  }

  CancelIo(hC0C);

  if (buf) {
    if (buf->GetSize() != done)
      myPTRACE(1, "T38Modem\t<-- Not sent " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, buf->GetSize() - done)));
    delete buf;
  }

  CloseEvents(EVENT_NUM, hEvents);

  myPTRACE(1, "T38Modem\t<-- Stopped" << GetThreadTimes(", CPU usage: "));
}
///////////////////////////////////////////////////////////////
PseudoModemC0C::PseudoModemC0C(
    const PString &_tty,
    const PString &_route,
    const PConfigArgs &/*args*/,
    const PNotifier &_callbackEndPoint)

  : PseudoModemBody(_tty, _route, _callbackEndPoint),
    hC0C(INVALID_HANDLE_VALUE),
    inC0C(NULL),
    outC0C(NULL),
    ready(FALSE)
{
  if (CheckTty(_tty)) {
    ptypath = _tty;
    ptyname = ptypath.Mid(4);
    valid = TRUE;
  } else {
    myPTRACE(1, "T38Modem\tPseudoModemC0C::PseudoModemC0C bad on " << _tty);
    valid = FALSE;
  }
}

PseudoModemC0C::~PseudoModemC0C()
{
  reset = TRUE;
  StopAll();
  CloseC0C();
}

inline const char *ttyPrefix()
{
  return "\\\\.\\";
}

PBoolean PseudoModemC0C::CheckTty(const PString &_tty)
{
  return _tty.Find(ttyPrefix()) == 0 && _tty != ttyPrefix();
}

PString PseudoModemC0C::ArgSpec()
{
  return "";
}

PStringArray PseudoModemC0C::Description()
{
  PStringArray description;

  description.Append(new PString("Uses serial port to communicate with fax application."));
  description.Append(new PString(PString("The tty format is ") + ttyPrefix() + "port."));

  return description;
}

PBoolean PseudoModemC0C::IsReady() const
{
  return ready && PseudoModemBody::IsReady();
}

const PString &PseudoModemC0C::ttyPath() const
{
  return ptypath;
}

ModemThreadChild *PseudoModemC0C::GetPtyNotifier()
{
  return outC0C;
}

PBoolean PseudoModemC0C::StartAll()
{
  reset = TRUE;

  myPTRACE(3, "T38Modem\tPseudoModemC0C::StartAll");

  if (IsOpenC0C()
     && (inC0C = new InC0C(*this, hC0C)) != NULL
     && (outC0C = new OutC0C(*this, hC0C)) != NULL
     && (PseudoModemBody::StartAll())
     ) {
    inC0C->Resume();
    outC0C->Resume();
    ready = TRUE;
    return TRUE;
  }
  StopAll();
  return FALSE;
}

void PseudoModemC0C::StopAll()
{
  ready = FALSE;

  myPTRACE(3, "T38Modem\tPseudoModemC0C::StopAll reset=" << reset << " stop=" << stop << " childstop=" << childstop);

  if (inC0C) {
    inC0C->SignalStop();
    inC0C->WaitForTermination();
    PWaitAndSignal mutexWait(Mutex);
    delete inC0C;
    inC0C = NULL;
  }
  if (outC0C) {
    outC0C->SignalStop();
    outC0C->WaitForTermination();
    PWaitAndSignal mutexWait(Mutex);
    delete outC0C;
    outC0C = NULL;
  }

  if (reset)
    PseudoModemBody::StopAll();
  else
    childstop = FALSE;
}

BOOL PseudoModemC0C::OpenC0C()
{
  if (IsOpenC0C())
    CloseC0C();

  hC0C = CreateFile(ptypath,
                    GENERIC_READ|GENERIC_WRITE,
                    0,
                    NULL,
                    OPEN_EXISTING,
                    FILE_FLAG_OVERLAPPED,
                    NULL);

  if (hC0C == INVALID_HANDLE_VALUE) {
    TraceLastError(PString("PseudoModemBody::OpenC0C CreateFile(") + ptypath + ")");
    return FALSE;
  }

  DCB dcb;

  if (!myGetCommState(hC0C, &dcb)) {
    CloseC0C();
    return FALSE;
  }

  COMMPROP commProp;

  dcb.XonLim = 64;

  if (GetCommProperties(hC0C, &commProp)) {
    myPTRACE(1, "T38Modem\tCurrentRxQueue=" << commProp.dwCurrentRxQueue);
    dcb.XoffLim = WORD(commProp.dwCurrentRxQueue - 128);
  }

  dcb.BaudRate = CBR_19200;
  dcb.ByteSize = 8;
  dcb.Parity   = NOPARITY;
  dcb.StopBits = ONESTOPBIT;

  dcb.fOutxCtsFlow = TRUE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fDsrSensitivity = TRUE;
  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fTXContinueOnXoff = TRUE;
  dcb.fOutX = FALSE;
  dcb.fInX = TRUE;
  dcb.XonChar = 0x11;
  dcb.XoffChar = 0x13;
  dcb.fParity = FALSE;
  dcb.fNull = FALSE;
  dcb.fAbortOnError = FALSE;
  dcb.fErrorChar = FALSE;

  if (!mySetCommState(hC0C, &dcb)) {
    CloseC0C();
    return FALSE;
  }

  COMMTIMEOUTS timeouts;

  if (!GetCommTimeouts(hC0C, &timeouts)) {
    TraceLastError("PseudoModemBody::OpenC0C GetCommTimeouts()");
    CloseC0C();
    return FALSE;
  }

  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = MAXDWORD - 1;
  timeouts.ReadIntervalTimeout = MAXDWORD;

  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;

  if (!SetCommTimeouts(hC0C, &timeouts)) {
    TraceLastError("PseudoModemBody::OpenC0C SetCommTimeouts()");
    CloseC0C();
    return FALSE;
  }

  myPTRACE(1, "T38Modem\tPseudoModemBody::OpenC0C opened " << ptypath);

  return TRUE;
}

void PseudoModemC0C::CloseC0C()
{
  if (!IsOpenC0C())
    return;

  if (!::CloseHandle(hC0C)) {
    TraceLastError("PseudoModemC0C::CloseC0C() CloseHandle()");
  }

  hC0C = INVALID_HANDLE_VALUE;
}

BOOL PseudoModemC0C::OutPnpId()
{
  DCB org_dcb, dcb;

  if (!myGetCommState(hC0C, &org_dcb))
    return FALSE;

  dcb = org_dcb;

  dcb.BaudRate = CBR_1200;
  dcb.ByteSize = 7;
  dcb.Parity   = NOPARITY;
  dcb.StopBits = ONESTOPBIT;

  if (!mySetCommState(hC0C, &dcb))
    return FALSE;

  enum {
    EVENT_WRITE,
    EVENT_NUM
  };

  HANDLE hEvents[EVENT_NUM];
  OVERLAPPED overlaps[EVENT_NUM];

  if (!PrepareEvents(EVENT_NUM, hEvents, overlaps))
    return FALSE;

  static char devID[] =
    "("
    "\x01\x24"                             // PnP revision 1.0
    "VAF"                                  // EISA ID
    "0001"                                 // Product ID
    "\\00000001"                           // Serial Number
    "\\MODEM"                              // Class Name
    "\\PNPC101"                            // Driver ID
    "\\t38modem"                           // User Name
    "??"                                   // CheckSum
    ")";

  static int lenDevID = sizeof(devID) - 1;

  devID[lenDevID - 3] = devID[lenDevID - 2] = 0;

  int checkSum = 0;

  for (int i = 0 ; i < lenDevID ; i++) {
    checkSum += devID[i];
  }

  myPTRACE(1, "T38Modem\tcheckSum = 0x" << hex << (checkSum & 0xFF));

  static char digs[] = "0123456789ABCDEF";

  devID[lenDevID - 3] = digs[(checkSum >> 4) & 0xF];
  devID[lenDevID - 2] = digs[checkSum & 0xF];

  PBYTEArray *buf = new PBYTEArray((const BYTE *)devID, lenDevID);
  PINDEX done = 0;
  DWORD written = 0;
  BOOL waitingWrite = FALSE;

  if (!waitingWrite) {
    if (!WriteFile(hC0C, (const BYTE *)*buf + done, buf->GetSize() - done, &written, &overlaps[EVENT_WRITE])) {
      DWORD err = ::GetLastError();
      if (err == ERROR_IO_PENDING) {
        waitingWrite = TRUE;
      } else {
        myPTRACE(1, "T38Modem\tWriteFile() ERROR " << strError(err));
      }
    }
  }

  if (waitingWrite) {
    switch (WaitForMultipleObjects(EVENT_NUM, hEvents, FALSE, 5000)) {
    case WAIT_OBJECT_0 + EVENT_WRITE:
      if (!GetOverlappedResult(hC0C, &overlaps[EVENT_WRITE], &written, FALSE)) {
        TraceLastError("GetOverlappedResult()");
      }
      waitingWrite = FALSE;
    break;
    case WAIT_TIMEOUT:
      myPTRACE(6, "T38Modem\tTIMEOUT");
      break;
    default:
      TraceLastError("WaitForMultipleObjects()");
    }
  }

  if (!waitingWrite && written) {
    myPTRACE(6, "T38Modem\t<-- " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, written)));

    done += written;

    if (buf->GetSize() <= done) {
      if (buf->GetSize() < done) {
        myPTRACE(1, "T38Modem\t<-- " << buf->GetSize() << "(size) < (done)" << done << " " << written);
      }
      delete buf;
      buf = NULL;
    }
  }

  Sleep((lenDevID*10*1000)/1200);

  CancelIo(hC0C);

  if (buf) {
    if (buf->GetSize() != done)
      myPTRACE(1, "T38Modem\t<-- Not sent " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, buf->GetSize() - done)));
    delete buf;
  }

  CloseEvents(EVENT_NUM, hEvents);

  if (!mySetCommState(hC0C, &org_dcb))
    return FALSE;

  return TRUE;
}

BOOL PseudoModemC0C::WaitReady()
{
  enum {
    EVENT_STAT,
    EVENT_NUM
  };

  HANDLE hEvents[EVENT_NUM];
  OVERLAPPED overlaps[EVENT_NUM];

  if (!PrepareEvents(EVENT_NUM, hEvents, overlaps))
    return FALSE;

  BOOL fault = FALSE;

  if (!SetCommMask(hC0C, EV_CTS|EV_DSR)) {
    TraceLastError("SetCommMask()");
    fault = TRUE;
  }

  DWORD maskStat = 0;
  BOOL waitingStat = FALSE;
  DWORD lastStat = 0;
  BOOL enumerator = FALSE;
  PTime TimeDSR;

  for (;;) {
    if (fault)
      break;

    if (!waitingStat) {
      if (!WaitCommEvent(hC0C, &maskStat, &overlaps[EVENT_STAT])) {
        DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
          myPTRACE(1, "T38Modem\tWaitCommEvent() ERROR " << strError(err));
          fault = TRUE;
          break;
        }
        waitingStat = TRUE;
      }

      DWORD stat;

      if (!GetCommModemStatus(hC0C, &stat)) {
        TraceLastError("GetCommModemStatus()");
        fault = TRUE;
        break;
      }

      DWORD changedStat = lastStat^stat;

      if (stat & MS_DSR_ON) {
        if (changedStat & MS_DSR_ON)
          TimeDSR = PTime();

        if (stat & MS_CTS_ON) {
          PInt64 msSinceDSR = (PTime() - TimeDSR).GetMilliSeconds();

          if (msSinceDSR > 150 && msSinceDSR < 250) {
            myPTRACE(1, "T38Modem\tPnP Enumerator detected");
            enumerator = TRUE;
          }

          break;
        }
      }
      lastStat = stat;
    }

    if (waitingStat) {
      DWORD undef;

      switch (WaitForMultipleObjects(EVENT_NUM, hEvents, FALSE, 5000)) {
      case WAIT_OBJECT_0 + EVENT_STAT:
        if (!GetOverlappedResult(hC0C, &overlaps[EVENT_STAT], &undef, FALSE)) {
          TraceLastError("GetOverlappedResult(EVENT_STAT)");
          fault = TRUE;
        }
        waitingStat = FALSE;
        myPTRACE(6, "T38Modem\tEVENT_STAT " << hex << maskStat);
        break;
      case WAIT_TIMEOUT:
        break;
      default:
        TraceLastError("WaitForMultipleObjects()");
        fault = TRUE;
      }
    }
  }

  CancelIo(hC0C);

  CloseEvents(EVENT_NUM, hEvents);

  if (!fault && enumerator)
    fault = !OutPnpId();

  return !fault;
}

void PseudoModemC0C::MainLoop()
{
  if (AddModem() && OpenC0C()) {
    while (!stop && WaitReady() && StartAll()) {
      while (!stop && !childstop) {
        WaitDataReady();
      }
      StopAll();
    }
    CloseC0C();
  }
}
///////////////////////////////////////////////////////////////

#endif // MODEM_DRIVER_C0C

