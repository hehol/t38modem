/*
 * pmodeme.cxx
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
 * $Log: pmodeme.cxx,v $
 * Revision 1.13  2002-05-15 16:10:52  vfrolov
 * Reimplemented AT+FTS and AT+FRS
 * Added workaround "Reset state stSendAckWait"
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

#include "pmodemi.h"
#include "pmodeme.h"
#include "dle.h"
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
    CidModeReg = 50,
    MaxReg = 50,
    MaxBit = 7
  };
  
  public:
    Profile();
  
    void Echo(BOOL val) {
      SetBit(23, 0, val);
    }

    BOOL Echo() const {
      BOOL val;
      GetBit(23, 0, val);
      return val;
    }

    void asciiResultCodes(BOOL val) {
      SetBit(23, 6, val);
    }

    BOOL asciiResultCodes() const {
      BOOL val;
      GetBit(23, 6, val);
      return val;
    }

    void noResultCodes(BOOL val) {
      SetBit(23, 7, val);
    }

    BOOL noResultCodes() const {
      BOOL val;
      GetBit(23, 7, val);
      return val;
    }
    
    void CidMode(BYTE val) {
      SetReg(CidModeReg, val);
    }

    BYTE CidMode() const {
      BYTE val;
      GetReg(CidModeReg, val);
      return val;
    }

    BOOL SetBit(PINDEX r, PINDEX b, BOOL val) {
      if( !ChkRB(r, b) ) return FALSE;
      BYTE msk = MaskB(b);
      if( val ) S[r] |= msk;
      else      S[r] &= ~msk;
      return TRUE;
    }
    BOOL GetBit(PINDEX r, PINDEX b, BOOL &val) const {
      if( !ChkRB(r, b) ) return FALSE;
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
      if( !ChkR(r) ) return FALSE;
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
      if( !ChkRBB(r, bl, bh) ) return FALSE;
      BYTE msk = MaskBB(bl, bh);
      val = (S[r] & msk) >> bl;
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
      return ("\x01\x03\x07\x0F\x1F\x3F\x7F\xFF"[bh - bl]) << bl;
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
    void Detach(T38Engine *_t38engine);
    void HandleData(const PBYTEArray &buf, PBYTEArray &bresp);
    void CheckState(PBYTEArray &bresp);
    BOOL IsReady() const {
      PWaitAndSignal mutexWait(Mutex);
      return state == stCommand && (PTime() - lastPtyActivity) > 5*1000;
    }
  //@}
  
  protected:
    BOOL Echo() const { return P.Echo(); }
    BOOL HandleClass1Cmd(const char **ppCmd, PString &resp);
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
        resp += "\r\nCONNECT\r\n";
        return TRUE;
      } else {
        state = stCommand;
        return FALSE;
      }
    }
    
    BOOL RecvStart(int _dataType, int br) {
      PWaitAndSignal mutexWait(Mutex);
      dataType = _dataType;
      state = stRecvBegWait;
      timeout.Start(60000);
      if( !t38engine || !t38engine->RecvWait(dataType, br, NextSeq()) ) {
        state = stCommand;
        timeout.Stop();
        return FALSE;
      }
      return TRUE;
    }
    
    void ClearCall() { PWaitAndSignal mutexWait(Mutex); _ClearCall(); }
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

    Profile P;
    Profile Profiles[1];

    PMutex Mutex;
    
    DeclareStringParam(CallToken)
    DeclareStringParam(SrcNum)
    DeclareStringParam(DstNum)

};
///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
ModemEngine::ModemEngine(PseudoModemBody &_parent)
  : ModemThreadChild(_parent)
{
  SetThreadName(ptyName() + "(e):%0x");
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

    PBYTEArray *buf = Parent().FromInPtyQ();

    if (buf)  {
      body->HandleData(*buf, bresp);
      delete buf;
      if (stop)
        break;
    }
    
    if (bresp.GetSize()) {
      ToPtyQ(bresp, bresp.GetSize());
    }

    if (stop)
      break;

    WaitDataReady();
  }

  myPTRACE(1, "<-> Stopped");
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
    myCallback(PCREATE_NOTIFIER(OnMyCallback)),
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
  ClearCall();

  if( t38engine ) {
    myPTRACE(1, "ModemEngineBody::~ModemEngineBody t38engine was not Detached");
    Detach(t38engine);
  }
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
    if (CallToken().IsEmpty()) {
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
  PTRACE(1, "ModemEngineBody::Attach");
  PWaitAndSignal mutexWait(Mutex);

  if( t38engine != NULL ) {
    myPTRACE(1, "ModemEngineBody::Attach other t38engine already Attached");
    return FALSE;
  }
  t38engine = _t38engine;
  if( !t38engine || !t38engine->Attach(myCallback) ) {
    myPTRACE(1, "ModemEngineBody::Attach can't Attach myCallback to t38engine");
    t38engine = NULL;
    return FALSE;
  }
  
  if( state == stReqModeAckWait ) {
    state = stReqModeAckHandle;
    timeout.Stop();
    parent.SignalDataReady();
  }
  
  myPTRACE(1, "ModemEngineBody::Attach t38engine Attached");
  return TRUE;
}

void ModemEngineBody::Detach(T38Engine *_t38engine)
{
  PWaitAndSignal mutexWait(Mutex);
  if( t38engine == _t38engine ) {
    t38engine = NULL;
    myPTRACE(1, "ModemEngineBody::Detach t38engine Detached");
  } else {
    myPTRACE(1, "ModemEngineBody::Detach other t38engine was Attached");
  }
  if( _t38engine )
    _t38engine->Detach(myCallback);
}

void ModemEngineBody::OnMyCallback(PObject &from, INT extra)
{
  PTRACE(1, "ModemEngineBody::OnMyCallback " << from.GetClass() << " " << extra);
  if( from.IsDescendant(T38Engine::Class()) ) {
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

BOOL ModemEngineBody::HandleClass1Cmd(const char **ppCmd, PString &resp)
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
              resp += "\r\n0-255\r\nOK\r\n";
            break;
          default:
            {
              int dms = ParseNum(ppCmd);
              if( dms >= 0 ) {
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
              resp += "\r\n24,48,72,73,74,96,97,98,121,122,145,146\r\nOK\r\n";
            else
              resp += "\r\n3\r\nOK\r\n";
            break;
          default:
            {
              int br = ParseNum(ppCmd);
              
              if( (dt == T38Engine::dtRaw && br == 3) || (dt == T38Engine::dtHdlc && br != 3) ) {
                return FALSE;
	      }
                
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
                  if( T )
                    return SendStart(dt, br, resp);
                  else
                    return RecvStart(dt, br);
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
  if (!(cmd.Left(2) *= "AT"))
    return;
    
  PString ucmd = cmd.ToUpper();
  const char * pCmd = ucmd;
  pCmd += 2;

  BOOL err = FALSE;
  BOOL ok = TRUE;
  BOOL crlf = TRUE;
  
  while (state == stCommand && !err && *pCmd)  {
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
              crlf = FALSE;
            }
          }
          break;
        case 'D':	// Dial
          ok = FALSE;
          forceFaxMode = FALSE;
          {
            PString num;
          
            for( char ch ; (ch = *pCmd) != 0 && !err ; pCmd++ ) {
              if( isdigit(ch) ) {
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
                    forceFaxMode = TRUE;
                    break;
                  case 'V':
                    forceFaxMode = FALSE;
                    break;
                  default:
                    err = TRUE;
                }
              }
            }
            
            if (!err) {
              PWaitAndSignal mutexWait(Mutex);
              callDirection = cdOutgoing;
              timerRing.Stop();
              state = stConnectWait;
              timeout.Start(60000);
            
              PStringToString request;
              request.SetAt("modemtoken", parent.modemToken());
              request.SetAt("command", "dial");
              request.SetAt("number", num);
            
              Mutex.Signal();
              callbackEndPoint(request, 3);
              Mutex.Wait();

              PString response = request("response");
              
              if (response == "confirm") {
                CallToken(request("calltoken"));
                crlf = FALSE;
              } else {
                callDirection = cdUndefined;
                forceFaxMode = FALSE;
                timeout.Stop();
                state = stCommand;
                if (response == "reject") {
                  PString diag = request("diag");
                  if( diag == "noroute" )
                    resp += "\r\nBUSY";
                  else
                    resp += "\r\nNO DIALTONE";
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
            if( callDirection != cdUndefined )
              ClearCall();
          } else {
            err = TRUE;
          }
          break;
        case 'I':	// Information
          {
            int val = ParseNum(&pCmd, 0, 1);
            
            switch( val ) {
              case 0:
                resp += "\r\n";
                resp += Model;
                break;
              case 3:
                resp += "\r\n";
                resp += Manufacturer;
                break;
              case 8:
                resp += "\r\nNMBR=" + SrcNum();
                break;
              case 9:
                resp += "\r\nNDID=" + DstNum();
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
        case 'Z':	// Load Registers from Profile
          {
            int val = ParseNum(&pCmd, 0, 1, sizeof(Profiles)/sizeof(Profiles[0]) - 1);
            if( val >= 0 ) {
              if( callDirection != cdUndefined )
                ClearCall();
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
                            break;
                          default:
                            switch( ParseNum(&pCmd) ) {
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
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                  break;
                case 'M':
                  if( strncmp(pCmd, "FR", 2) == 0 ) {	// +FMFR
                    pCmd += 2;
                    switch( *pCmd++ ) {
                      case '?':
                        resp += "\r\n";
                        resp += Manufacturer;
                        break;
                      default:
                        err = TRUE;
                    }
                  } else if( strncmp(pCmd, "DL", 2) == 0 ) {	// +FMDL
                    pCmd += 2;
                    switch( *pCmd++ ) {
                      case '?':
                        resp += "\r\n";
                        resp += Model;
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                  break;
                case 'R':
                  switch( *pCmd ) {
                    case 'M':				// +FRM
                    case 'H':				// +FRH
                    case 'S':				// +FRS
                      pCmd++;
                      ok = FALSE;
                      if( HandleClass1Cmd(&pCmd, resp) )
                        crlf = FALSE;
                      else
                        err = TRUE;
                      break;
                    default:
                      if( strncmp(pCmd, "EV", 2) == 0 ) {	// +FREV
                        pCmd += 2;
                        switch( *pCmd++ ) {
                          case '?':
                            resp += "\r\n";
                            resp += Revision;
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
                      ok = FALSE;
                      if( HandleClass1Cmd(&pCmd, resp) )
                        crlf = FALSE;
                      else
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
              if( callDirection != cdUndefined )
                ClearCall();
              P = Profiles[0];
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
          if( strncmp(pCmd, "CID", 3) == 0 ) {		// #CID
            pCmd += 3;
            switch( *pCmd++ ) {
              case '=':
                switch( *pCmd ) {
                  case '?':
                    pCmd++;
                    resp += "\r\n(0,10)";
                    break;
                  default:
                    {
                      int val = ParseNum(&pCmd);
                      switch( val ) {
                        case 0:
                        case 10:
                          P.CidMode(val);
                          break;
                        default:
                          err = TRUE;
                      }
                    }
                }
                break;
              case '?':
                resp.sprintf("\r\n%u", (unsigned)P.CidMode());
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
  if( err ) {
    resp += "\r\nERROR";
  } else if( ok ) {
    resp += "\r\nOK";
  }
  if( crlf )
    resp += "\r\n";
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
    
              if( Echo() )
                bresp.Concatenate(PBYTEArray((const BYTE *)"\r", 1));
        
              PString resp;
        
              myPTRACE(1, "--> " << " " << cmd);
              HandleCmd(cmd, resp);
              
              if( resp.GetLength() ) {
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
                    bresp.Concatenate(PBYTEArray((const BYTE *)"\r\nERROR\r\n", 9));
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
            if (t38engine) {
              t38engine->ResetModemState();
              bresp.Concatenate(PBYTEArray((const BYTE *)"\r\nOK\r\n", 6));
            } else {
              _ClearCall();
              bresp.Concatenate(PBYTEArray((const BYTE *)"\r\nNO CARRIER\r\n", 14));
            }
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
      resp += "\r\nRING\r\n";
      BYTE s0, ringCount;
      P.GetReg(0, s0);
      P.GetReg(1, ringCount);
      if (!ringCount && P.CidMode() == 10) {
        resp += "NMBR = " + SrcNum();
        resp += "\r\nNDID = " + DstNum();
        resp += "\r\nRING\r\n";
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
        resp += "\r\nNO CARRIER\r\n";
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
            resp += "\r\nERROR\r\n";
            break;
          case stRecvBegWait:
            resp += "\r\nNO CARRIER\r\n";
            break;
          case stConnectWait:
          case stConnectHandle:
          case stReqModeAckWait:
          case stReqModeAckHandle:
            if (callDirection == cdOutgoing)
              resp += "\r\nBUSY\r\n";
            else
              resp += "\r\nERROR\r\n";
            break;
          default:
            resp += "\r\nERROR\r\n";
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
          resp += "\r\nERROR\r\n";
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
                resp += "\r\nNO CARRIER\r\n";
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
                resp += "\r\nERROR\r\n";
                state = stCommand;
              }
            } else {
              resp += "\r\nNO CARRIER\r\n";
              state = stCommand;
            }
            break;
          case cdOutgoing:
            if (t38engine)
              t38engine->SendOnIdle(T38Engine::dtCng);
            if( !RecvStart(T38Engine::dtHdlc, 3) ) {
              resp += "\r\nERROR\r\n";
            }
            break;
          default:
            resp += "\r\nNO CARRIER\r\n";
            state = stCommand;
        }
      }
      break;
    case stSendAckHandle:
      switch( dataType ) {
        case T38Engine::dtCed:
          if( !SendStart(T38Engine::dtHdlc, 3, resp) )
            resp += "\r\nERROR\r\n";
          break;
        case T38Engine::dtSilence:
            resp += "\r\nOK\r\n";
            state = stCommand;
          break;
        case T38Engine::dtHdlc:
        case T38Engine::dtRaw:
          {
            PWaitAndSignal mutexWait(Mutex);
            if( moreFrames ) {
              resp += "\r\nCONNECT\r\n";
              state = stSend;
            } else {
              resp += "\r\nOK\r\n";
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
            resp += "\r\nCONNECT\r\n";
            dataCount = 0;
            state = stRecv;
            parent.SignalDataReady();	// try to Recv w/o delay
          } else {
            t38engine->RecvStop();
            resp += "\r\n+FCERROR\r\n";
            state = stCommand;
          }
        } else {
          resp += "\r\nERROR\r\n";
          state = stCommand;
        }
        ResetDleData();
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
          
          switch( count ) {
            case -1:
              dleData.SetDiag(t38engine->RecvDiag()).PutEof();
              t38engine->RecvStop();
              if( dataCount == 0 && dataType == T38Engine::dtHdlc ) {
                dleData.GetDleData(Buf, 1024);	// discard <DLE><ETX>
              }
              break;
            case 0:
              break;
            default:
              dataCount += count;
              dleData.PutData(Buf, count);
          }
          if( count <= 0 ) break;
        }

        for(;;) {
          int count = dleData.GetDleData(Buf, 1024);
        
          switch( count ) {
            case -1:
              {
                PWaitAndSignal mutexWait(Mutex);
                state = stCommand;
                int diag = dleData.GetDiag();
                
                if( dataType == T38Engine::dtHdlc ) {
                  if( diag == 0 )
                    resp += "\r\nOK\r\n";
                  else if( dataCount == 0 && (diag & ~T38Engine::diagNoCarrier) == 0 )
                    resp += "\r\nNO CARRIER\r\n";
                  else
                    resp += "\r\nERROR\r\n";
                } else {
                  resp += "\r\nNO CARRIER\r\n";
                }
              }
              break;
            case 0:
              break;
            default:
              PTRACE(1, "T38DLE--> " << PRTHEX(PBYTEArray(Buf, count)));
              bresp.Concatenate(PBYTEArray(Buf, count));
          }
          if( count <= 0 ) break;
        }
      }
      break;
  }
  
  if( resp.GetLength() ) {
    PBYTEArray _bresp((const BYTE *)(const char *)resp, resp.GetLength());
    
    myPTRACE(1, "<-- " << PRTHEX(_bresp));
    bresp.Concatenate(_bresp);
  }
}
///////////////////////////////////////////////////////////////

