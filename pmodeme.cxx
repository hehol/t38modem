/*
 * pmodeme.cxx
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
 * $Log: pmodeme.cxx,v $
 * Revision 1.46  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.46  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.45  2008/09/10 07:05:06  frolov
 * Fixed doubled mutex lock
 *
 * Revision 1.44  2007/08/27 10:55:21  vfrolov
 * Added missing moreFrames = FALSE
 *
 * Revision 1.43  2007/08/24 16:12:14  vfrolov
 * Disabled CNG sending in voice class mode
 *
 * Revision 1.42  2007/05/04 09:58:57  vfrolov
 * Fixed Attach(audioEngine)
 *
 * Revision 1.41  2007/04/24 16:26:02  vfrolov
 * Fixed unexpected state change
 *
 * Revision 1.40  2007/04/09 08:07:12  vfrolov
 * Added symbolic logging ModemCallbackParam
 *
 * Revision 1.39  2007/03/30 07:56:36  vfrolov
 * Included g711.c
 *
 * Revision 1.38  2007/03/23 14:54:20  vfrolov
 * Fixed compiler warnings
 *
 * Revision 1.37  2007/03/23 10:14:35  vfrolov
 * Implemented voice mode functionality
 *
 * Revision 1.36  2007/03/22 16:26:04  vfrolov
 * Fixed compiler warnings
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
#include <ptclib/dtmf.h>
#include "pmodemi.h"
#include "pmodeme.h"
#include "dle.h"
#include "fcs.h"
#include "t38engine.h"
#include "audio.h"
#include "version.h"

///////////////////////////////////////////////////////////////
#include "g711.c"
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
      void name(PBoolean val) { SetBit(byte, bit, val); } \
      PBoolean name() const { PBoolean val; GetBit(byte, bit, val); return val; }

    DeclareRegisterBit(Echo, 23, 0);
    DeclareRegisterBit(asciiResultCodes, 23, 6);
    DeclareRegisterBit(noResultCodes, 23, 7);

    #define DeclareRegisterByte(name, byte)	\
      void name(BYTE val) { SetReg(byte, val); } \
      BYTE name() const { BYTE val; GetReg(byte, val); return val; }

    DeclareRegisterByte(IfcByDCE, 45);
    DeclareRegisterByte(IfcByDTE, 46);
    DeclareRegisterByte(ClearMode, 47);
    DeclareRegisterByte(DelayFrmConnect, 48);
    DeclareRegisterByte(DidMode, 49);
    DeclareRegisterByte(CidMode, 50);

    void Flo(BYTE val) { IfcByDTE(val); IfcByDCE(val); }
    BYTE Flo() const {
      if (IfcByDTE() == IfcByDCE())
        return IfcByDTE();
      return 255;
    }

    PBoolean SetBit(PINDEX r, PINDEX b, PBoolean val) {
      if( !ChkRB(r, b) ) return FALSE;
      BYTE msk = MaskB(b);
      if( val ) S[r] |= msk;
      else      S[r] &= ~msk;
      return TRUE;
    }
    PBoolean GetBit(PINDEX r, PINDEX b, PBoolean &val) const {
      if (!ChkRB(r, b)) {
        val = 0;
        return FALSE;
      }
      BYTE msk = MaskB(b);
      val = (S[r] & msk) ? TRUE : FALSE;
      return TRUE;
    }
    PBoolean SetReg(PINDEX r, BYTE val) {
      if( !ChkR(r) ) return FALSE;
      S[r] = val;
      return TRUE;
    }
    PBoolean GetReg(PINDEX r, BYTE &val) const {
      if (!ChkR(r)) {
        val = 0;
        return FALSE;
      }
      val = S[r];
      return TRUE;
    }
    PBoolean SetBits(PINDEX r, PINDEX bl, PINDEX bh, BYTE val) {
      if( !ChkRBB(r, bl, bh) ) return FALSE;
      BYTE msk = MaskBB(bl, bh);
      S[r] &= ~msk;
      S[r] &= (val << bl) & msk;
      return TRUE;
    }
    PBoolean GetBits(PINDEX r, PINDEX bl, PINDEX bh, BYTE &val) const {
      if (!ChkRBB(r, bl, bh)) {
        val = 0;
        return FALSE;
      }
      BYTE msk = MaskBB(bl, bh);
      val = BYTE((S[r] & msk) >> bl);
      return TRUE;
    }
    
    Profile &operator=(const Profile &p);

    void ModemClass(const PString &_modemClass) {
      modemClass = _modemClass;
      audioClass = (modemClass == "8");
    }
    const PString &ModemClass() const { return modemClass; }
    PBoolean AudioClass() const { return audioClass; }
    PBoolean FaxClass() const { return !audioClass; }

  protected:
    static PBoolean ChkR(PINDEX r) {
      return r <= MaxReg;
    }
    static PBoolean ChkB(PINDEX b) {
      return b <= MaxBit;
    }
    static PBoolean ChkRB(PINDEX r, PINDEX b) {
      return ChkR(r) && ChkB(b);
    }
    static PBoolean ChkRBB(PINDEX r, PINDEX bl, PINDEX bh) {
      return ChkR(r) && ChkB(bl) && ChkB(bh) && bl <= bh;
    }
    static BYTE MaskB(PINDEX b) {
      return "\x01\x02\x04\x08\x10\x20\x40\x80"[b];
    }
    static BYTE MaskBB(PINDEX bl, PINDEX bh) { // bl <= bh
      return BYTE(("\x01\x03\x07\x0F\x1F\x3F\x7F\xFF"[bh - bl]) << bl);
    }

    BYTE S[MaxReg + 1];	// S-registers
    PString modemClass;
    PBoolean audioClass;
};
///////////////////////////////////////////////////////////////
class Timeout : public PTimer
{
    PCLASSINFO(Timeout, PTimer);
  public:
    Timeout(const PNotifier &callback, PBoolean _continuous = FALSE) 
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
    
    PBoolean Get() { 
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
    PBoolean continuous;
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
    PBoolean Request(PStringToString &request);
    PBoolean Attach(T38Engine *_t38engine);
    void Detach(T38Engine *_t38engine) { PWaitAndSignal mutexWait(Mutex); _Detach(_t38engine); }
    PBoolean Attach(AudioEngine *_audioEngine);
    void Detach(AudioEngine *_audioEngine) { PWaitAndSignal mutexWait(Mutex); _Detach(_audioEngine); }
    void OnParentStop();
    void HandleData(const PBYTEArray &buf, PBYTEArray &bresp);
    void CheckState(PBYTEArray &bresp);
    PBoolean IsReady() const {
      PWaitAndSignal mutexWait(Mutex);
      return state == stCommand && (PTime() - lastPtyActivity) > 5*1000;
    }
    PBoolean isOutBufFull() const {
      PWaitAndSignal mutexWait(Mutex);
      return (t38engine && t38engine->isOutBufFull()) || (audioEngine && audioEngine->isOutBufFull());
    }
  //@}

  protected:
    PBoolean Echo() const { return P.Echo(); }
    PBoolean HandleClass1Cmd(const char **ppCmd, PString &resp, PBoolean &ok, PBoolean &crlf);
    PBoolean HandleClass8Cmd(const char **ppCmd, PString &resp, PBoolean &ok, PBoolean &crlf);
    PBoolean Answer();
    void HandleCmd(const PString &cmd, PString &resp);

    void ResetDleData() {
      dleData.Clean();
      dleData.BitRev(TRUE);
      dataCount = 0;
      moreFrames = FALSE;
    }

    PBoolean SendStart(int dt, int br, PString &resp) {
      PWaitAndSignal mutexWait(Mutex);
      dataType = dt;
      ResetDleData();
      state = stSend;

      if ((P.AudioClass() && (!audioEngine || !audioEngine->SendStart(dataType, br))) ||
          (P.FaxClass() && (!t38engine || !t38engine->SendStart(dataType, br))))
      {
        state = stCommand;
        return FALSE;
      }

      resp = RC_CONNECT();
      return TRUE;
    }

    PBoolean RecvStart(int dt, int br) {
      PBoolean done = FALSE;

      PWaitAndSignal mutexWait(Mutex);
      dataType = dt;
      state = stRecvBegWait;
      timeout.Start(60000);

      if ((P.AudioClass() && (!audioEngine || !audioEngine->RecvWait(dataType, br, NextSeq(), done))) ||
          (P.FaxClass() && (!t38engine || !t38engine->RecvWait(dataType, br, NextSeq(), done))))
      {
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
    void _Detach(AudioEngine *_audioEngine);
    void _ClearCall();

    int NextSeq() { return seq = ++seq & EngineBase::cbpUserDataMask; }

    ModemEngine &parent;
    T38Engine *t38engine;
    AudioEngine *audioEngine;
    const PNotifier callbackEndPoint;

    PDECLARE_NOTIFIER(PObject, ModemEngineBody, OnEngineCallback);
    PDECLARE_NOTIFIER(PObject, ModemEngineBody, OnTimerCallback);
    const PNotifier engineCallback;
    const PNotifier timerCallback;
    Timeout timerRing;
    Timeout timeout;
    PTime lastPtyActivity;

    int seq;

    int callDirection;
    PBoolean forceFaxMode;
    PBoolean connectionEstablished;
    int state;
    int param;
    PString cmd;
    int dataType;

    DLEData dleData;
    PINDEX dataCount;
    PBoolean moreFrames;
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

PBoolean ModemEngine::IsReady() const
{
  return body && body->IsReady();
}

PBoolean ModemEngine::Attach(T38Engine *t38engine) const
{
  return body && body->Attach(t38engine);
}

void ModemEngine::Detach(T38Engine *t38engine) const
{
  if( body )
    body->Detach(t38engine);
}

PBoolean ModemEngine::Attach(AudioEngine *audioEngine) const
{
  return body && body->Attach(audioEngine);
}

void ModemEngine::Detach(AudioEngine *audioEngine) const
{
  if( body )
    body->Detach(audioEngine);
}

PBoolean ModemEngine::Request(PStringToString &request) const
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
    audioEngine(NULL),
    callbackEndPoint(_callbackEndPoint),
#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif
    engineCallback(PCREATE_NOTIFIER(OnEngineCallback)),
    timerCallback(PCREATE_NOTIFIER(OnTimerCallback)),
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
    timerRing(timerCallback, TRUE),
    timeout(timerCallback),
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

  if (audioEngine) {
    myPTRACE(1, "ModemEngineBody::~ModemEngineBody audioEngine was not Detached");
    _Detach(audioEngine);
  }
}

void ModemEngineBody::OnParentStop()
{
  PWaitAndSignal mutexWait(Mutex);

  _ClearCall();

  if (t38engine)
    _Detach(t38engine);

  if (audioEngine)
    _Detach(audioEngine);
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

PBoolean ModemEngineBody::Request(PStringToString &request)
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

PBoolean ModemEngineBody::Attach(T38Engine *_t38engine)
{
  if (_t38engine == NULL) {
    myPTRACE(1, "ModemEngineBody::Attach _t38engine==NULL");
    return FALSE;
  }

  PTRACE(1, "ModemEngineBody::Attach t38engine");

  PWaitAndSignal mutexWait(Mutex);

  for (;;) {
    if (t38engine != NULL) {
      myPTRACE(1, "ModemEngineBody::Attach Other t38engine already Attached");
      return FALSE;
    }

    if (_t38engine->TryLockModemCallback()) {
      if (!_t38engine->Attach(engineCallback)) {
        myPTRACE(1, "ModemEngineBody::Attach Can't Attach engineCallback to _t38engine");
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
      _t38engine->Detach(engineCallback);
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

PBoolean ModemEngineBody::Attach(AudioEngine *_audioEngine)
{
  if (_audioEngine == NULL) {
    myPTRACE(1, "ModemEngineBody::Attach _audioEngine==NULL");
    return FALSE;
  }

  PTRACE(1, "ModemEngineBody::Attach audioEngine");

  PWaitAndSignal mutexWait(Mutex);

  for (;;) {
    if (audioEngine != NULL) {
      myPTRACE(1, "ModemEngineBody::Attach Other audioEngine already Attached");
      return FALSE;
    }

    if (_audioEngine->TryLockModemCallback()) {
      if (!_audioEngine->Attach(engineCallback)) {
        myPTRACE(1, "ModemEngineBody::Attach Can't Attach engineCallback to _audioEngine");
        _audioEngine->UnlockModemCallback();
        return FALSE;
      }
      _audioEngine->UnlockModemCallback();
      audioEngine = _audioEngine;
      break;
    }

    Mutex.Signal();
    PThread::Sleep(20);
    Mutex.Wait();
  }

  audioEngine->AudioClass(P.AudioClass());

  if (callDirection == cdOutgoing && P.FaxClass())
    audioEngine->SendOnIdle(EngineBase::dtCng);

  myPTRACE(1, "ModemEngineBody::Attach audioEngine Attached");
  return TRUE;
}

void ModemEngineBody::_Detach(AudioEngine *_audioEngine)
{
  if (_audioEngine == NULL) {
    myPTRACE(1, "ModemEngineBody::_Detach _audioEngine==NULL");
    return;
  }

  for (;;) {
    if (audioEngine != _audioEngine) {
      myPTRACE(1, "ModemEngineBody::_Detach " << (audioEngine  ? "Other" : "No") << " audioEngine was Attached");
      return;
    }

    if (_audioEngine->TryLockModemCallback()) {
      _audioEngine->Detach(engineCallback);
      _audioEngine->UnlockModemCallback();
      audioEngine = NULL;
      break;
    }

    Mutex.Signal();
    PThread::Sleep(20);
    Mutex.Wait();
  }

  myPTRACE(1, "ModemEngineBody::_Detach audioEngine Detached");
}

void ModemEngineBody::OnEngineCallback(PObject & PTRACE_PARAM(from), INT extra)
{
  PTRACE(extra < 0 ? 2 : 4, "ModemEngineBody::OnEngineCallback "
      << from.GetClass() << " " << EngineBase::ModemCallbackParam(extra));

  PWaitAndSignal mutexWait(Mutex);

  if (extra == seq) {
    switch (state) {
      case stSendAckWait:
        state = stSendAckHandle;
        timeout.Stop();
        break;
      case stRecvBegWait:
        state = stRecvBegHandle;
        timeout.Stop();
        break;
    }
  }
  else
  switch (extra) {
    case EngineBase::cbpReset:
      switch (state) {
        case stSend:
          param = state;
          state = stResetHandle;
          break;
      }
      break;
    case EngineBase::cbpOutBufEmpty:
      switch (state) {
        case stSend:
          state = stSendBufEmptyHandle;
          break;
      }
      break;
    case EngineBase::cbpOutBufNoFull:
    case EngineBase::cbpUserInput:
      break;
    default:
      myPTRACE(1, "ModemEngineBody::OnEngineCallback extra(" << extra << ") != seq(" << seq << ")");
  }

  parent.SignalDataReady();
}

void ModemEngineBody::OnTimerCallback(PObject & PTRACE_PARAM(from), INT PTRACE_PARAM(extra))
{
  PTRACE(2, "ModemEngineBody::OnTimerCallback " << from.GetClass() << " " << extra);

  parent.SignalDataReady();
}

static int ParseNum(const char **ppCmd,
  PINDEX minDigits = 1, PINDEX maxDigits = 3, int maxNum = 255, int defNum = 0)
{
    const char *pEnd = *ppCmd;
    int num = 0;
    
    for( ; isdigit(*pEnd) ; pEnd++ ) {
      num = (num * 10) + (*pEnd - '0');
    }
    
    PINDEX len = PINDEX(pEnd - *ppCmd);
    *ppCmd = pEnd;
    
    if (len < minDigits || len > maxDigits || num > maxNum)
      return -1;
    else
    if (!len)
      return defNum;

    return num;
}

PBoolean ModemEngineBody::HandleClass1Cmd(const char **ppCmd, PString &resp, PBoolean &ok, PBoolean &crlf)
{
  PBoolean T;
  
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
      dt = EngineBase::dtSilence;
      break;
    case 'M':
      dt = EngineBase::dtRaw;
      break;
    case 'H':
      dt = EngineBase::dtHdlc;
      break;
    default:
      return FALSE;
  }
  
  if (dt == EngineBase::dtSilence) {
    switch( *(*ppCmd)++ ) {
      case '=':
        switch( **ppCmd ) {
          case '?':
            (*ppCmd)++;
            resp += "\r\n0-255";
            crlf = TRUE;
            break;
          default:
            if (P.FaxClass()) {
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
            } else {
              return FALSE;
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
            if (dt == EngineBase::dtRaw)
              resp += "\r\n24,48,72,73,74,96,97,98,121,122,145,146";
            else
              resp += "\r\n3";
            crlf = TRUE;
            break;
          default:
            if (P.FaxClass()) {
              int br = ParseNum(ppCmd);

              if ((dt == EngineBase::dtRaw && br == 3) || (dt == EngineBase::dtHdlc && br != 3))
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
                    PBoolean res = T ? SendStart(dt, br, _resp) : RecvStart(dt, br);

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
            } else {
              return FALSE;
            }
        }
        break;
      default:
        return FALSE;
    }
  }
  return TRUE;
}

PBoolean ModemEngineBody::HandleClass8Cmd(const char **ppCmd, PString &resp, PBoolean &ok, PBoolean &crlf)
{
  PBoolean T;

  switch (*(*ppCmd - 2)) {
    case 'T':
      T = TRUE;
      break;
    case 'R':
      T = FALSE;
      break;
    default:
      return FALSE;
  }

#define TONE_FREQUENCY_MIN	PDTMFEncoder::MinFrequency
#define TONE_FREQUENCY_MAX	PDTMFEncoder::MaxFrequency
#define TONE_DMS_MAX		500
#define TONE_VOLUME		15

  switch (*(*ppCmd - 1)) {
    case 'S':
      switch (*(*ppCmd)++) {
        case '=':
          switch (**ppCmd) {
            case '?': {
              (*ppCmd)++;
              resp += PString(PString::Printf, "\r\n(0,%u-%u),(0,%u-%u),(0-%u)",
                              (unsigned)TONE_FREQUENCY_MIN, (unsigned)TONE_FREQUENCY_MAX,
                              (unsigned)TONE_FREQUENCY_MIN, (unsigned)TONE_FREQUENCY_MAX,
                              (unsigned)TONE_DMS_MAX);
              crlf = TRUE;
              break;
            }
            default:
              if (P.AudioClass()) {
                ok = FALSE;

                PDTMFEncoder tone;

                for (;;) {
                  int dms = PDTMFEncoder::DefaultToneLen/10;

                  switch (**ppCmd) {
                    case '[': {
                      (*ppCmd)++;

                      int f1 = 0;
                      int f2 = 0;

                      f1 = ParseNum(ppCmd, 0, 5, TONE_FREQUENCY_MAX, f1);

                      if (f1 && f1 < TONE_FREQUENCY_MIN) {
                        myPTRACE(1, "Parse error: wrong f1 before " << *ppCmd);
                        return FALSE;
                      }

                      if (**ppCmd == ',') {
                        (*ppCmd)++;
                        f2 = ParseNum(ppCmd, 0, 5, TONE_FREQUENCY_MAX, f2);

                        if (f2 && f2 < TONE_FREQUENCY_MIN) {
                          myPTRACE(1, "Parse error: wrong f2 before " << *ppCmd);
                          return FALSE;
                        }

                        if (**ppCmd == ',') {
                          (*ppCmd)++;
                          dms = ParseNum(ppCmd, 0, 5, TONE_DMS_MAX, dms);

                          if (dms < 0) {
                            myPTRACE(1, "Parse error: wrong dms before " << *ppCmd);
                            return FALSE;
                          }
                        }
                      }

                      if (**ppCmd != ']') {
                        myPTRACE(1, "Parse error: no ']' before " << *ppCmd);
                        return FALSE;
                      }

                      if (dms) {
                        unsigned ms = dms*10;
                        char op;

                        if (f1 && f2) {
                          op = '+';
                        }
                        else
                        if (f1) {
                          op = '-';
                        }
                        else
                        if (f2) {
                          op = '-';
                          f1 = f2;
                          f2 = 0;
                        }
                        else {
                          op = ' ';
                        }

                        if (!tone.Generate(op, f1, f2, ms, TONE_VOLUME)) {
                          myPTRACE(1, "Cannot encode tone \"" << f1 << op << f2 << ":" << ms << "\"");
                          return FALSE;
                        }

                        myPTRACE(2, "Encoded tone \"" << f1 << op << f2 << ":" << ms << "\", size=" << tone.GetSize());
                      }

                      (*ppCmd)++;
                      break;
                    }
                    case '{': {
                      (*ppCmd)++;

                      char dtmf = **ppCmd;

                      if (isdigit(dtmf) || (dtmf >= 'A' && dtmf <= 'C') || dtmf == '*' || dtmf == '#')
                        (*ppCmd)++;
                      else
                        dtmf = ' ';

                      if (**ppCmd == ',') {
                        (*ppCmd)++;
                        dms = ParseNum(ppCmd, 0, 5, TONE_DMS_MAX, dms);

                        if (dms < 0) {
                          myPTRACE(1, "Parse error: wrong dms before " << *ppCmd);
                          return FALSE;
                        }
                      }

                      if (**ppCmd != '}') {
                        myPTRACE(1, "Parse error: no '}' before " << *ppCmd);
                        return FALSE;
                      }

                      if (dms) {
                        unsigned ms = dms*10;

                        if (dtmf == ' ') {
                          if (!tone.Generate(' ', 0, 0, ms)) {
                            myPTRACE(1, "Cannot encode tone \"0 0:" << ms << "\"");
                            return FALSE;
                          }

                          myPTRACE(2, "Encoded tone \"0 0:" << ms << "\", size=" << tone.GetSize());
                        } else {
                          tone.AddTone(dtmf, ms);
                          myPTRACE(2, "Encoded DTMF tone \"" << dtmf << ":" << ms << "\", size=" << tone.GetSize());
                        }
                      }

                      (*ppCmd)++;
                      break;
                    }
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case 'A':
                    case 'B':
                    case 'C':
                    case '*':
                    case '#': {
                      char dtmf = *(*ppCmd)++;
                      unsigned ms = dms*10;

                      tone.AddTone(dtmf, ms);
                      myPTRACE(2, "Encoded DTMF tone \"" << dtmf << ":" << ms << "\", size=" << tone.GetSize());
                      break;
                    }
                    case ',': {
                      unsigned ms = dms*10;

                      if (!tone.Generate(' ', 0, 0, ms)) {
                        myPTRACE(1, "Cannot encode tone \"0 0:" << ms << "\"");
                        return FALSE;
                      }

                      myPTRACE(2, "Encoded tone \"0 0:" << ms << "\", size=" << tone.GetSize());
                      break;
                    }
                    default:
                      break;
                  }

                  if (**ppCmd != ',')
                    break;

                  (*ppCmd)++;
                }

                PWaitAndSignal mutexWait(Mutex);
                dataType = EngineBase::dtRaw;
                moreFrames = FALSE;
                state = stSend;
                if (audioEngine && audioEngine->SendStart(dataType, 0)) {
                  state = stSendAckWait;

                  PINDEX len = tone.GetSize();

                  if (len) {
                    const PInt16 *ps = tone.GetPointer();
                    audioEngine->Send(ps, len*sizeof(*ps));
                  }

                  if (!audioEngine->SendStop(FALSE, NextSeq())) {
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
          break;
        default:
          return FALSE;
      }
      break;
    case 'X':
      if (P.AudioClass()) {
        ok = FALSE;

        PString _resp;
        PBoolean res = T ? SendStart(EngineBase::dtRaw, 0, _resp) : RecvStart(EngineBase::dtRaw, 0);

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
      } else {
        return FALSE;
      }
    default:
      return FALSE;
  }

  return TRUE;
}

PBoolean ModemEngineBody::Answer()
{
  PWaitAndSignal mutexWait(Mutex);

  timerRing.Stop();
  callDirection = cdIncoming;
  forceFaxMode = P.FaxClass();

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
      return FALSE;
    }
  } else {
    timeout.Stop();
    param = chHandle;
    state = stConnectHandle;
    parent.SignalDataReady();
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

  PBoolean err = FALSE;
  PBoolean ok = TRUE;
  PBoolean crlf = FALSE;

  while (state == stCommand && !err && *pCmd) {
      switch( *pCmd++ ) {
        case ' ':
          break;
        case 'A':	// Accept incoming call
          ok = FALSE;

          if (!Answer())
            err = TRUE;

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
            PBoolean local = FALSE;
            PBoolean setForceFaxMode = FALSE;
          
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

              if (P.FaxClass())
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

            P.ModemClass("1");

            if (audioEngine)
              audioEngine->AudioClass(P.AudioClass());
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
                          if( val < 0 || !P.SetBit(r, b, (PBoolean)val) ) {
                            err = TRUE;
                          }
                        }
                        break;
                      case '?':
                        {
                          PBoolean val;
                
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

              if (audioEngine)
                audioEngine->AudioClass(P.AudioClass());
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
                  if (*pCmd == 'A') {			// +FAA
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
                            resp += "\r\n1,8";
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
                              case 8:
                                P.ModemClass("8");
                                break;
                              default:
                                err = TRUE;
                            }

                            if (audioEngine)
                              audioEngine->AudioClass(P.AudioClass());
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
                    case 'O':				// +FLO
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
                        case 'L':			// +FMDL
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
                        case 'R':			// +FMFR
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
                    case 'I':				// +FMI
                      switch (*pCmd++) {
                        case '?':
                          resp += "\r\n" + PString(Manufacturer);
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'M':				// +FMM
                      switch (*pCmd++) {
                        case '?':
                          resp += "\r\n" + PString(Model);
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'R':				// +FMR
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
            case 'I':	// DTE-DCE interface commands
              switch (*pCmd++) {
                case 'F':
                  switch (*pCmd++) {
                    case 'C':				// +IFC
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n+IFC:(0-2),(0-2)";
                              crlf = TRUE;
                              break;
                            default: {
                              int valByDTE = ParseNum(&pCmd, 1, 1, 2);

                              if (valByDTE < 0) {
                                err = TRUE;
                                break;
                              }

                              int valByDCE;

                              if (*pCmd == ',') {
                                pCmd++;

                                valByDCE = ParseNum(&pCmd);

                                if (valByDCE < 0) {
                                  err = TRUE;
                                  break;
                                }
                              } else {
                                valByDCE = P.IfcByDCE();
                              }

                              P.IfcByDTE((BYTE)valByDTE);
                              P.IfcByDCE((BYTE)valByDCE);
                            }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n+IFC:%u,%u", (unsigned)P.IfcByDTE(), (unsigned)P.IfcByDCE());
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
                default:
                  err = TRUE;
              }
              break;
            case 'V':	// Voice
              switch (*pCmd++) {
                case 'C':
                  if (strncmp(pCmd, "ID", 2) == 0) {	// +VCID
                    pCmd += 2;
                    switch (*pCmd++) {
                      case '=':
                        switch (*pCmd) {
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
                  } else {
                    err = TRUE;
                  }
                  break;
                case 'L':
                  switch (*pCmd++) {
                    case 'S':				// +VLS
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n0,\"\",00000000,00000000,00000000";
                              resp += "\r\n1,\"T\",00000000,00000000,00000000";
                              crlf = TRUE;
                              break;
                            default:
                              switch (ParseNum(&pCmd)) {
                                case 0: {
                                  PWaitAndSignal mutexWait(Mutex);

                                  if (callDirection != cdUndefined || P.ClearMode())
                                    _ClearCall();
                                  break;
                                }
                                case 1:
                                  ok = FALSE;

                                  if (!Answer())
                                    err = TRUE;
                                  break;
                                default:
                                  err = TRUE;
                              }
                          }
                          break;
                        case '?':
                          resp += connectionEstablished ? "\r\n1" : "\r\n0";
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
                  switch (*pCmd++) {
                    case 'X':				// +VRX
                      if (!HandleClass8Cmd(&pCmd, resp, ok, crlf))
                        err = TRUE;
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'S':
                  switch (*pCmd++) {
                    case 'M':				// +VSM
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n132,\"G.711 ALAW\",8,0,(8000),(0),(0)";
                              crlf = TRUE;
                              break;
                            default: {
                              int cml = ParseNum(&pCmd);

                              switch (cml) {
                                case 132:
                                  break;
                                default:
                                  err = TRUE;
                              }

                              if (err)
                                break;

                              if (*pCmd != ',') {
                                err = TRUE;
                                break;
                              }

                              pCmd++;

                              int vsr = ParseNum(&pCmd, 4, 4, 8000);

                              if (vsr != 8000) {
                                err = TRUE;
                                break;
                              }

                              int scs = 0;
                              int sel = 0;

                              if (*pCmd == ',') {
                                pCmd++;
                                scs = ParseNum(&pCmd);

                                if (scs < 0) {
                                  err = TRUE;
                                  break;
                                }

                                if (*pCmd == ',') {
                                  pCmd++;
                                  sel = ParseNum(&pCmd);

                                  if (sel < 0) {
                                    err = TRUE;
                                    break;
                                  }
                                }
                              }
                            }
                          }
                          break;
                        case '?':
                          resp += "\r\n132";
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
                case 'T':
                  switch (*pCmd) {
                    case 'X':				// +VTX
                    case 'S':				// +VTS
                      pCmd++;
                      if (!HandleClass8Cmd(&pCmd, resp, ok, crlf))
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

                if (audioEngine)
                  audioEngine->AudioClass(P.AudioClass());
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
                    resp += "\r\n(0,1,10,11)";
                    crlf = TRUE;
                    break;
                  default:
                    {
                      int val = ParseNum(&pCmd);
                      switch (val) {
                        case 0:
                          P.CidMode(0);
                          P.DidMode(0);
                          break;
                        case 1:
                          P.CidMode(1);
                          P.DidMode(0);
                          break;
                        case 10:
                          P.CidMode(1);
                          P.DidMode(1);
                          break;
                        case 11:
                          P.CidMode(0);
                          P.DidMode(1);
                          break;
                        default:
                          err = TRUE;
                      }
                    }
                }
                break;
              case '?': {
                unsigned cid;

                if (P.CidMode()) {
                  if (P.DidMode()) {
                    cid = 10;
                  } else {
                    cid = 1;
                  }
                } else {
                  if (P.DidMode()) {
                    cid = 11;
                  } else {
                    cid = 0;
                  }
                }

                resp.sprintf("\r\n%u", P.CidMode());
                crlf = TRUE;
                break;
              }
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
              int rlen = int(pEnd - pBuf);
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

            int dt = dataType;
            BYTE Buf[1024];

            for(;;) {
              int count = dleData.GetData(Buf, sizeof(Buf));

              PWaitAndSignal mutexWait(Mutex);
              switch( count ) {
                case -1:
                  state = stSendAckWait;
                  if ((P.AudioClass() && (!audioEngine || !audioEngine->SendStop(moreFrames, NextSeq()))) ||
                      (P.FaxClass() && (!t38engine || !t38engine->SendStop(moreFrames, NextSeq()))))
                  {
                    PString resp = RC_PREF() + RC_ERROR();

                    bresp.Concatenate(PBYTEArray((const BYTE *)resp, resp.GetLength()));
                    state = stCommand;
                  }
                  break;
                case 0:
                  break;
                default:
                  switch( dt ) {
                    case EngineBase::dtHdlc:
                      if( dataCount < 2 && (dataCount + count) >= 2 && 
                          			(Buf[1 - dataCount] & 0x08) == 0 ) {
                        moreFrames = TRUE;
                      }
                  }
                  dataCount += count;
                  if (P.AudioClass()) {
                    if (audioEngine) {
                      const signed char *pb = (const signed char *)Buf;
                      PInt16 Buf2[sizeof(Buf)];
                      PInt16 *ps = Buf2;

                      while (count--)
                        *ps++ = (PInt16)alaw2linear(*pb++);

                      count = int(pb - (const signed char *)Buf);

                      audioEngine->Send(Buf2, count*sizeof(*ps));
                    }
                  } else {
                    if (t38engine)
                      t38engine->Send(Buf, count);
                  }
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
            timeout.Stop();

            PString resp = RC_PREF();

            if (P.AudioClass()) {
              if (state == stRecv)
                bresp.Concatenate(PBYTEArray((const BYTE *)"\x10\x03", 2));	// add <DLE><ETX>

              state = stCommand;
              resp += RC_OK();
            } else {
              state = stCommand;

              if (t38engine) {
                t38engine->ResetModemState();
                resp += RC_OK();
              } else {
                _ClearCall();
                resp += RC_NO_CARRIER();
              }
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
      if (!ringCount) {
        if(P.CidMode())
          resp += "NMBR = " + SrcNum() + "\r\n";
        if(P.DidMode())
          resp += "NDID = " + DstNum() + "\r\n";
        if(P.CidMode() || P.DidMode())
          resp += RC_RING();
      }
      P.SetReg(1, ++ringCount);
      if (s0 > 0 && (ringCount >= s0)) {
        Mutex.Signal();
        Answer();
        Mutex.Wait();
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

  if (connectionEstablished && P.AudioClass()) {
    PWaitAndSignal mutexWait(Mutex);

    if (audioEngine) {
      for(;;) {
        BYTE b[2];

        if (audioEngine->RecvUserInput(&b[1], 1) <= 0)
          break;

        b[0] = '\x10';		// <DLE>

        PBYTEArray _bresp(b, 2);
        bresp.Concatenate(_bresp);
        PTRACE(2, "<-- DLE " << PRTHEX(_bresp));
      }
    }
  }

  switch( state ) {
    case stResetHandle:
      {
        PWaitAndSignal mutexWait(Mutex);

        if (!connectionEstablished && P.AudioClass()) {
          PBYTEArray _bresp((const BYTE *)"\x10" "b", 2);		// <DLE>b
          bresp.Concatenate(_bresp);
          PTRACE(2, "<-- DLE " << PRTHEX(_bresp));
        }

        switch( param ) {
          case stCommand:
            break;
          case stSend:
            resp = RC_ERROR();
            break;
          case stRecvBegWait:
            resp = RC_NO_CARRIER();
            break;
          case stRecv:
            if (dataCount || P.AudioClass()) {
              PBYTEArray _bresp((const BYTE *)"\x10\x03", 2);		// <DLE><ETX>
              bresp.Concatenate(_bresp);
              PTRACE(2, "<-- DLE " << PRTHEX(_bresp));
            }

            resp = P.AudioClass() ? RC_OK() : RC_ERROR();
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

            if (P.AudioClass()) {
              state = stCommand;
              timeout.Stop();
              resp = RC_OK();
            }
            else
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
            dataType = EngineBase::dtCed;
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
              t38engine->SendOnIdle(EngineBase::dtCng);
            if (!RecvStart(EngineBase::dtHdlc, 3))
              resp = RC_ERROR();
            break;
          default:
            resp = RC_NO_CARRIER();
            state = stCommand;
        }
      }
      break;
    case stSendAckHandle:
      switch( dataType ) {
        case EngineBase::dtCed:
          if (!SendStart(EngineBase::dtHdlc, 3, resp))
            resp = RC_ERROR();
          break;
        case EngineBase::dtSilence:
            resp = RC_OK();
            state = stCommand;
          break;
        case EngineBase::dtHdlc:
        case EngineBase::dtRaw:
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

        if (P.AudioClass() && audioEngine && audioEngine->RecvStart(NextSeq())) {
          resp = RC_CONNECT();
          dataCount = 0;
          state = stRecv;
          parent.SignalDataReady();	// try to Recv w/o delay
        }
        else
        if (P.FaxClass() && t38engine && t38engine->RecvStart(NextSeq())) {
          if ((t38engine->RecvDiag() & EngineBase::diagDiffSig) == 0) {
            if (dataType != EngineBase::dtRaw) {
              resp = RC_CONNECT();

              if (dataType == EngineBase::dtHdlc)
                fcs = FCS();
            }
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
      }
      break;
    case stRecv:
      {
        BYTE Buf[1024];
        int count;

        for(;;) {
          PWaitAndSignal mutexWait(Mutex);

          if (P.AudioClass()) {
            if (!audioEngine) {
              dleData.PutEof();
              count = -1;
              break;
            }
            count = audioEngine->Recv(Buf, sizeof(Buf));
          } else {
            if (!t38engine) {
              dleData.SetDiag(EngineBase::diagError).PutEof();
              count = -1;
              break;
            }
            count = t38engine->Recv(Buf, sizeof(Buf));
          }

          switch (count) {
            case -1:
              if (P.AudioClass()) {
                dleData.PutEof();
                audioEngine->RecvStop();
              } else {
                int diag = t38engine->RecvDiag();

                if (dataType == EngineBase::dtHdlc) {
                  Buf[0] = BYTE(fcs >> 8);
                  Buf[1] = BYTE(fcs);
                  if (diag & EngineBase::diagBadFcs)
                     Buf[0]++;
                  dleData.PutData(Buf, 2);
                }
                dleData.SetDiag(diag).PutEof();
                t38engine->RecvStop();
              }
              if (dataCount == 0 && P.FaxClass())
                dleData.GetDleData(Buf, sizeof(Buf));	// discard ...<DLE><ETX>
              break;
            case 0:
              break;
            default:
              if (P.FaxClass()) {
                dleData.PutData(Buf, count);

                switch (dataType) {
                case EngineBase::dtHdlc:
                  fcs.build(Buf, count);
                  break;
                case EngineBase::dtRaw:
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
              } else {
                const PInt16 *ps = (const PInt16 *)Buf;
                signed char *pb = (signed char *)Buf;

                count /= sizeof(*ps);

                while (count--)
                  *pb++ = (signed char)linear2alaw(*ps++);

                count = int(pb - (signed char *)Buf);

                dleData.PutData(Buf, count);
              }

              dataCount += count;
          }
          if (count <= 0)
            break;
        }

        if (P.AudioClass()) {
          if (count < 0) {
            PBYTEArray _bresp((const BYTE *)"\x10" "b", 2);	// <DLE>b
            bresp.Concatenate(_bresp);
            PTRACE(2, "<-- DLE " << PRTHEX(_bresp));
          }
        }

        for(;;) {
          PWaitAndSignal mutexWait(Mutex);

          count = dleData.GetDleData(Buf, sizeof(Buf));

          switch (count) {
            case -1:
              {
                state = stCommand;

                if (P.AudioClass()) {
                  resp = RC_OK();
                }
                else
                if (dataType == EngineBase::dtHdlc) {
                  int diag = dleData.GetDiag();

                  if (diag == 0)
                    resp = RC_OK();
                  else
                  if (dataCount == 0 && (diag & ~EngineBase::diagNoCarrier) == 0)
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
            default: {
              PBYTEArray _bresp(PBYTEArray(Buf, count));
#if PTRACING
              if (myCanTrace(2)) {
                 if (count <= 16) {
                   PTRACE(2, "<-- DLE " << PRTHEX(_bresp));
                 } else {
                   PTRACE(2, "<-- DLE " << count << " bytes");
                 }
              }
#endif
              bresp.Concatenate(_bresp);
            }
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

