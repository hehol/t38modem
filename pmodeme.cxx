/*
 * $Id: pmodeme.cxx,v 1.2 2002-01-01 23:59:52 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: pmodeme.cxx,v $
 * Revision 1.2  2002-01-01 23:59:52  craigs
 * Lots of additional implementation thanks to Vyacheslav Frolov
 *
 * Revision 1.2  2002/01/01 23:59:52  craigs
 * Lots of additional implementation thanks to Vyacheslav Frolov
 *
 */

#include "pmodemi.h"
#include "pmodeme.h"
#include "dle.h"
#include "t38engine.h"

///////////////////////////////////////////////////////////////
static const char Manufacturer[] = "Vyacheslav Frolov";
static const char Model[] = "T38FAX";
static const char Revision[] = "0.00b";
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
      stReqModeAckWait,
      stReqModeAckHandle,
      stSend,
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
      return state == stCommand && (PTime() - lastPtyActivity) > 10*1000;
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
    
    void ClearCall();

    int NextSeq() { return seq = ++seq % 255; }

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
    int state;
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
    
    if( stop ) break;
    WaitDataReady();
    if( stop ) break;
    body->CheckState(bresp);
    if( stop ) break;

    PBYTEArray *buf = Parent().FromInPtyQ();

    if( buf ) {
      body->HandleData(*buf, bresp);
      delete buf;
      if( stop ) break;
    }
    
    if( bresp.GetSize() ) {
      ToPtyQ(bresp, bresp.GetSize());
      if( stop ) break;
    }
  }

  myPTRACE(1, "<-> Stoped");
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

void ModemEngineBody::ClearCall()
{
  if( CallToken().IsEmpty() )
    return;
    
  timerRing.Stop();
  
  PStringToString request;
  request.SetAt("modemtoken", parent.modemToken());
  request.SetAt("command", "clearcall");
  request.SetAt("calltoken", CallToken());

  callbackEndPoint(request, 1);
  CallToken("");
  callDirection = cdUndefined;
}

BOOL ModemEngineBody::Request(PStringToString &request)
{
  myPTRACE(1, "ModemEngineBody::Request request={\n" << request << "}");
  
  PString command = request("command");
  
  if( command == "call" ) {
    PWaitAndSignal mutexWait(Mutex);
    CallToken(request("calltoken"));
    SrcNum(request("srcnum"));
    DstNum(request("dstnum"));
    timerRing.Start(6000);
    request.SetAt("response", "confirm");
    request.SetAt("answer", "pending");
    //request.SetAt("answer", "now");
  } else if( command == "clearcall" ) {
    PWaitAndSignal mutexWait(Mutex);
    if( !CallToken().IsEmpty() ) {
      timerRing.Stop();
      // TODO
    }
  } else {
    request.SetAt("response", "reject");
    myPTRACE(1, "ModemEngineBody::Request reject");
  }
  return TRUE;
}

BOOL ModemEngineBody::Attach(T38Engine *_t38engine)
{
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
    } else if( extra == -1 ) {	// reset
      switch( state ) {
        case stSend:
          state = stResetHandle;
          break;
      }
    } else {
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
                // TODO
                PThread::Sleep(dms*10);
                resp += "\r\nOK\r\n";
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
              
              if( (dt == T38Engine::dtRaw && br == 3) || (dt == T38Engine::dtHdlc && br != 3) )
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

void ModemEngineBody::HandleCmd(const PString &cmd, PString &resp)
{
  const PString &ucmd = cmd.ToUpper();
  const char *pCmd = ucmd;
    
  if( strncmp(pCmd, "AT", 2) != 0 )
    return;
    
  pCmd += 2;
  BOOL err = FALSE;
  BOOL ok = TRUE;
  BOOL crlf = TRUE;
  
  while( state == stCommand && !err && *pCmd ) {
      switch( *pCmd++ ) {
	case ' ':
          break;
        case 'A':	// Accept incoming call
          {
            ok = FALSE;
            PWaitAndSignal mutexWait(Mutex);
            timerRing.Stop();
            state = stReqModeAckWait;
            timeout.Start(60000);
            if( t38engine != NULL ) {
              state = stReqModeAckHandle;
              timeout.Stop();
              parent.SignalDataReady();
              crlf = FALSE;
            } else {
              PStringToString request;
              request.SetAt("modemtoken", parent.modemToken());
              request.SetAt("command", "answer");
              request.SetAt("calltoken", CallToken());
            
              callbackEndPoint(request, 1);
              
              PString response = request("response");
              
              if( response == "confirm" ) {
                callDirection = cdIncoming;
                crlf = FALSE;
              } else {
                state = stCommand;
                timeout.Stop();
                err = TRUE;
              }
            }
          }
          break;
        case 'D':	// Dial
          {
            ok = FALSE;
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
                  default:
                    err = TRUE;
                }
              }
            }
            
            if( !err ) {
              PStringToString request;
              request.SetAt("modemtoken", parent.modemToken());
              request.SetAt("command", "dial");
              request.SetAt("number", num);
            
              callbackEndPoint(request, 1);

              PString response = request("response");
              
              if( response == "reject" ) {
                PString diag = request("diag");
                if( diag == "noroute" )
                  resp += "\r\nBUSY";
                else
                  resp += "\r\nNO DIALTONE";
              } else if( response == "confirm" ) {
                CallToken(request("calltoken"));
                crlf = FALSE;
                PWaitAndSignal mutexWait(Mutex);
                state = stReqModeAckWait;
                timeout.Start(60000);
                callDirection = cdOutgoing;
              } else {
                err = TRUE;
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
                  {
                    state = stSendAckWait;
                    if( !t38engine || !t38engine->SendStop(moreFrames, NextSeq()) ) {
                      bresp.Concatenate(PBYTEArray((const BYTE *)"\r\nERROR\r\n", 9));
                      state = stCommand;
                    }
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
            if( t38engine )
              t38engine->ResetModemState();
          }
          bresp.Concatenate(PBYTEArray((const BYTE *)"\r\nOK\r\n", 6));
      }
    }
}

void ModemEngineBody::CheckState(PBYTEArray &bresp)
{
  PString resp;
  
  {
    PWaitAndSignal mutexWait(Mutex);
    if(cmd.IsEmpty() && timerRing.Get() ) {
      resp += "\r\nRING\r\n";
    }
  }

  if( timeout.Get() ) {
    PWaitAndSignal mutexWait(Mutex);
    switch( state ) {
      case stReqModeAckWait:
      case stRecvBegWait:
        resp += "\r\nNO CARRIER\r\n";
        state = stCommand;
        if( t38engine )
          t38engine->ResetModemState();
        break;
    }
  }

  switch( state ) {
    case stResetHandle:
      resp += "\r\nERROR\r\n";
      state = stCommand;
      if( t38engine )
        t38engine->ResetModemState();
      break;
    case stReqModeAckHandle:
      {
        PWaitAndSignal mutexWait(Mutex);
        dataType = T38Engine::dtCed;
        switch( callDirection ) {
          case cdIncoming:
            state = stSend;
            if( t38engine && t38engine->SendStart(dataType, 5000) ) {
              state = stSendAckWait;
              if( !t38engine->SendStop(FALSE, NextSeq()) ) {
                resp += "\r\nERROR\r\n";
                state = stCommand;
              }
            } else {
              resp += "\r\nNO CARRIER\r\n";
              state = stCommand;
            }
            break;
          case cdOutgoing:
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
          
          /*
          if( dataCount == 0 && count != 0 ) {
            // Remote can do too long delay between indication and data
            // so we respond CONNECT on data rather then on indication
            PBYTEArray _bresp((const BYTE *)"\r\nCONNECT\r\n", 11);
            myPTRACE(1, "<-- " << PRTHEX(_bresp));
            bresp.Concatenate(_bresp);
          }
          */
        
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

