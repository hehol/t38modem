/*
 * drv_c0c.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2004 Vyacheslav Frolov
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
 * Revision 1.1  2004-07-07 13:36:46  vfrolov
 * Initial revision
 *
 * Revision 1.1  2004/07/07 13:36:46  vfrolov
 * Initial revision
 *
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
static PString strError(DWORD err)
{
  return PString(strerror(err)) + " (" + PString(err) + ")";
}
///////////////////////////////////////////////////////////////
static BOOL PrepareEvents(int num, HANDLE *hEvents, OVERLAPPED *overlaps)
{
  memset(hEvents, 0, num * sizeof(HANDLE));
  memset(overlaps, 0, num * sizeof(OVERLAPPED));

  for (int i = 0 ; i < num ; i++) {
    overlaps[i].hEvent = hEvents[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (!hEvents[i]) {
      DWORD err = ::GetLastError();
      myPTRACE(1, "PrepareEvents() CreateEvent() ERROR " << strError(err));
      return FALSE;
    }
  }
  return TRUE;
}

static void CloseEvents(int num, HANDLE *hEvents)
{
  for (int i = 0 ; i < num ; i++) {
    if (hEvents[i])
      CloseHandle(hEvents[i]);
  }
}

static BOOL myClearCommError(HANDLE hC0C, DWORD *pErrors)
{
  if (!ClearCommError(hC0C, pErrors, NULL)) {
    DWORD err = ::GetLastError();
    myPTRACE(1, "ClearCommError() ERROR " << strError(err));
    return FALSE;
  }
  return TRUE;
}

static BOOL myGetCommState(HANDLE hC0C, DCB *dcb)
{
  dcb->DCBlength = sizeof(*dcb);

  if (!GetCommState(hC0C, dcb)) {
    DWORD err = ::GetLastError();
    myPTRACE(1, "GetCommState() ERROR " << strError(err));
    return FALSE;
  }
  return TRUE;
}

static BOOL mySetCommState(HANDLE hC0C, DCB *dcb)
{
  if (!SetCommState(hC0C, dcb)) {
    DWORD err = ::GetLastError();
    myPTRACE(1, "SetCommState() ERROR " << strError(err));
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
  myPTRACE(1, "--> Started");

  enum {
    EVENT_READ,
    EVENT_STAT,
    EVENT_NUM
  };

  HANDLE hEvents[EVENT_NUM];
  OVERLAPPED overlaps[EVENT_NUM];

  if (!PrepareEvents(EVENT_NUM, hEvents, overlaps))
    SignalStop();

  if (!SetCommMask(hC0C, EV_CTS|EV_DSR|EV_BREAK)) {
    DWORD err = ::GetLastError();
    myPTRACE(1, "SetCommMask() ERROR " << strError(err));
    SignalStop();
  }

  char cbuf[1024];
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
      if (!ReadFile(hC0C, cbuf, 1024, &cbufRead, &overlaps[EVENT_READ])) {
        DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
          myPTRACE(1, "ReadFile() ERROR " << strError(err));
          SignalStop();
          break;
        }
        waitingRead = TRUE;
      }
    }

    if (!waitingStat) {
      if (!WaitCommEvent(hC0C, &maskStat, &overlaps[EVENT_STAT])) {
        DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
          myPTRACE(1, "WaitCommEvent() ERROR " << strError(err));
          SignalStop();
          break;
        }
        waitingStat = TRUE;
      }

      DWORD stat;

      if (!GetCommModemStatus(hC0C, &stat)) {
        DWORD err = ::GetLastError();
        myPTRACE(1, "GetCommModemStatus() ERROR " << strError(err));
        SignalStop();
        break;
      }

      if (!(stat & MS_DSR_ON)) {
        myPTRACE(1, "DSR is OFF");
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
          myPTRACE(1, "BREAK is detected");
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
          DWORD err = ::GetLastError();
          myPTRACE(1, "GetOverlappedResult(EVENT_READ) ERROR " << strError(err));
          SignalStop();
        }
        waitingRead = FALSE;
        break;
      case WAIT_OBJECT_0 + EVENT_STAT:
        if (!GetOverlappedResult(hC0C, &overlaps[EVENT_STAT], &undef, FALSE)) {
          DWORD err = ::GetLastError();
          myPTRACE(1, "GetOverlappedResult(EVENT_STAT) ERROR " << strError(err));
          SignalStop();
        }
        waitingStat = FALSE;
        PTRACE(6, "EVENT_STAT " << hex << maskStat);
        break;
      case WAIT_TIMEOUT:
        break;                       
      default:
        DWORD err = ::GetLastError();
        myPTRACE(1, "WaitForMultipleObjects() ERROR " << strError(err));
        SignalStop();
      }
      if (stop)
        break;
    }

    if (!waitingRead && cbufRead) {
      PTRACE(6, "--> " << PRTHEX(PBYTEArray((const BYTE *)cbuf, cbufRead)));
      Parent().ToInPtyQ(cbuf, cbufRead);
      cbufRead = 0;
    }
  }

  CancelIo(hC0C);

  CloseEvents(EVENT_NUM, hEvents);

  myPTRACE(1, "--> Stopped" << GetThreadTimes(", CPU usage: "));
}
///////////////////////////////////////////////////////////////
OutC0C::OutC0C(PseudoModemC0C &_parent, HANDLE _hC0C)
  : UniC0C(_parent, _hC0C)
{
}

void OutC0C::Main()
{
  RenameCurrentThread(Parent().ptyName() + "(o)");
  myPTRACE(1, "<-- Started");

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
    while (buf == NULL) {
      if (stop)
        break;
      WaitDataReady();
      if (stop)
        break;
      buf = Parent().FromOutPtyQ();
      done = 0;
    }

    if (stop)
      break;

    if (!waitingWrite) {
      if (!WriteFile(hC0C, (const BYTE *)*buf + done, buf->GetSize() - done, &written, &overlaps[EVENT_WRITE])) {
        DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
          myPTRACE(1, "WriteFile() ERROR " << strError(err));
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
          DWORD err = ::GetLastError();
          myPTRACE(1, "GetOverlappedResult() ERROR " << strError(err));
          SignalStop();
        }
        waitingWrite = FALSE;
        break;
      case WAIT_TIMEOUT:
        myPTRACE(6, "TIMEOUT");
        break;                       
      default:
        DWORD err = ::GetLastError();
        myPTRACE(1, "WaitForMultipleObjects() ERROR " << strError(err));
        SignalStop();
      }
      if (stop)
        break;
    }

    if (!waitingWrite && written) {
      PTRACE(6, "<-- " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, written)));

      done += written;

      if (buf->GetSize() <= done) {
        if (buf->GetSize() < done) {
          myPTRACE(1, "<-- " << buf->GetSize() << "(size) < (done)" << done << " " << written);
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
      myPTRACE(1, "<-- Not sent " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, buf->GetSize() - done)));
    delete buf;
  }

  CloseEvents(EVENT_NUM, hEvents);

  myPTRACE(1, "<-- Stopped" << GetThreadTimes(", CPU usage: "));
}
///////////////////////////////////////////////////////////////
PseudoModemC0C::PseudoModemC0C(const PString &_tty, const PString &_route, const PNotifier &_callbackEndPoint)
  : PseudoModemBody(_route, _callbackEndPoint),
    hC0C(INVALID_HANDLE_VALUE),
    inC0C(NULL),
    outC0C(NULL)
{
  if (CheckTty(_tty)) {
    ptypath = _tty;
    ptyname = &ptypath[4];
    valid = TRUE;
  } else {
    myPTRACE(1, "PseudoModemC0C::PseudoModemC0C bad on " << _tty);
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

BOOL PseudoModemC0C::CheckTty(const PString &_tty)
{
  return _tty.Find(ttyPrefix()) == 0 && _tty != ttyPrefix();
}

PStringArray PseudoModemC0C::Description()
{
  PStringArray description;

  description.Append(new PString("Uses serial port to communicate with fax application."));
  description.Append(new PString(PString("The tty format is ") + ttyPrefix() + "port."));

  return description;
}

const PString &PseudoModemC0C::ttyPath() const
{
  return ptypath;
}

ModemThreadChild *PseudoModemC0C::GetPtyNotifier()
{
  return outC0C;
}

BOOL PseudoModemC0C::StartAll()
{
  reset = TRUE;

  if (IsOpenC0C()
     && (inC0C = new InC0C(*this, hC0C)) != NULL
     && (outC0C = new OutC0C(*this, hC0C)) != NULL
     && (PseudoModemBody::StartAll())
     ) {
    inC0C->Resume();
    outC0C->Resume();
    return TRUE;
  }
  StopAll();
  return FALSE;
}

void PseudoModemC0C::StopAll()
{
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
    DWORD err = ::GetLastError();
    myPTRACE(1, "PseudoModemBody::OpenC0C CreateFile(" << ptypath << ") ERROR " << strError(err));
    return FALSE;
  }

  DCB dcb;

  if (!myGetCommState(hC0C, &dcb))
    return FALSE;

  dcb.BaudRate = CBR_19200;
  dcb.ByteSize = 8;
  dcb.Parity   = NOPARITY;
  dcb.StopBits = ONESTOPBIT;

  dcb.fOutxCtsFlow = TRUE;
  dcb.fOutxDsrFlow = FALSE;
  dcb.fDsrSensitivity = TRUE;
  dcb.fRtsControl = RTS_CONTROL_HANDSHAKE;
  dcb.fDtrControl = DTR_CONTROL_ENABLE;
  dcb.fOutX = FALSE;
  dcb.fInX = FALSE;
  dcb.fParity = FALSE;
  dcb.fNull = FALSE;

  if (!mySetCommState(hC0C, &dcb))
    return FALSE;

  COMMTIMEOUTS timeouts;

  if (!GetCommTimeouts(hC0C, &timeouts)) {
    DWORD err = ::GetLastError();
    myPTRACE(1, "PseudoModemBody::OpenC0C GetCommTimeouts() ERROR " << strError(err));
    return FALSE;
  }

  timeouts.ReadIntervalTimeout = MAXDWORD;
  timeouts.ReadTotalTimeoutMultiplier = MAXDWORD;
  timeouts.ReadTotalTimeoutConstant = MAXDWORD - 1;
  timeouts.ReadIntervalTimeout = MAXDWORD;

  timeouts.WriteTotalTimeoutMultiplier = 0;
  timeouts.WriteTotalTimeoutConstant = 0;

  if (!SetCommTimeouts(hC0C, &timeouts)) {
    DWORD err = ::GetLastError();
    myPTRACE(1, "PseudoModemBody::OpenC0C SetCommTimeouts() ERROR " << strError(err));
    return FALSE;
  }

  myPTRACE(1, "PseudoModemBody::OpenC0C opened " << ptypath);

  return TRUE;
}

void PseudoModemC0C::CloseC0C()
{
  if (!IsOpenC0C())
    return;

  if (!::CloseHandle(hC0C)) {
    DWORD err = ::GetLastError();
    myPTRACE(1, "PseudoModemC0C::CloseC0C close " << " ERROR:" << strError(err));
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

  myPTRACE(1, "checkSum = 0x" << hex << (checkSum & 0xFF));

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
      if (err != ERROR_IO_PENDING) {
        myPTRACE(1, "WriteFile() ERROR " << strError(err));
      }
      waitingWrite = TRUE;
    }
  }

  if (waitingWrite) {
    switch (WaitForMultipleObjects(EVENT_NUM, hEvents, FALSE, 5000)) {
    case WAIT_OBJECT_0 + EVENT_WRITE:
      if (!GetOverlappedResult(hC0C, &overlaps[EVENT_WRITE], &written, FALSE)) {
        DWORD err = ::GetLastError();
        myPTRACE(1, "GetOverlappedResult() ERROR " << strError(err));
      }
      waitingWrite = FALSE;
    break;
    case WAIT_TIMEOUT:
      myPTRACE(6, "TIMEOUT");
      break;                       
    default:
      DWORD err = ::GetLastError();
      myPTRACE(1, "WaitForMultipleObjects() ERROR " << strError(err));
    }
  }

  if (!waitingWrite && written) {
    myPTRACE(6, "<-- " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, written)));

    done += written;

    if (buf->GetSize() <= done) {
      if (buf->GetSize() < done) {
        myPTRACE(1, "<-- " << buf->GetSize() << "(size) < (done)" << done << " " << written);
      }
      delete buf;
      buf = NULL;
    }
  }

  Sleep((lenDevID*10*1000)/1200);

  CancelIo(hC0C);

  if (buf) {
    if (buf->GetSize() != done)
      myPTRACE(1, "<-- Not sent " << PRTHEX(PBYTEArray((const BYTE *)*buf + done, buf->GetSize() - done)));
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

  if (!SetCommMask(hC0C, EV_CTS|EV_DSR)) {
    DWORD err = ::GetLastError();
    myPTRACE(1, "--> SetCommMask() ERROR " << strError(err));
    SignalStop();
  }

  DWORD maskStat = 0;
  BOOL waitingStat = FALSE;
  DWORD lastStat = 0;
  BOOL fault = FALSE;
  BOOL enumerator = FALSE;
  PTime TimeDSR;

  for (;;) {
    if (!waitingStat) {
      if (!WaitCommEvent(hC0C, &maskStat, &overlaps[EVENT_STAT])) {
        DWORD err = ::GetLastError();
        if (err != ERROR_IO_PENDING) {
          myPTRACE(1, "WaitCommEvent() ERROR " << strError(err));
          fault = TRUE;
          break;
        }
        waitingStat = TRUE;
      }

      DWORD stat;

      if (!GetCommModemStatus(hC0C, &stat)) {
        DWORD err = ::GetLastError();
        myPTRACE(1, "GetCommModemStatus() ERROR " << strError(err));
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
            myPTRACE(1, "PnP Enumerator detected");
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
          DWORD err = ::GetLastError();
          myPTRACE(1, "GetOverlappedResult(EVENT_STAT) ERROR " << strError(err));
          fault = TRUE;
        }
        waitingStat = FALSE;
        myPTRACE(6, "EVENT_STAT " << hex << maskStat);
        break;
      case WAIT_TIMEOUT:
        break;                       
      default:
        DWORD err = ::GetLastError();
        myPTRACE(1, "WaitForMultipleObjects() ERROR " << strError(err));
        fault = TRUE;
      }
      if (fault)
        break;
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

