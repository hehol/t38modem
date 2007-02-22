/*
 * pmodeme.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2007 Vyacheslav Frolov
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
 * $Log: pmodeme.cxx,v $
 * Revision 1.35  2007-02-22 16:00:33  vfrolov
 * Implemented AT#HCLR command
 *
 * Revision 1.35  2007/02/22 16:00:33  vfrolov
 * Implemented AT#HCLR command
 *
 * Revision 1.34  2006/12/11 11:19:48  vfrolov
 * Fixed race condition with modem Callback
 *
 * Revision 1.33  2006/12/07 10:53:24  vfrolov
 * Added OnParentStop()
 *
 * Revision 1.32  2006/12/01 13:35:25  vfrolov
 * Fixed modem locking after unexpected dial while connection
 *
 * Revision 1.31  2005/11/22 16:39:36  vfrolov
 * Fixed MSVC compile warning
 *
 * Revision 1.30  2005/03/05 15:42:39  vfrolov
 * Added missing check for PTRACING
 * Fixed typo in T38DLE trace
 *
 * Revision 1.29  2005/03/04 16:35:38  vfrolov
 * Implemented AT#DFRMC command
 * Redisigned class Profile
 *
 * Revision 1.28  2005/02/16 12:14:47  vfrolov
 * Send CONNECT just before data for AT+FRM command
 *
 * Revision 1.27  2005/02/10 10:35:18  vfrolov
 * Fixed AT prefix searching
 *
 * Revision 1.26  2005/02/03 11:32:11  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.25  2005/02/01 11:43:46  vfrolov
 * Implemented ATV0 command (numeric format for result codes)
 * Implemented AT+FMI?, AT+FMM? and AT+FMR? commands
 * Added stubs for ATBn, ATX3 and AT+FCLASS=0 commands
 * Added stub for AT+FLO command
 *
 * Revision 1.24  2004/10/27 13:36:26  vfrolov
 * Decreased binary, DLE, and callback tracing
 *
 * Revision 1.23  2004/07/06 16:07:24  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.22  2004/06/24 17:20:22  vfrolov
 * Added stub for ATXn command
 *
 * Revision 1.21  2004/05/09 07:46:11  csoutheren
 * Updated to compile with new PIsDescendant function
 *
 * Revision 1.20  2004/03/01 17:14:34  vfrolov
 * Fixed binary log in command mode
 *
 * Revision 1.19  2003/12/04 16:09:51  vfrolov
 * Implemented FCS generation
 * Implemented ECM support
 *
 * Revision 1.18  2003/01/08 16:58:58  vfrolov
 * Added cbpOutBufNoFull and isOutBufFull()
 *
 * Revision 1.17  2002/12/30 12:49:29  vfrolov
 * Added tracing thread's CPU usage (Linux only)
 *
 * Revision 1.16  2002/12/20 10:12:50  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.15  2002/12/19 10:31:33  vfrolov
 * Changed usage multiple dial modifiers 'L' (for secure reasons)
 *   each next 'L' overrides previous 'L'
 *   ("ATD4444L123L456" eq "ATD4444L456", "ATD4444L123L" eq "ATD4444")
 * Added dial modifier 'D' - continue dial number
 *   ("ATD000L123D4444" eq "ATD0004444L123")
 * Added mising spaces into "NMBR = " and "NDID = "
 *
 * Revision 1.14  2002/11/05 13:59:11  vfrolov
 * Implemented Local Party Number dial modifier 'L'
 * (put dial string 1234L5678 to dial 1234 from 5678)
 *
 * Revision 1.13  2002/05/15 16:10:52  vfrolov
 * Reimplemented AT+FTS and AT+FRS
 * Added workaround "Reset state stSendAckWait"
 *
 * Revision 1.12  2002/05/07 10:36:16  vfrolov
 * Added more code for case stResetHandle
 * Changed duration of CED (T.30 requires 2.6...4.0 secs)
 *
 * Revision 1.11  2002/04/19 14:06:04  vfrolov
 * Implemented T.38 mode request dial modifiers
 *   F - enable
 *   V - disable
 *
 * Revision 1.10  2002/04/03 02:45:36  vfrolov
 * Implemented AT#CID=10 - ANI/DNIS reporting between RINGs
 *
 * Revision 1.9  2002/03/01 14:59:48  vfrolov
 * Get data for Revision string from version.h
 *
 * Revision 1.8  2002/03/01 10:00:26  vfrolov
 * Added Copyright header
 * Implemented connection established handling and mode change request
 * Implemented ATI8 command
 * Fixed some deadlocks
 * Added some other changes
 *
 * Revision 1.7  2002/02/11 08:40:15  vfrolov
 * More clear call implementation
 *
 * Revision 1.6  2002/01/10 06:10:02  craigs
 * Added MPL header
 *
 * Revision 1.5  2002/01/06 03:48:45  craigs
 * Added changes to support efax 0.9
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.4  2002/01/03 21:36:00  craigs
 * Added change to use S1 register for number of rings on answer
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.3  2002/01/02 04:49:37  craigs
 * Added support for ATS0 register
 *
 * Revision 1.2  2002/01/01 23:59:52  craigs
 * Lots of additional implementation thanks to Vyacheslav Frolov
 *
 */

#include <ptlib.h>
#include "pmodemi.h"
#include "pmodeme.h"
#include "dle.h"
#include "fcs.h"
#include "t38engine.h"
#include "version.h"

///////////////////////////////////////////////////////////////
static const char Manufacturer[] = "Vyacheslav Frolov";
static const char Model[] = "T38FAX";
#define _TOSTR(s) #s
#define TOSTR(s) _TOSTR(s)
static const char Revision[] = TOSTR(MAJOR_VERSION) "." TOSTR(MINOR_VERSION) "." TOSTR(BUILD_NUMBER);
///////////////////////////////////////////////////////////////
#define DeclareStringParam(name)	\
  public:			\
    void name(const PString &_s##name) { s##name = _s##name; }	\
    const PString &name() const { return s##name; }		\
  protected:			\
    PString s##name;
///////////////////////////////////////////////////////////////
class Profile
{
  enum {
    MaxReg = 50,
    MaxBit = 7
  };

  public:
    Profile();

    #define DeclareRegisterBit(name, byte, bit)	\
      void name(BOOL val) { SetBit(byte, bit, val); } \
      BOOL name() const { BOOL val; GetBit(byte, bit, val); return val; }

    DeclareRegisterBit(Echo, 23, 0);
    DeclareRegisterBit(asciiResultCodes, 23, 6);
    DeclareRegisterBit(noResultCodes, 23, 7);

    #define DeclareRegisterByte(name, byte)	\
      void name(BYTE val) { SetReg(byte, val); } \
      BYTE name() const { BYTE val; GetReg(byte, val); return val; }

    DeclareRegisterByte(ClearMode, 47);
    DeclareRegisterByte(DelayFrmConnect, 48);
    DeclareRegisterByte(Flo, 49);
    DeclareRegisterByte(CidMode, 50);

    BOOL SetBit(PINDEX r, PINDEX b, BOOL val) {
      if( !ChkRB(r, b) ) return FALSE;
      BYTE msk = MaskB(b);
      if( val ) S[r] |= msk;
      else      S[r] &= ~msk;
      return TRUE;
    }
    BOOL GetBit(PINDEX r, PINDEX b, BOOL &val) const {
      if (!ChkRB(r, b)) {
        val = 0;
        return FALSE;
      }
      BYTE msk = MaskB(b);
      val = (S[r] & msk) ? TRUE : FALSE;
      return TRUE;
    }
    BOOL SetReg(PINDEX r, BYTE val) {
      if( !ChkR(r) ) return FALSE;
      S[r] = val;
      return TRUE;
    }
    BOOL GetReg(PINDEX r, BYTE &val) const {
      if (!ChkR(r)) {
        val = 0;
        return FALSE;
      }
      val = S[r];
      return TRUE;
    }
    BOOL SetBits(PINDEX r, PINDEX bl, PINDEX bh, BYTE val) {
      if( !ChkRBB(r, bl, bh) ) return FALSE;
      BYTE msk = MaskBB(bl, bh);
      S[r] &= ~msk;
      S[r] &= (val << bl) & msk;
      return TRUE;
    }
    BOOL GetBits(PINDEX r, PINDEX bl, PINDEX bh, BYTE &val) const {
      if (!ChkRBB(r, bl, bh)) {
        val = 0;
        return FALSE;
      }
      BYTE msk = MaskBB(bl, bh);
      val = BYTE((S[r] & msk) >> bl);
      return TRUE;
    }
    
    Profile &operator=(const Profile &p);

  protected:
    static BOOL ChkR(PINDEX r) {
      return r <= MaxReg;
    }
    static BOOL ChkB(PINDEX b) {
      return b <= MaxBit;
    }
    static BOOL ChkRB(PINDEX r, PINDEX b) {
      return ChkR(r) && ChkB(b);
    }
    static BOOL ChkRBB(PINDEX r, PINDEX bl, PINDEX bh) {
      return ChkR(r) && ChkB(bl) && ChkB(bh) && bl <= bh;
    }
    static BYTE MaskB(PINDEX b) {
      return "\x01\x02\x04\x08\x10\x20\x40\x80"[b];
    }
    static BYTE MaskBB(PINDEX bl, PINDEX bh) { // bl <= bh
      return BYTE(("\x01\x03\x07\x0F\x1F\x3F\x7F\xFF"[bh - bl]) << bl);
    }
    
    BYTE S[MaxReg + 1];	// S-registers
    
    DeclareStringParam(ModemClass)
};
///////////////////////////////////////////////////////////////
class Timeout : public PTimer
{
    PCLASSINFO(Timeout, PTimer);
  public:
    Timeout(const PNotifier &callback, BOOL _continuous = FALSE) 
        : state(0), continuous(_continuous) { 
      SetNotifier(callback);
    }
    
    void Start(unsigned period) {
      PWaitAndSignal mutexWait(Mutex);
      state = 1;
      if( continuous ) {
        RunContinuous(period);
        OnTimeout();
      } else {
        PTimer::operator=(period);
      }
    }
    
    void Stop() {
      PWaitAndSignal mutexWait(Mutex);
      state = 0;
      PTimer::operator=(0);
    }
    
    BOOL Get() { 
      PWaitAndSignal mutexWait(Mutex); 
      if( state == 2 ) { 
        state = continuous ? 1 : 0;
        return TRUE;
      }
      return FALSE;
    }
  protected:
    void OnTimeout() {
      PWaitAndSignal mutexWait(Mutex);
      if( state == 1 )
        state = 2;
      PTimer::OnTimeout();
    }
  
    int state;
    BOOL continuous;
    PMutex Mutex;
};
///////////////////////////////////////////////////////////////
#define DeclareResultCode(name, v0, v1)	\
  PString name() const { return P.asciiResultCodes() ? (v1) : (v0); }
///////////////////////////////////////////////////////////////
class ModemEngineBody : public PObject
{
    PCLASSINFO(ModemEngineBody, PObject);
  public:
    enum {
      stResetHandle,
      stCommand,
      stConnectWait,
      stConnectHandle,
      stReqModeAckWait,
      stReqModeAckHandle,
      stSend,
      stSendBufEmptyHandle,
      stSendAckWait,
      stSendAckHandle,
      stRecvBegWait,
      stRecvBegHandle,
      stRecv,
    };

    enum {
      cdUndefined,
      cdOutgoing,
      cdIncoming,
    };
    
    enum {
      chEvent,
      chDelay,
      chHandle,
    };

  /**@name Construction */
  //@{
    ModemEngineBody(ModemEngine &_parent, const PNotifier &_callbackEndPoint);
    ~ModemEngineBody();
  //@}

  /**@name Operations */
  //@{
    BOOL Request(PStringToString &request);
    BOOL Attach(T38Engine *_t38engine);
    void Detach(T38Engine *_t38engine) { PWaitAndSignal mutexWait(Mutex); _Detach(_t38engine); }
    void OnParentStop();
    void HandleData(const PBYTEArray &buf, PBYTEArray &bresp);
    void CheckState(PBYTEArray &bresp);
    BOOL IsReady() const {
      PWaitAndSignal mutexWait(Mutex);
      return state == stCommand && (PTime() - lastPtyActivity) > 5*1000;
    }
    BOOL isOutBufFull() const {
      PWaitAndSignal mutexWait(Mutex);
      return t38engine && t38engine->isOutBufFull();
    }
  //@}

  protected:
    BOOL Echo() const { return P.Echo(); }
    BOOL HandleClass1Cmd(const char **ppCmd, PString &resp, BOOL &ok, BOOL &crlf);
    void HandleCmd(const PString &cmd, PString &resp);

    void ResetDleData() {
      dleData.Clean();
      dleData.BitRev(TRUE /*dataType == T38Engine::dtHdlc*/);
      dataCount = 0;
      moreFrames = FALSE;
    }
    
    BOOL SendStart(int _dataType, int br, PString &resp) {
      PWaitAndSignal mutexWait(Mutex);
      dataType = _dataType;
      ResetDleData();
      state = stSend;
      if( t38engine && t38engine->SendStart(dataType, br) ) {
        resp = RC_CONNECT();
        return TRUE;
      } else {
        state = stCommand;
        return FALSE;
      }
    }

    BOOL RecvStart(int _dataType, int br) {
      BOOL done = FALSE;

      PWaitAndSignal mutexWait(Mutex);
      dataType = _dataType;
      state = stRecvBegWait;
      timeout.Start(60000);
      if (!t38engine || !t38engine->RecvWait(dataType, br, NextSeq(), done)) {
        state = stCommand;
        timeout.Stop();
        return FALSE;
      }

      if (done) {
        state = stRecvBegHandle;
        timeout.Stop();
        parent.SignalDataReady();
      }

      return TRUE;
    }

    void _Detach(T38Engine *_t38engine);
    void _ClearCall();

    int NextSeq() { return seq = ++seq % T38Engine::cbpUserDataMod; }

    ModemEngine &parent;
    T38Engine *t38engine;
    const PNotifier callbackEndPoint;

    PDECLARE_NOTIFIER(PObject, ModemEngineBody, OnMyCallback);
    const PNotifier myCallback;
    Timeout timerRing;
    Timeout timeout;
    PTime lastPtyActivity;

    int seq;

    int callDirection;
    BOOL forceFaxMode;
    BOOL connectionEstablished;
    int state;
    int param;
    PString cmd;
    int dataType;

    DLEData dleData;
    PINDEX dataCount;
    BOOL moreFrames;
    FCS fcs;

    Profile P;
    Profile Profiles[1];

    PMutex Mutex;
    
    DeclareStringParam(CallToken)
    DeclareStringParam(SrcNum)
    DeclareStringParam(DstNum)

    DeclareResultCode(RC_PREF,            "", "\r\n")

    DeclareResultCode(RC_OK,           "0\r", "OK\r\n")
    DeclareResultCode(RC_CONNECT,      "1\r", "CONNECT\r\n")
    DeclareResultCode(RC_RING,         "2\r", "RING\r\n")
    DeclareResultCode(RC_NO_CARRIER,   "3\r", "NO CARRIER\r\n")
    DeclareResultCode(RC_ERROR,        "4\r", "ERROR\r\n")
    DeclareResultCode(RC_CONNECT_1200, "5\r", "CONNECT 1200\r\n")
    DeclareResultCode(RC_NO_DIALTONE,  "6\r", "NO DIALTONE\r\n")
    DeclareResultCode(RC_BUSY,         "7\r", "BUSY\r\n")
    DeclareResultCode(RC_NO_ANSWER,    "8\r", "NO ANSWER\r\n")
    DeclareResultCode(RC_RINGING,      "9\r", "RINGING\r\n")
    DeclareResultCode(RC_FCERROR,    "+F4\r", "+FCERROR\r\n")
};
///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
ModemEngine::ModemEngine(PseudoModemBody &_parent)
  : ModemThreadChild(_parent)
{
  body = new ModemEngineBody(*this, Parent().GetCallbackEndPoint());
}

ModemEngine::~ModemEngine()
{
  if( body )
    delete body;
}

BOOL ModemEngine::IsReady() const
{
  return body && body->IsReady();
}

BOOL ModemEngine::Attach(T38Engine *t38engine) const
{
  return body && body->Attach(t38engine);
}

void ModemEngine::Detach(T38Engine *t38engine) const
{
  if( body )
    body->Detach(t38engine);
}

BOOL ModemEngine::Request(PStringToString &request) const
{
  return body && body->Request(request);
}

void ModemEngine::Main()
{
  RenameCurrentThread(ptyName() + "(e)");

  myPTRACE(1, "<-> Started");
  if( !body ) {
    myPTRACE(1, "<-> no body" << ptyName());
    SignalStop();
    return;
  }
  
  for(;;) {
    PBYTEArray bresp;
    
    if (stop)
      break;

    body->CheckState(bresp);

    if (stop)
      break;

    while( !body->isOutBufFull() ) {
      PBYTEArray *buf = Parent().FromInPtyQ();

      if (buf)  {
        body->HandleData(*buf, bresp);
        delete buf;
        if (stop)
          break;
      } else
          break;
    }

    if (stop)
      break;

    if (bresp.GetSize()) {
      ToPtyQ(bresp, bresp.GetSize());
    }

    if (stop)
      break;

    WaitDataReady();
  }

  body->OnParentStop();

  myPTRACE(1, "<-> Stopped" << GetThreadTimes(", CPU usage: "));
}

///////////////////////////////////////////////////////////////
Profile::Profile() {
  for( PINDEX r = 0 ; r <= MaxReg ; r++ ) {
    S[r] = 0;
  }
  
  Echo(TRUE);
  asciiResultCodes(TRUE);
  noResultCodes(FALSE);
  ModemClass("1");
}

Profile &Profile::operator=(const Profile &p) {
  for( PINDEX r = 0 ; r <= MaxReg ; r++ ) {
    S[r] = p.S[r];
  }
  ModemClass(p.ModemClass());
  return *this;
}
///////////////////////////////////////////////////////////////
ModemEngineBody::ModemEngineBody(ModemEngine &_parent, const PNotifier &_callbackEndPoint)
  : parent(_parent),
    t38engine(NULL),
    callbackEndPoint(_callbackEndPoint),
#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif
    myCallback(PCREATE_NOTIFIER(OnMyCallback)),
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
    timerRing(myCallback, TRUE),
    timeout(myCallback),
    seq(0),
    callDirection(cdUndefined),
    forceFaxMode(FALSE),
    connectionEstablished(FALSE),
    state(stCommand)
{
}

ModemEngineBody::~ModemEngineBody()
{
  PWaitAndSignal mutexWait(Mutex);

  if (!CallToken().IsEmpty()) {
    myPTRACE(1, "ModemEngineBody::~ModemEngineBody Call " << CallToken() << " was not cleared");
    _ClearCall();
  }

  if (t38engine) {
    myPTRACE(1, "ModemEngineBody::~ModemEngineBody t38engine was not Detached");
    _Detach(t38engine);
  }
}

void ModemEngineBody::OnParentStop()
{
  PWaitAndSignal mutexWait(Mutex);

  _ClearCall();

  if (t38engine)
    _Detach(t38engine);
}

void ModemEngineBody::_ClearCall()
{
  connectionEstablished = FALSE;
  if (CallToken().IsEmpty())
    return;
    
  timerRing.Stop();
  
  PStringToString request;
  request.SetAt("modemtoken", parent.modemToken());
  request.SetAt("command", "clearcall");
  request.SetAt("calltoken", CallToken());

  Mutex.Signal();
  callbackEndPoint(request, 1);
  Mutex.Wait();
  CallToken("");
  callDirection = cdUndefined;
  forceFaxMode = FALSE;
}

BOOL ModemEngineBody::Request(PStringToString &request)
{
  myPTRACE(1, "ModemEngineBody::Request request={\n" << request << "}");
  
  PString command = request("command");

  request.SetAt("response", "reject");
  
  if( command == "call" ) {
    PWaitAndSignal mutexWait(Mutex);
    if (callDirection == cdUndefined && CallToken().IsEmpty()) {
      CallToken(request("calltoken"));
      SrcNum(request("srcnum"));
      DstNum(request("dstnum"));
      timerRing.Start(5000);
      P.SetReg(1, 0);
      request.SetAt("response", "confirm");
    } else {
      myPTRACE(1, "ModemEngineBody::Request already in use by " << CallToken());
    }
  } else if (command == "established") {
    PWaitAndSignal mutexWait(Mutex);
    if (CallToken().IsEmpty()) {
      myPTRACE(1, "ModemEngineBody::Request not in use");
    } else if (CallToken() == request("calltoken")) {
      if (state == stConnectWait) {
        param = chEvent;
        state = stConnectHandle;
        parent.SignalDataReady();
        request.SetAt("response", "confirm");
      }
    } else {
      myPTRACE(1, "ModemEngineBody::Request in use by " << CallToken());
    }
  } else if (command == "clearcall") {
    PWaitAndSignal mutexWait(Mutex);
    if (CallToken().IsEmpty()) {
      PTRACE(3, "ModemEngineBody::Request not in use");
    } else if (CallToken() == request("calltoken")) {
      connectionEstablished = FALSE;
      timerRing.Stop();
      if (state != stResetHandle) {
        param = state;
        state = stResetHandle;
        parent.SignalDataReady();
        request.SetAt("response", "confirm");
      }
    } else {
      myPTRACE(1, "ModemEngineBody::Request in use by " << CallToken());
    }
  } else {
    myPTRACE(1, "ModemEngineBody::Request unknown request");
  }
  return TRUE;
}

BOOL ModemEngineBody::Attach(T38Engine *_t38engine)
{
  if (_t38engine == NULL) {
    myPTRACE(1, "ModemEngineBody::Attach _t38engine==NULL");
    return FALSE;
  }

  PTRACE(1, "ModemEngineBody::Attach");

  PWaitAndSignal mutexWait(Mutex);

  for (;;) {
    if (t38engine != NULL) {
      myPTRACE(1, "ModemEngineBody::Attach Other t38engine already Attached");
      return FALSE;
    }

    if (_t38engine->TryLockModemCallback()) {
      if (!_t38engine->Attach(myCallback)) {
        myPTRACE(1, "ModemEngineBody::Attach Can't Attach myCallback to _t38engine");
        _t38engine->UnlockModemCallback();
        return FALSE;
      }
      _t38engine->UnlockModemCallback();
      t38engine = _t38engine;
      break;
    }

    Mutex.Signal();
    PThread::Sleep(20);
    Mutex.Wait();
  }

  if (state == stReqModeAckWait) {
    state = stReqModeAckHandle;
    timeout.Stop();
    parent.SignalDataReady();
  }

  myPTRACE(1, "ModemEngineBody::Attach t38engine Attached");
  return TRUE;
}

void ModemEngineBody::_Detach(T38Engine *_t38engine)
{
  if (_t38engine == NULL) {
    myPTRACE(1, "ModemEngineBody::_Detach _t38engine==NULL");
    return;
  }

  for (;;) {
    if (t38engine != _t38engine) {
      myPTRACE(1, "ModemEngineBody::_Detach " << (t38engine  ? "Other" : "No") << " t38engine was Attached");
      return;
    }

    if (_t38engine->TryLockModemCallback()) {
      _t38engine->Detach(myCallback);
      _t38engine->UnlockModemCallback();
      t38engine = NULL;
      break;
    }

    Mutex.Signal();
    PThread::Sleep(20);
    Mutex.Wait();
  }

  myPTRACE(1, "ModemEngineBody::_Detach t38engine Detached");
}

void ModemEngineBody::OnMyCallback(PObject &from, INT extra)
{
  PTRACE(extra < 0 ? 1 : 3, "ModemEngineBody::OnMyCallback " << from.GetClass() << " " << extra);
  if (PIsDescendant(&from, T38Engine) ) {
    PWaitAndSignal mutexWait(Mutex);
    if( extra == seq ) {
      switch( state ) {
        case stSendAckWait:
          state = stSendAckHandle;
          timeout.Stop();
          break;
        case stRecvBegWait:
          state = stRecvBegHandle;
          timeout.Stop();
          break;
      }
    } else switch( extra ) {
      case T38Engine::cbpReset:
        switch( state ) {
          case stSend:
            param = state;
            state = stResetHandle;
            break;
        }
        break;
      case T38Engine::cbpOutBufEmpty:
        switch( state ) {
          case stSend:
            state = stSendBufEmptyHandle;
            break;
        }
        break;
      case T38Engine::cbpOutBufNoFull:
        break;
      default:
        myPTRACE(1, "ModemEngineBody::OnMyCallback extra(" << extra << ") != seq(" << seq << ")");
    }
  }
  parent.SignalDataReady();
}

static int ParseNum(const char **ppCmd, 
  PINDEX minDigits = 1, PINDEX maxDigits = 3, int maxNum = 255)
{
    const char *pEnd = *ppCmd;
    int num = 0;
    
    for( ; isdigit(*pEnd) ; pEnd++ ) {
      num = (num * 10) + (*pEnd - '0');
    }
    
    PINDEX len = pEnd - *ppCmd;
    *ppCmd = pEnd;
    
    if( len < minDigits || len > maxDigits || num > maxNum) {
      num = -1;
    }
    return num;
}

BOOL ModemEngineBody::HandleClass1Cmd(const char **ppCmd, PString &resp, BOOL &ok, BOOL &crlf)
{
  BOOL T;
  
  switch( *(*ppCmd - 2) ) {
    case 'T':
      T = TRUE;
      break;
    case 'R':
      T = FALSE;
      break;
    default:
      return FALSE;
  }

  int dt;
  
  switch( *(*ppCmd - 1) ) {
    case 'S':
      dt = T38Engine::dtSilence;
      break;
    case 'M':
      dt = T38Engine::dtRaw;
      break;
    case 'H':
      dt = T38Engine::dtHdlc;
      break;
    default:
      return FALSE;
  }
  
  if( dt == T38Engine::dtSilence ) {
    switch( *(*ppCmd)++ ) {
      case '=':
        switch( **ppCmd ) {
          case '?':
            (*ppCmd)++;
            resp += "\r\n0-255";
            crlf = TRUE;
            break;
          default:
            {
              int dms = ParseNum(ppCmd);
              if( dms >= 0 ) {
                ok = FALSE;

                PWaitAndSignal mutexWait(Mutex);
                dataType = dt;
                state = stSend;
                if (t38engine && t38engine->SendStart(dataType, dms*10)) {
                  state = stSendAckWait;
                  if (!t38engine->SendStop(FALSE, NextSeq())) {
                    state = stCommand;
                    return FALSE;
                  }
                } else {
                  state = stCommand;
                  return FALSE;
                }
              } else {
                return FALSE;
              }
            }
        }
        break;
      default:
        return FALSE;
    }
  } else {
    switch( *(*ppCmd)++ ) {
      case '=':
        switch( **ppCmd ) {
          case '?':
            (*ppCmd)++;
            if( dt == T38Engine::dtRaw )
              resp += "\r\n24,48,72,73,74,96,97,98,121,122,145,146";
            else
              resp += "\r\n3";
            crlf = TRUE;
            break;
          default:
            {
              int br = ParseNum(ppCmd);

              if ((dt == T38Engine::dtRaw && br == 3) || (dt == T38Engine::dtHdlc && br != 3))
                return FALSE;

              switch( br ) {
                case 3:
                case 24:
                case 48:
                case 72:
                case 73:
                case 74:
                case 96:
                case 97:
                case 98:
                case 121:
                case 122:
                case 145:
                case 146:
                  ok = FALSE;
                  {
                    PString _resp;
                    BOOL res = T ? SendStart(dt, br, _resp) : RecvStart(dt, br);

                    if (_resp.GetLength()) {
                      if (crlf) {
                        resp += "\r\n";
                        crlf = FALSE;
                      } else {
                        resp += RC_PREF();
                      }

                      resp += _resp;
                    }

                    if (!res)
                      PThread::Sleep(100);	// workaround
                    return res;
                  }
                default:
                  return FALSE;
              }
            }
        }
        break;
      default:
        return FALSE;
    }
  }
  return TRUE;
}

#define ToSBit(funk)			\
  switch( ParseNum(&pCmd, 0, 1, 1) ) {	\
    case 0:				\
      P.funk(FALSE);			\
      break;				\
    case 1:				\
      P.funk(TRUE);			\
      break;				\
    default:				\
      err = TRUE;			\
  }

void ModemEngineBody::HandleCmd(const PString & cmd, PString & resp)
{

  PINDEX i;

  for (i = 0 ;  ; i++) {
    i = cmd.FindOneOf("Aa", i);

    if (i == P_MAX_INDEX) {
      myPTRACE(1, "--> " << cmd.GetLength() << " bytes of binary");
      return;
    }

    PString at = cmd.Mid(i, 2);

    if (at == "AT" || at == "at")
      break;
  }

#if PTRACING
  if (i) {
    PBYTEArray bin((const BYTE *)(const char *)cmd, i);

    myPTRACE(1, "--> " << PRTHEX(bin));
  }
#endif

  const char *pCmd;

  pCmd = ((const char *)cmd) + i;

  myPTRACE(1, "--> " << pCmd);

  pCmd += 2;  // skip AT

  PString ucmd = PString(pCmd).ToUpper();
  pCmd = ucmd;

  BOOL err = FALSE;
  BOOL ok = TRUE;
  BOOL crlf = FALSE;

  while (state == stCommand && !err && *pCmd) {
      switch( *pCmd++ ) {
        case ' ':
          break;
        case 'A':	// Accept incoming call
          ok = FALSE;
          {
            PWaitAndSignal mutexWait(Mutex);
            callDirection = cdIncoming;
            forceFaxMode = TRUE;
            timerRing.Stop();
            if (!connectionEstablished) {
              state = stConnectWait;
              timeout.Start(60000);
            
              PStringToString request;
              request.SetAt("modemtoken", parent.modemToken());
              request.SetAt("command", "answer");
              request.SetAt("calltoken", CallToken());
            
              Mutex.Signal();
              callbackEndPoint(request, 2);
              Mutex.Wait();
              
              PString response = request("response");
              
              if (response != "confirm" ) {
                callDirection = cdUndefined;
                forceFaxMode = FALSE;
                timeout.Stop();
                state = stCommand;
                err = TRUE;
              }
            }
            
            if (!err) {
              if (connectionEstablished) {
                timeout.Stop();
                param = chHandle;
                state = stConnectHandle;
                parent.SignalDataReady();
              }
            }
          }
          break;
        case 'B':       // Turn ITU-T V.22/BELL 212A
          if (ParseNum(&pCmd, 0, 1) >= 0) {
          } else {
            err = TRUE;
          }
          break;
        case 'D':	// Dial
          ok = FALSE;

          {
            PString num;
            PString LocalPartyName;
            BOOL local = FALSE;
            BOOL setForceFaxMode = FALSE;
          
            for( char ch ; (ch = *pCmd) != 0 && !err ; pCmd++ ) {
              if( isdigit(ch) ) {
                if (local)
                  LocalPartyName += ch;
                else
                  num += ch;
              } else {
                switch( ch ) {
                  case '.':
                  case ' ':
                  case '-':
                  case 'T':
                  case 'P':
                    break;
                  case 'F':
                    setForceFaxMode = TRUE;
                    break;
                  case 'V':
                    setForceFaxMode = FALSE;
                    break;
                  case 'L':
                    LocalPartyName = "";
                    local = TRUE;
                    break;
                  case 'D':
                    local = FALSE;
                    break;
                  default:
                    err = TRUE;
                }
              }
            }
            
            if (!err) {
              PWaitAndSignal mutexWait(Mutex);

              if (!CallToken().IsEmpty()) {
                _ClearCall();

                if (crlf) {
                  resp += "\r\n";
                  crlf = FALSE;
                } else {
                  resp += RC_PREF();
                }

                resp += RC_NO_DIALTONE();
                break;
              }

              forceFaxMode = setForceFaxMode;
              callDirection = cdOutgoing;
              timerRing.Stop();
              state = stConnectWait;
              timeout.Start(60000);
            
              PStringToString request;
              request.SetAt("modemtoken", parent.modemToken());
              request.SetAt("command", "dial");
              request.SetAt("number", num);
              request.SetAt("localpartyname", LocalPartyName);
            
              Mutex.Signal();
              callbackEndPoint(request, 3);
              Mutex.Wait();

              PString response = request("response");
              
              if (response == "confirm") {
                CallToken(request("calltoken"));
              } else {
                callDirection = cdUndefined;
                forceFaxMode = FALSE;
                timeout.Stop();
                state = stCommand;
                if (response == "reject") {
                  if (crlf) {
                    resp += "\r\n";
                    crlf = FALSE;
                  } else {
                    resp += RC_PREF();
                  }

                  PString diag = request("diag");
                  if( diag == "noroute" )
                    resp += RC_BUSY();
                  else
                    resp += RC_NO_DIALTONE();
                } else {
                  err = TRUE;
                }
              }
            }
          }
          break;
        case 'E':	// Turn Echo on/off
          ToSBit(Echo);
          break;
        case 'H':	// On/Off-hook
          if( ParseNum(&pCmd, 0, 1, 0) >= 0 ) {	// ATH & ATH0
            PWaitAndSignal mutexWait(Mutex);

            if (callDirection != cdUndefined || P.ClearMode())
              _ClearCall();
          } else {
            err = TRUE;
          }
          break;
        case 'I':	// Information
          {
            int val = ParseNum(&pCmd, 0, 1);
            
            switch( val ) {
              case 0:
                resp += "\r\n" + PString(Model);
                crlf = TRUE;
                break;
              case 3:
                resp += "\r\n" + PString(Manufacturer);
                crlf = TRUE;
                break;
              case 8:
                resp += "\r\nNMBR = " + SrcNum();
                crlf = TRUE;
                break;
              case 9:
                resp += "\r\nNDID = " + DstNum();
                crlf = TRUE;
                break;
              default:
                if( val < 0 )
                  err = TRUE;
            }
          }
          break;
        case 'L':	// Turn Speaker on/off
          if( ParseNum(&pCmd, 0, 1) >= 0 ) {
          } else {
            err = TRUE;
          }
          break;
        case 'M':	// Turn Speaker on/off
          if( ParseNum(&pCmd, 0, 1) >= 0 ) {
          } else {
            err = TRUE;
          }
          break;
        case 'N':	// Ring volume
          if( ParseNum(&pCmd, 0, 1) >= 0 ) {
          } else {
            err = TRUE;
          }
          break;
        case 'O':	// Go online
          err = TRUE;
          break;
        case 'Q':	// Turn result codes on/off
          ToSBit(noResultCodes);
          break;
        case 'S':	// Set/Get Register
          {
          int r = ParseNum(&pCmd);
          
          if( r >= 0 ) {
            switch( *pCmd++ ) {
              case '=':
                {
                  int val = ParseNum(&pCmd);
                  if( val < 0 || !P.SetReg(r, (BYTE)val) ) {
                    err = TRUE;
                  }
                }
                break;
              case '?':
                {
                  BYTE val;
                
                  if( P.GetReg(r, val) ) {
                    resp.sprintf("\r\n%3.3u", (unsigned)val);
                    crlf = TRUE;
                  } else {
                    err = TRUE;
                  }
                }
                break;
              case '.':
                {
                  int b = ParseNum(&pCmd, 1, 1, 7);
          
                  if( b >= 0 ) {
                    switch( *pCmd++ ) {
                      case '=':
                        {
                          int val = ParseNum(&pCmd, 1, 1, 1);
                          if( val < 0 || !P.SetBit(r, b, (BOOL)val) ) {
                            err = TRUE;
                          }
                        }
                        break;
                      case '?':
                        {
                          BOOL val;
                
                          if( P.GetBit(r, b, val) ) {
                            resp.sprintf("\r\n%u", (unsigned)val);
                            crlf = TRUE;
                          } else {
                            err = TRUE;
                          }
                        }
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                }
                break;
              default:
                err = TRUE;
            }
          } else {
            err = TRUE;
          }
          }
          break;
        case 'V':	// Numeric or ASCII result codes
          ToSBit(asciiResultCodes);
          break;
        case 'X':	// Which result codes
          if (ParseNum(&pCmd, 0, 1) >= 3) {
          } else {
            err = TRUE;
          }
          break;
        case 'Z':	// Load Registers from Profile
          {
            int val = ParseNum(&pCmd, 0, 1, sizeof(Profiles)/sizeof(Profiles[0]) - 1);
            if( val >= 0 ) {
              PWaitAndSignal mutexWait(Mutex);

              if (callDirection != cdUndefined)
                _ClearCall();

              P = Profiles[val];
            } else {
              err = TRUE;
            }
          }
          break;
        case '+':
          switch( *pCmd++ ) {
            case 'F':	// FAX
              switch( *pCmd++ ) {
                case 'A':
                  if( strncmp(pCmd, "A", 1) == 0 ) {	// +FAA
                    pCmd += 1;
                    switch( *pCmd++ ) {
                      case '=':
                        switch( *pCmd ) {
                          case '?':
                            pCmd++;
                            resp += "\r\n0";
                            crlf = TRUE;
                            break;
                          default:
                            switch( ParseNum(&pCmd) ) {
                              case 0:
                                break;
                              default:
                                err = TRUE;
                            }
                        }
                        break;
                      case '?':
                        resp += "\r\n0";
                        crlf = TRUE;
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                  break;
                case 'C':
                  if( strncmp(pCmd, "LASS", 4) == 0 ) {	// +FCLASS
                    pCmd += 4;
                    switch( *pCmd++ ) {
                      case '=':
                        switch( *pCmd ) {
                          case '?':
                            pCmd++;
                            resp += "\r\n1";	// "\r\n1,8"
                            crlf = TRUE;
                            break;
                          default:
                            switch( ParseNum(&pCmd) ) {
                              case 0:
                                P.ModemClass("0");
                                break;
                              case 1:
                                P.ModemClass("1");
                                break;
                              //case 8:
                              //  P.ModemClass("8");
                              //  break;
                              default:
                                err = TRUE;
                            }
                        }
                        break;
                      case '?':
                        resp += "\r\n" + P.ModemClass();
                        crlf = TRUE;
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                  break;
                case 'L':
                  switch (*pCmd++) {
                    case 'O': // +FLO
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n0-2";
                              crlf = TRUE;
                              break;
                            default:
                              {
                                int val = ParseNum(&pCmd);
                                switch (val) {
                                  case 0:
                                  case 1:
                                  case 2:
                                    P.Flo((BYTE)val);
                                    break;
                                  default:
                                    err = TRUE;
                                }
                              }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u", (unsigned)P.Flo());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'M':
                  switch (*pCmd++) {
                    case 'D':
                      switch (*pCmd++) {
                        case 'L': // +FMDL
                          switch (*pCmd++) {
                            case '?':
                              resp += "\r\n" + PString(Model);
                              crlf = TRUE;
                              break;
                            default:
                              err = TRUE;
                          }
                          break;
                      default:
                          err = TRUE;
                      }
                      break;
                    case 'F':
                      switch (*pCmd++) {
                        case 'R': // +FMFR
                          switch (*pCmd++) {
                            case '?':
                              resp += "\r\n" + PString(Manufacturer);
                              crlf = TRUE;
                              break;
                            default:
                              err = TRUE;
                          }
                          break;
                      default:
                          err = TRUE;
                      }
                      break;
                    case 'I': // +FMI
                      switch (*pCmd++) {
                        case '?':
                          resp += "\r\n" + PString(Manufacturer);
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'M': // +FMM
                      switch (*pCmd++) {
                        case '?':
                          resp += "\r\n" + PString(Model);
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'R': // +FMR
                      switch (*pCmd++) {
                        case '?':
                          resp += "\r\n" + PString(Revision);
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'R':
                  switch( *pCmd ) {
                    case 'M':				// +FRM
                    case 'H':				// +FRH
                    case 'S':				// +FRS
                      pCmd++;
                      if (!HandleClass1Cmd(&pCmd, resp, ok, crlf))
                        err = TRUE;
                      break;
                    default:
                      if( strncmp(pCmd, "EV", 2) == 0 ) {	// +FREV
                        pCmd += 2;
                        switch( *pCmd++ ) {
                          case '?':
                            resp += "\r\n" + PString(Revision);
                            crlf = TRUE;
                            break;
                          default:
                            err = TRUE;
                        }
                      } else {
                        err = TRUE;
                      }
                  }
                  break;
                case 'T':
                  switch( *pCmd ) {
                    case 'M':				// +FTM
                    case 'H':				// +FTH
                    case 'S':				// +FTS
                      pCmd++;
                      if (!HandleClass1Cmd(&pCmd, resp, ok, crlf))
                        err = TRUE;
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                default:
                  err = TRUE;
              }
              break;
            case 'V':	// Voice
              break;
            default:
              err = TRUE;
          }
          break;
        case '&':
          switch( *pCmd++ ) {
            case 'C':					// &C
              {
                int val = ParseNum(&pCmd, 0, 1, 1);
                if( val >= 0 ) {
                } else {
                  err = TRUE;
                }
              }
              break;
            case 'D':					// &D
              {
                int val = ParseNum(&pCmd, 0, 1, 3);
                if( val >= 0 ) {
                } else {
                  err = TRUE;
                }
              }
              break;
            case 'F':					// &F
              {
                PWaitAndSignal mutexWait(Mutex);

                if (callDirection != cdUndefined)
                  _ClearCall();

                P = Profiles[0];
              }
              break;
            case 'H':					// &H
              {
                int val = ParseNum(&pCmd, 0, 1, 7);
                if( val >= 0 ) {
                  P.SetBits(27, 3, 5, (BYTE)val);
                } else {
                  err = TRUE;
                }
              }
              break;
            default:
              err = TRUE;
          }
          break;
        case '#':
          if (strncmp(pCmd, "CID", 3) == 0) {		// #CID
            pCmd += 3;
            switch( *pCmd++ ) {
              case '=':
                switch( *pCmd ) {
                  case '?':
                    pCmd++;
                    resp += "\r\n(0,10)";
                    crlf = TRUE;
                    break;
                  default:
                    {
                      int val = ParseNum(&pCmd);
                      switch (val) {
                        case 0:
                        case 10:
                          P.CidMode((BYTE)val);
                          break;
                        default:
                          err = TRUE;
                      }
                    }
                }
                break;
              case '?':
                resp.sprintf("\r\n%u", (unsigned)P.CidMode());
                crlf = TRUE;
                break;
              default:
                err = TRUE;
            }
          } else
          if (strncmp(pCmd, "DFRMC", 5) == 0) {         // #DFRMC
            pCmd += 5;
            switch( *pCmd++ ) {
              case '=':
                switch( *pCmd ) {
                  case '?':
                    pCmd++;
                    resp += "\r\n(0-255)";
                    crlf = TRUE;
                    break;
                  default:
                    {
                      int val = ParseNum(&pCmd);
                      if (val >= 0) {
                        P.DelayFrmConnect((BYTE)val);
                      } else {
                        err = TRUE;
                      }
                    }
                }
                break;
              case '?':
                resp.sprintf("\r\n%u", (unsigned)P.DelayFrmConnect());
                crlf = TRUE;
                break;
              default:
                err = TRUE;
            }
          } else
          if (strncmp(pCmd, "HCLR", 4) == 0) {         // #HCLR
            pCmd += 4;
            switch( *pCmd++ ) {
              case '=':
                switch( *pCmd ) {
                  case '?':
                    pCmd++;
                    resp += "\r\n(0,1)";
                    crlf = TRUE;
                    break;
                  default:
                    {
                      int val = ParseNum(&pCmd);
                      switch (val) {
                        case 0:
                        case 1:
                          P.ClearMode((BYTE)val);
                          break;
                        default:
                          err = TRUE;
                      }
                    }
                }
                break;
              case '?':
                resp.sprintf("\r\n%u", (unsigned)P.ClearMode());
                crlf = TRUE;
                break;
              default:
                err = TRUE;
            }
          } else {
            err = TRUE;
          }
          break;
        default:
          err = TRUE;
      }
  }

  if (crlf)
    resp += "\r\n";

  if (err) {
    if (!crlf)
      resp += RC_PREF();
    resp += RC_ERROR();
  }
  else
  if (ok) {
    if (!crlf)
      resp += RC_PREF();
    resp += RC_OK();
  }
}

void ModemEngineBody::HandleData(const PBYTEArray &buf, PBYTEArray &bresp)
{
    int len = buf.GetSize();
    const BYTE *pBuf = buf;
    
    while( len > 0 ) {
      switch( state ) {
        case stCommand:
          {
          PWaitAndSignal mutexWait(Mutex);
          lastPtyActivity = PTime();
          }
          
          while( state == stCommand && len > 0 ) {
            const BYTE *pEnd = (const BYTE *)memchr(pBuf, '\r', len);
    
            if( pEnd == NULL ) {
              cmd += PString((const char *)pBuf, len);
              if( Echo() )
                bresp.Concatenate(PBYTEArray(pBuf, len));
              len = 0;
            } else {
              int rlen = pEnd - pBuf;
              if( rlen ) {
                cmd += PString((const char *)pBuf, rlen);
                if( Echo() ) {
                  bresp.Concatenate(PBYTEArray(pBuf, rlen));
                }
                len -= rlen;
                pBuf += rlen;
              }
              len--;
              pBuf++;

              if (Echo())
                bresp.Concatenate(PBYTEArray((const BYTE *)"\r", 1));

              PString resp;

              HandleCmd(cmd, resp);

              if (resp.GetLength()) {
                PBYTEArray _bresp((const BYTE *)(const char *)resp, resp.GetLength());

                myPTRACE(1, "<-- " << PRTHEX(_bresp));
                bresp.Concatenate(_bresp);
              }

              cmd = PString();
            }
          }
          break;
        case stSend:
          {
            int lendone = dleData.PutDleData(pBuf, len);
            
            if( lendone > 0 ) {
                if( Echo() )
                  bresp.Concatenate(PBYTEArray(pBuf, lendone));
                len -= lendone;
                pBuf += lendone;
            }
            
            BYTE Buf[1024];
            
            for(;;) {
              int count = dleData.GetData(Buf, 1024);
            
              PWaitAndSignal mutexWait(Mutex);
              switch( count ) {
                case -1:
                  state = stSendAckWait;
                  if( !t38engine || !t38engine->SendStop(moreFrames, NextSeq()) ) {
                    PString resp = RC_PREF() + RC_ERROR();

                    bresp.Concatenate(PBYTEArray((const BYTE *)resp, resp.GetLength()));
                    state = stCommand;
                  }
                  break;
                case 0:
                  break;
                default:
                  switch( dataType ) {
                    case T38Engine::dtHdlc:
                      if( dataCount < 2 && (dataCount + count) >= 2 && 
                          			(Buf[1 - dataCount] & 0x08) == 0 ) {
                        moreFrames = TRUE;
                      }
                  }
                  dataCount += count;
                  if( t38engine )
                    t38engine->Send(Buf, count);
              }
              if( count <= 0 ) break;
            }
          }
          break;
        case stSendAckWait:
          myPTRACE(1, "Reset state stSendAckWait");
          {
            PWaitAndSignal mutexWait(Mutex);
            state = stCommand;
            timeout.Stop();
            if (t38engine)
              t38engine->ResetModemState();
          }
          break;
        default:
          myPTRACE(1, "Reset state " << state);
          if( Echo() )
            bresp.Concatenate(PBYTEArray(pBuf, 1));
          len--;
          pBuf++;
          {
            PWaitAndSignal mutexWait(Mutex);
            state = stCommand;
            timeout.Stop();

            PString resp = RC_PREF();

            if (t38engine) {
              t38engine->ResetModemState();
              resp += RC_OK();
            } else {
              _ClearCall();
              resp += RC_NO_CARRIER();
            }

            bresp.Concatenate(PBYTEArray((const BYTE *)resp, resp.GetLength()));
          }
      }
    }
}

void ModemEngineBody::CheckState(PBYTEArray & bresp)
{
  PString resp;
  
  {
    PWaitAndSignal mutexWait(Mutex);
    if (cmd.IsEmpty() && timerRing.Get())  {
      resp = RC_RING();
      BYTE s0, ringCount;
      P.GetReg(0, s0);
      P.GetReg(1, ringCount);
      if (!ringCount && P.CidMode() == 10) {
        resp += "NMBR = " + SrcNum() + "\r\n"
                "NDID = " + DstNum() + "\r\n";
        resp += RC_RING();
      }
      P.SetReg(1, ++ringCount);
      if (s0 > 0 && (ringCount >= s0)) {
        PString resp;
        HandleCmd("ATA", resp);
      }
    }
  }

  if( timeout.Get() ) {
    PWaitAndSignal mutexWait(Mutex);
    switch( state ) {
      case stConnectWait:
      case stReqModeAckWait:
      case stRecvBegWait:
        resp = RC_NO_CARRIER();
        state = stCommand;
        if (t38engine)
          t38engine->ResetModemState();
        else
          _ClearCall();
        break;
      case stConnectHandle:
        param = chHandle;
        break;
    }
  }

  switch( state ) {
    case stResetHandle:
      {
        PWaitAndSignal mutexWait(Mutex);
        switch( param ) {
          case stCommand:
            break;
          case stSend:
            resp = RC_ERROR();
            break;
          case stRecvBegWait:
            resp = RC_NO_CARRIER();
            break;
          case stConnectWait:
          case stConnectHandle:
          case stReqModeAckWait:
          case stReqModeAckHandle:
            if (callDirection == cdOutgoing)
              resp = RC_BUSY();
            else
              resp = RC_ERROR();
            break;
          default:
            resp = RC_ERROR();
        }
        state = stCommand;
        if (t38engine)
          t38engine->ResetModemState();
        else
          _ClearCall();
      }
      break;
    case stSendBufEmptyHandle:
      {
        PWaitAndSignal mutexWait(Mutex);
        state = stSendAckWait;
        if( !t38engine || !t38engine->SendStop(moreFrames, NextSeq()) ) {
          resp = RC_ERROR();
          state = stCommand;
        }
      }
      break;
    case stConnectHandle:
      {
        PWaitAndSignal mutexWait(Mutex);
        switch(param) {
          case chEvent:
            if (forceFaxMode) {
              param = chDelay;
              timeout.Start(1000);    // wait 1 sec before request mode
              break;
            }
          case chHandle:
            connectionEstablished = TRUE;
            if (t38engine) {
              state = stReqModeAckHandle;
              timeout.Stop();
              parent.SignalDataReady();
            } else {
              state = stReqModeAckWait;
              if (!forceFaxMode) {
                timeout.Start(60000);
                break;
              }
              timeout.Start(10000);
              PStringToString request;
              request.SetAt("modemtoken", parent.modemToken());
              request.SetAt("command", "requestmode");
              request.SetAt("calltoken", CallToken());
              request.SetAt("mode", "fax");
    
              Mutex.Signal();
              callbackEndPoint(request, 4);
              Mutex.Wait();
    
              PString response = request("response");
    
              if (response == "confirm" ) {
              } else {
                state = stCommand;
                timeout.Stop();
                resp = RC_NO_CARRIER();
              }
            }
            break;
        }
      }
      break;
    case stReqModeAckHandle:
      {
        PWaitAndSignal mutexWait(Mutex);
        switch( callDirection ) {
          case cdIncoming:
            dataType = T38Engine::dtCed;
            state = stSend;
            if (t38engine && t38engine->SendStart(dataType, 3000)) {
              state = stSendAckWait;
              if (!t38engine->SendStop(FALSE, NextSeq())) {
                resp = RC_ERROR();
                state = stCommand;
              }
            } else {
              resp = RC_NO_CARRIER();
              state = stCommand;
            }
            break;
          case cdOutgoing:
            if (t38engine)
              t38engine->SendOnIdle(T38Engine::dtCng);
            if( !RecvStart(T38Engine::dtHdlc, 3) ) {
              resp = RC_ERROR();
            }
            break;
          default:
            resp = RC_NO_CARRIER();
            state = stCommand;
        }
      }
      break;
    case stSendAckHandle:
      switch( dataType ) {
        case T38Engine::dtCed:
          if( !SendStart(T38Engine::dtHdlc, 3, resp) )
            resp = RC_ERROR();
          break;
        case T38Engine::dtSilence:
            resp = RC_OK();
            state = stCommand;
          break;
        case T38Engine::dtHdlc:
        case T38Engine::dtRaw:
          {
            PWaitAndSignal mutexWait(Mutex);
            if( moreFrames ) {
              resp = RC_CONNECT();
              state = stSend;
            } else {
              resp = RC_OK();
              state = stCommand;
            }
            ResetDleData();
          }
          break;
      }
      break;
    case stRecvBegHandle:
      {
        PWaitAndSignal mutexWait(Mutex);
        
        if( t38engine && t38engine->RecvStart(NextSeq()) ) {
          if( (t38engine->RecvDiag() & T38Engine::diagDiffSig) == 0 ) {
            if (dataType != T38Engine::dtRaw)
              resp = RC_CONNECT();
            dataCount = 0;
            state = stRecv;
            parent.SignalDataReady();	// try to Recv w/o delay
          } else {
            t38engine->RecvStop();
            resp = RC_FCERROR();
            state = stCommand;
          }
        } else {
          resp = RC_ERROR();
          state = stCommand;
        }
        ResetDleData();
        if (dataType == T38Engine::dtHdlc)
          fcs = FCS();
      }
      break;
    case stRecv:
      {
        BYTE Buf[1024];
        
        for(;;) {
          PWaitAndSignal mutexWait(Mutex);
          if( !t38engine ) {
            dleData.SetDiag(T38Engine::diagError).PutEof();
            break;
          }
          int count = t38engine->Recv(Buf, 1024);

          switch (count) {
            case -1:
              {
                int diag = t38engine->RecvDiag();

                if (dataType == T38Engine::dtHdlc) {
                  Buf[0] = BYTE(fcs >> 8);
                  Buf[1] = BYTE(fcs);
                  if (diag & T38Engine::diagBadFcs)
                     Buf[0]++;
                  dleData.PutData(Buf, 2);
                }
                dleData.SetDiag(diag).PutEof();
              }
              t38engine->RecvStop();
              if (dataCount == 0)
                dleData.GetDleData(Buf, 1024);	// discard ...<DLE><ETX>
              break;
            case 0:
              break;
            default:
              dleData.PutData(Buf, count);
              switch (dataType) {
                case T38Engine::dtHdlc:
                  fcs.build(Buf, count);
                  break;
                case T38Engine::dtRaw:
                  if (!dataCount) {
                    int dms = P.DelayFrmConnect();

                    if (dms) {
                      Mutex.Signal();
                      PThread::Sleep(dms * 10);
                      Mutex.Wait();
                    }

                    // send CONNECT just before data for AT+FRM command

                    PString _resp = RC_PREF() + RC_CONNECT();

                    PBYTEArray _bresp((const BYTE *)(const char *)_resp, _resp.GetLength());

                    myPTRACE(1, "<-- " << PRTHEX(_bresp));
                    bresp.Concatenate(_bresp);
                  }
              }
              dataCount += count;
          }
          if (count <= 0)
            break;
        }

        for(;;) {
          int count = dleData.GetDleData(Buf, 1024);
        
          switch( count ) {
            case -1:
              {
                PWaitAndSignal mutexWait(Mutex);
                state = stCommand;
                int diag = dleData.GetDiag();
                
                if (dataType == T38Engine::dtHdlc) {
                  if (diag == 0)
                    resp = RC_OK();
                  else if (dataCount == 0 && (diag & ~T38Engine::diagNoCarrier) == 0)
                    resp = RC_NO_CARRIER();
                  else
                    resp = RC_ERROR();
                } else {
                  resp = RC_NO_CARRIER();
                }
              }
              break;
            case 0:
              break;
            default:
#if PTRACING
              if (myCanTrace(2)) {
                 if (count <= 16) {
                   PTRACE(2, "<-- T38DLE " << PRTHEX(PBYTEArray(Buf, count)));
                 } else {
                   PTRACE(2, "<-- T38DLE " << count << " bytes");
                 }
              }
#endif
              bresp.Concatenate(PBYTEArray(Buf, count));
          }
          if( count <= 0 ) break;
        }
      }
      break;
  }
  
  if( resp.GetLength() ) {
    resp = RC_PREF() + resp;

    PBYTEArray _bresp((const BYTE *)(const char *)resp, resp.GetLength());
    
    myPTRACE(1, "<-- " << PRTHEX(_bresp));
    bresp.Concatenate(_bresp);
  }
}
///////////////////////////////////////////////////////////////

