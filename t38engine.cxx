/*
 * t38engine.cxx
 *
 * T38FAX Pseudo Modem
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
 * $Log: t38engine.cxx,v $
 * Revision 1.6  2002-01-10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.6  2002/01/10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.5  2002/01/06 03:48:45  craigs
 * Added changes to support efax 0.9
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.4  2002/01/03 21:36:42  craigs
 * Added additional logic to work with efax
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.3  2002/01/02 04:50:34  craigs
 * General formatting cleanups whilst looking for efax problem
 *
 * Revision 1.2  2002/01/01 23:59:52  craigs
 * Lots of additional implementation thanks to Vyacheslav Frolov
 *
 */

#include "t38engine.h"
#include "t38.h"

#define new PNEW

#define T38I(t30_indicator) T38_Type_of_msg_t30_indicator::t30_indicator
#define T38D(msg_data) T38_Type_of_msg_data::msg_data
#define T38F(field_type) T38_Data_Field_subtype_field_type::field_type
#define msPerOut 30

///////////////////////////////////////////////////////////////
enum StateOut {
  stOutIdle,
  
  stOutCedWait,
  stOutSilenceWait,
  stOutIndWait,

  stOutData,
  
  stOutHdlcFcs,
  stOutHdlcFlagsWait,
  
  stOutDataNoSig,

  stOutNoSig,
};
///////////////////////////////////////////////////////////////
enum StateModem {
  stmIdle,
  
  stmOutMoreData,
  stmOutNoMoreData,
  
  stmInWaitData,
  stmInReadyData,
  stmInRecvData,
};

#define isStateModemOut() (stateModem >= stmOutMoreData && stateModem <= stmOutNoMoreData)
#define isStateModemIn() (stateModem >= stmInWaitData && stateModem <= stmInRecvData)
///////////////////////////////////////////////////////////////
class ModStream
{
  public:
    ModStream(const MODPARS &_ModPars);
    ~ModStream();
    
    void PushBuf();
    BOOL DeleteFirstBuf();
    BOOL PopBuf();
    int GetData(void *pBuf, PINDEX count);
    int PutData(const void *pBuf, PINDEX count);
    BOOL SetDiag(int diag);
    BOOL PutEof(int diag = 0);
    void Move(ModStream &from);
  
    DataStream *firstBuf;
    DataStreamQ bufQ;
    DataStream *lastBuf;	// if not NULL then shold be in bufQ or firstBuf
    const MODPARS ModPars;
};

ModStream::ModStream(const MODPARS &_ModPars) : firstBuf(NULL), lastBuf(NULL), ModPars(_ModPars)
{
}

ModStream::~ModStream()
{
  if( firstBuf != NULL ) {
    PTRACE(1, "ModStream::~ModStream firstBuf != NULL, clean");
    delete firstBuf;
  }
  if( bufQ.GetSize() > 0 )
    PTRACE(1, "ModStream::~ModStream bufQ.GetSize()=" << bufQ.GetSize() << ", clean");
}

void ModStream::PushBuf()
{
  lastBuf = new DataStream();
  bufQ.Enqueue(lastBuf);
}

BOOL ModStream::DeleteFirstBuf()
{
  if( firstBuf != NULL ) {
    if( lastBuf == firstBuf )
      lastBuf = NULL;
    delete firstBuf;
    firstBuf = NULL;
    return TRUE;
  }
  return FALSE;
}

BOOL ModStream::PopBuf()
{
  if( DeleteFirstBuf() )
    PTRACE(1, "ModStream::PopBuf DeleteFirstBuf(), clean");
  return( (firstBuf = bufQ.Dequeue()) != NULL );
}

int ModStream::GetData(void *pBuf, PINDEX count)
{
  if( firstBuf == NULL ) {
    myPTRACE(1, "ModStream::GetData firstBuf == NULL");
    return -1;
  }
  return firstBuf->GetData(pBuf, count);
}

int ModStream::PutData(const void *pBuf, PINDEX count)
{
  if( lastBuf == NULL ) {
    myPTRACE(1, "ModStream::PutData lastBuf == NULL");
    return -1;
  }
  return lastBuf->PutData(pBuf, count);
}

BOOL ModStream::SetDiag(int diag)
{
  if( lastBuf == NULL )
    return FALSE;
  lastBuf->SetDiag(lastBuf->GetDiag() | diag);
  return TRUE;
}

BOOL ModStream::PutEof(int diag)
{
  if( lastBuf == NULL )
    return FALSE;
  lastBuf->SetDiag(lastBuf->GetDiag() | diag).PutEof();
  lastBuf = NULL;
  return TRUE;
}

void ModStream::Move(ModStream &from)
{
  if( from.firstBuf != NULL ) {
    bufQ.Enqueue(from.firstBuf);
    from.firstBuf = NULL;
  }

  DataStream *buf;
  while( (buf = from.bufQ.Dequeue()) != NULL ) {
    bufQ.Enqueue(buf);
  }
  lastBuf = from.lastBuf;
  from.lastBuf = NULL;
}
///////////////////////////////////////////////////////////////
static const MODPARS mods[] = {
MODPARS(T38Engine::dtHdlc,   3, T38I(e_v21_preamble),              900, T38D(e_v21),         300 ),
MODPARS( T38Engine::dtRaw,  24, T38I(e_v27_2400_training),         800, T38D(e_v27_2400),   2400 ),
MODPARS( T38Engine::dtRaw,  48, T38I(e_v27_4800_training),         800, T38D(e_v27_4800),   4800 ),
MODPARS( T38Engine::dtRaw,  72, T38I(e_v29_7200_training),         800, T38D(e_v29_7200),   7200 ),
MODPARS( T38Engine::dtRaw,  73, T38I(e_v17_7200_long_training),   1500, T38D(e_v17_7200),   7200 ),
MODPARS( T38Engine::dtRaw,  74, T38I(e_v17_7200_short_training),   400, T38D(e_v17_7200),   7200 ),
MODPARS( T38Engine::dtRaw,  96, T38I(e_v29_9600_training),         800, T38D(e_v29_9600),   9600 ),
MODPARS( T38Engine::dtRaw,  97, T38I(e_v17_9600_long_training),   1500, T38D(e_v17_9600),   9600 ),
MODPARS( T38Engine::dtRaw,  98, T38I(e_v17_9600_short_training),   400, T38D(e_v17_9600),   9600 ),
MODPARS( T38Engine::dtRaw, 121, T38I(e_v17_12000_long_training),  1500, T38D(e_v17_12000), 12000 ),
MODPARS( T38Engine::dtRaw, 122, T38I(e_v17_12000_short_training),  400, T38D(e_v17_12000), 12000 ),
MODPARS( T38Engine::dtRaw, 145, T38I(e_v17_14400_long_training),  1500, T38D(e_v17_14400), 14400 ),
MODPARS( T38Engine::dtRaw, 146, T38I(e_v17_14400_short_training),  400, T38D(e_v17_14400), 14400 ),
};

static const MODPARS invalidMods;

enum GetModParsBy {
  by_val,
  by_ind,
};

static const MODPARS &GetModPars(int key, enum GetModParsBy by = by_val) {
  for( PINDEX i = 0 ; i < sizeof(mods)/sizeof(mods[0]) ; i++ ) {
    switch( by ) {
      case by_val:
        if( mods[i].val == key )
          return mods[i];
        break;
      case by_ind:
        if( mods[i].ind == (unsigned)key )
          return mods[i];
        break;
      default:
        return invalidMods;
    }
  }
  return invalidMods;
}
///////////////////////////////////////////////////////////////
static void t38indicator(T38_IFPPacket &ifp, unsigned type)
{
    ifp.m_type_of_msg.SetTag(T38_Type_of_msg::e_t30_indicator);
    (T38_Type_of_msg_t30_indicator &)ifp.m_type_of_msg = type;
}

static T38_Data_Field_subtype &t38data(T38_IFPPacket &ifp, unsigned type, unsigned field_type)
{
    ifp.m_type_of_msg.SetTag(T38_Type_of_msg::e_data);
    (T38_Type_of_msg_data &)ifp.m_type_of_msg = type;
    
    ifp.IncludeOptionalField(T38_IFPPacket::e_data_field);
    ifp.m_data_field.SetSize(ifp.m_data_field.GetSize()+1);
    T38_Data_Field_subtype &Data_Field = ifp.m_data_field[0];
    Data_Field.m_field_type = field_type;
    return Data_Field;
}

static void t38data(T38_IFPPacket &ifp, unsigned type, unsigned field_type, const PBYTEArray &data)
{
    T38_Data_Field_subtype &Data_Field = t38data(ifp, type, field_type);
    
    if( data.GetSize() > 0 ) {
        Data_Field.IncludeOptionalField(T38_Data_Field_subtype::e_field_data);
        Data_Field.m_field_data = data;
    }
}
///////////////////////////////////////////////////////////////
T38Engine::T38Engine(const PString &_name)
  : OpalT38Protocol(), name(_name)
{
  PTRACE(1, "T38Engine::T38Engine");
  stateModem = stmIdle;
  stateOut = stOutNoSig;
  
  modStreamIn = NULL;
  modStreamInSaved = NULL;
  
  T38Mode = TRUE;
  isCarrierIn = 0;
}

BOOL T38Engine::Originate(H323Transport & transport)
{
  if (!name.IsEmpty()) {
    PString old = PThread::Current()->GetThreadName();
    PThread::Current()->SetThreadName(name + "(tx):%0x");
    PTRACE(2, "myT38Protocol::Originate old ThreadName=" << old);
  }
  return OpalT38Protocol::Originate(transport);
}

BOOL T38Engine::Answer(H323Transport & transport)
{
  if( !name.IsEmpty() ) {
    PString old = PThread::Current()->GetThreadName();
    PThread::Current()->SetThreadName(name + "(rx):%0x");
    PTRACE(2, "myT38Protocol::Answer old ThreadName=" << old);
  }
  return OpalT38Protocol::Answer(transport);
}

T38Engine::~T38Engine()
{
  PTRACE(1, "T38Engine::~T38Engine");

  SignalOutDataReady();
  PWaitAndSignal mutexWaitIn(MutexIn);
  PWaitAndSignal mutexWaitOut(MutexOut);
  PWaitAndSignal mutexWaitModem(MutexModem);
  
  if( modStreamIn != NULL )
    delete modStreamIn;
  if( modStreamInSaved != NULL )
    delete modStreamInSaved;
  if( !modemCallback.IsNULL() )
    myPTRACE(1, "T38Engine::~T38Engine !modemCallback.IsNULL()");
}

BOOL T38Engine::Attach(const PNotifier &callback)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PTRACE(1, "T38Engine::Attach");
  PWaitAndSignal mutexWait(Mutex);
  if( !modemCallback.IsNULL() ) {
    myPTRACE(1, "T38Engine::Attach !modemCallback.IsNULL()");
    return FALSE;
  }
  modemCallback = callback;
  ResetModemState();
  return TRUE;
}

void T38Engine::Detach(const PNotifier &callback)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PTRACE(1, "T38Engine::Detach");
  PWaitAndSignal mutexWait(Mutex);
  if( modemCallback == callback ) {
    modemCallback = NULL;
    ResetModemState();
    SetT38Mode(FALSE);
  } else {
    myPTRACE(1, "T38Engine::Detach modemCallback != callback");
  }
}
///////////////////////////////////////////////////////////////
//
void T38Engine::SetT38Mode(BOOL mode)
{
  PWaitAndSignal mutexWait(Mutex);
  T38Mode = mode;
  myPTRACE(1, "T38Engine::SetT38Mode T38Mode=" << (T38Mode ? "TRUE" : "FALSE"));
  SignalOutDataReady();
  if (!T38Mode && !modemCallback.IsNULL() )
    modemCallback(*this, cbpReset);
}

void T38Engine::ResetModemState() {
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if( modStreamIn && modStreamIn->DeleteFirstBuf() )
    PTRACE(1, "T38Engine::ResetModemState modStreamIn->DeleteFirstBuf(), clean");

  bufOut.PutEof();
  if( stateModem != stmIdle ) {
    myPTRACE(1, "T38Engine::ResetModemState stateModem(" << stateModem << ") != stmIdle, reset");
    stateModem = stmIdle;
  }
  callbackParamIn = -1;
  callbackParamOut = -1;
}
///////////////////////////////////////////////////////////////
BOOL T38Engine::SendStart(int _dataType, int param) {
  PWaitAndSignal mutexWaitModem(MutexModem);
  if (!IsT38Mode())
    return FALSE;

  if (stateModem != stmIdle)  {
    myPTRACE(1, "T38Engine::SendStart stateModem(" << stateModem << ") != stmIdle");
    return FALSE;
  }

  PWaitAndSignal mutexWait(Mutex);
  
  if (modStreamIn != NULL) {
    delete modStreamIn;
    modStreamIn = NULL;
  }

  if (modStreamInSaved != NULL)  {
    delete modStreamInSaved;
    modStreamInSaved = NULL;
  }
  
  ModParsOut = invalidMods;

  switch( _dataType ) {
    case dtCed:
      ModParsOut.dataType = _dataType;
      ModParsOut.ind = T38I(e_ced);
      ModParsOut.lenInd = param;
      break;
    case dtSilence:
      ModParsOut.dataType = _dataType;
      ModParsOut.ind = T38I(e_no_signal);
      ModParsOut.lenInd = param;
      break;
    case dtHdlc:
    case dtRaw:
      ModParsOut = GetModPars(param);
      if( !ModParsOut.IsModValid() || ModParsOut.dataType != _dataType ) 
        return FALSE;
      break;
    default:
      return FALSE;
  }
  bufOut.Clean();		// reset eof
  stateModem = stmOutMoreData;
  SignalOutDataReady();
  return TRUE;
}

int T38Engine::Send(const void *pBuf, PINDEX count) {
  PWaitAndSignal mutexWaitModem(MutexModem);
  if (!IsT38Mode())
    return -1;

  if (stateModem != stmOutMoreData) {
    myPTRACE(1, "T38Engine::Send stateModem(" << stateModem << ") != stmOutMoreData");
    return -1;
  }

  PWaitAndSignal mutexWait(Mutex);
  int res = bufOut.PutData(pBuf, count);
  if (res < 0)
    myPTRACE(1, "T38Engine::Send res(" << res << ") < 0");

  SignalOutDataReady();
  return res;
}

BOOL T38Engine::SendStop(BOOL moreFrames, int _callbackParam) {
  PWaitAndSignal mutexWaitModem(MutexModem);
  if(!IsT38Mode())
    return FALSE;

  if (stateModem != stmOutMoreData ) {
    myPTRACE(1, "T38Engine::SendStop stateModem(" << stateModem << ") != stmOutMoreData");
    return FALSE;
  }

  PWaitAndSignal mutexWait(Mutex);
  bufOut.PutEof();
  stateModem = stmOutNoMoreData;
  moreFramesOut = moreFrames;
  callbackParamOut = _callbackParam;
  SignalOutDataReady();
  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL T38Engine::RecvWait(int _dataType, int param, int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  if (!IsT38Mode())
    return FALSE;

  if( stateModem != stmIdle ) {
    myPTRACE(1, "T38Engine::RecvWait stateModem(" << stateModem << ") != stmIdle");
    return FALSE;
  }
  
  MODPARS ModParsIn;

  PWaitAndSignal mutexWait(Mutex);
  switch( _dataType ) {
    case dtHdlc:
    case dtRaw:
      ModParsIn = GetModPars(param);
      if( !ModParsIn.IsModValid() || ModParsIn.dataType != _dataType )
        return FALSE;
      break;
    default:
      return FALSE;
  }

  callbackParamIn = _callbackParam;

  if( modStreamIn != NULL ) {
    if( modStreamIn->DeleteFirstBuf() )
      PTRACE(1, "T38Engine::RecvWait modStreamIn->DeleteFirstBuf(), clean");
  
    if( modStreamIn->bufQ.GetSize() > 0 ) {
      PTRACE(1, "T38Engine::RecvWait modStreamIn->bufQ.GetSize()=" << modStreamIn->bufQ.GetSize());
      if( ModParsIn.val == modStreamIn->ModPars.val ) {
        PTRACE(1, "T38Engine::RecvWait ModParsIn.val == modStreamIn->ModPars.val(" <<  modStreamIn->ModPars.val << ")");
        stateModem = stmInReadyData;
        if( !modemCallback.IsNULL() )
          modemCallback(*this, callbackParamIn);
        return TRUE;
      }
      delete modStreamIn;
      modStreamIn = NULL;
    }
  }

  modStreamIn = new ModStream(ModParsIn);
  
  if( modStreamInSaved != NULL ) {
    if( modStreamIn->ModPars.val == modStreamInSaved->ModPars.val ) {
      myPTRACE(2, "T38Engine::RecvWait modStreamIn->ModPars.val(" <<  modStreamIn->ModPars.val << ") == modStreamInSaved->ModPars.val");
      modStreamIn->Move(*modStreamInSaved);
      delete modStreamInSaved;
      modStreamInSaved = NULL;
    } else {
      myPTRACE(2, "T38Engine::RecvWait modStreamIn->ModPars.val(" <<  modStreamIn->ModPars.val << ") != modStreamInSaved->ModPars.val(" << modStreamInSaved->ModPars.val << ")");
      modStreamIn->PushBuf();
      modStreamIn->PutEof(diagDiffSig);
    }
    stateModem = stmInReadyData;
    if( !modemCallback.IsNULL() )
      modemCallback(*this, callbackParamIn);
    return TRUE;
  }

  stateModem = stmInWaitData;
  return TRUE;
}

BOOL T38Engine::RecvStart(int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  if (!IsT38Mode())
    return FALSE;

  if (stateModem != stmInReadyData ) {
    myPTRACE(1, "T38Engine::RecvStart stateModem(" << stateModem << ") != stmInReadyData");
    return FALSE;
  }
  PWaitAndSignal mutexWait(Mutex);
  callbackParamIn = _callbackParam;
  
  if( modStreamIn != NULL ) {
    if( modStreamIn->PopBuf() ) {
      stateModem = stmInRecvData;
      return TRUE;
    }
    myPTRACE(1, "T38Engine::RecvStart can't receive firstBuf");
    delete modStreamIn;
    modStreamIn = NULL;
  } else {
    myPTRACE(1, "T38Engine::RecvStart modStreamIn == NULL");
  }

  stateModem = stmIdle;
  return FALSE;
}

int T38Engine::Recv(void *pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  if (!IsT38Mode())
    return -1;

  if( stateModem != stmInRecvData ) {
    myPTRACE(1, "T38Engine::Recv stateModem(" << stateModem << ") != stmInRecvData");
    return -1;
  }
  PWaitAndSignal mutexWait(Mutex);
  if( modStreamIn == NULL ) {
    myPTRACE(1, "T38Engine::Recv modStreamIn == NULL");
    return -1;
  }
  return modStreamIn->GetData(pBuf, count);
}

int T38Engine::RecvDiag()
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);
  if( modStreamIn == NULL ) {
    myPTRACE(1, "T38Engine::RecvDiag modStreamIn == NULL");
    return diagError;
  }
  if( modStreamIn->firstBuf == NULL ) {
    myPTRACE(1, "T38Engine::RecvDiag modStreamIn->firstBuf == NULL");
    return diagError;
  }
  return modStreamIn->firstBuf->GetDiag();
}

BOOL T38Engine::RecvStop()
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  if(!IsT38Mode())
    return FALSE;

  if(!isStateModemIn()) {
    myPTRACE(1, "T38Engine::RecvStop stateModem(" << stateModem << ") != stmIn");
    return FALSE;
  }
  PWaitAndSignal mutexWait(Mutex);

  if( modStreamIn )
    modStreamIn->DeleteFirstBuf();
  if( isStateModemIn() )
    stateModem = stmIdle;
  return FALSE;
}
///////////////////////////////////////////////////////////////
BOOL T38Engine::PreparePacket(T38_IFPPacket & ifp)
{
  PWaitAndSignal mutexWaitOut(MutexOut);

  if(!IsT38Mode())
    return FALSE;
  
  //myPTRACE(1, "T38Engine::PreparePacket begin stM=" << stateModem << " stO=" << stateOut);
  
  ifp = T38_IFPPacket();

  for(;;) {
    BOOL redo = FALSE;
    PTimeInterval outDelay;
    
    switch( stateOut ) {
      case stOutIdle:			outDelay = msPerOut; break;
      
      case stOutCedWait:		outDelay = ModParsOut.lenInd; break;
      case stOutSilenceWait:		outDelay = ModParsOut.lenInd; break;
      case stOutIndWait:		outDelay = ModParsOut.lenInd; break;

      case stOutData:
        outDelay = PTimeInterval((countOut*8*1000)/ModParsOut.br + msPerOut) - (PTime() - timeBeginOut);
        break;
      
      case stOutHdlcFcs:		outDelay = msPerOut; break;
      case stOutHdlcFlagsWait:		outDelay = msPerOut*3; break;
      
      case stOutDataNoSig:		outDelay = msPerOut; break;
      
      case stOutNoSig:			outDelay = msPerOut; break;
      default:				outDelay = 0;
    }
      
    //myPTRACE(1, "T38Engine::PreparePacket outDelay=" << outDelay);
    
    if( outDelay > 0 )
    #ifdef P_LINUX
      usleep(outDelay.GetMilliSeconds() * 1000);
    #else
      PThread::Sleep(outDelay);
    #endif
      
    if (!IsT38Mode())
      return FALSE;

    for(;;) {
      BOOL waitData = FALSE;
      {
        PWaitAndSignal mutexWait(Mutex);
        if( isStateModemOut() || stateOut != stOutIdle ) {
          switch( stateOut ) {
            case stOutIdle:
              if( isCarrierIn ) {
                myPTRACE(1, "T38Engine::PreparePacket waiting isCarrierIn(" << isCarrierIn << ") == 0");
                if( --isCarrierIn == 0 ) {	// to prevent dead lock
                  myPTRACE(1, "T38Engine::PreparePacket isCarrierIn expired");
                }
                redo = TRUE;
                break;
              }
              switch( ModParsOut.dataType ) {
                case dtHdlc:
                case dtRaw:
                  t38indicator(ifp, ModParsOut.ind);
                  stateOut = stOutIndWait;
                  break;
                case dtCed:
                  t38indicator(ifp, ModParsOut.ind);
                  stateOut = stOutCedWait;
                  break;
                case dtSilence:
                  stateOut = stOutSilenceWait;
                  redo = TRUE;
                  break;
                default:
                  myPTRACE(1, "T38Engine::PreparePacket bad dataType=" << ModParsOut.dataType);
                  return FALSE;
              }
              break;
            ////////////////////////////////////////////////////
            case stOutCedWait:
              stateOut = stOutNoSig;
              stateModem = stmIdle;
              if( !modemCallback.IsNULL() )
                modemCallback(*this, callbackParamOut);
              redo = TRUE;
              break;
            ////////////////////////////////////////////////////
            case stOutSilenceWait:
              stateOut = stOutIdle;
              stateModem = stmIdle;
              if( !modemCallback.IsNULL() )
                modemCallback(*this, callbackParamOut);
              redo = TRUE;
              break;
            ////////////////////////////////////////////////////
            case stOutIndWait:
              stateOut = stOutData;
              lastDteCharOut = -1;
              countOut = 0;
              timeBeginOut = PTime();
              redo = TRUE;
              break;
            ////////////////////////////////////////////////////
            case stOutData:
              {
                BYTE b[(msPerOut * 14400)/(8*1000)];
                PINDEX len = (msPerOut * ModParsOut.br)/(8*1000);
                if( len > sizeof(b) )
                  len = sizeof(b);
                int count = bufOut.GetData(b, len);
                
                switch( count ) {
                  case -1:
                    switch( ModParsOut.dataType ) {
                      case dtHdlc:
                        stateOut = stOutHdlcFcs;
                        break;
                      case dtRaw:
                        stateOut = stOutDataNoSig;
                        break;
                      default:
                        myPTRACE(1, "T38Engine::PreparePacket stOutData bad dataType=" << ModParsOut.dataType);
                        return FALSE;
                    }
                    redo = TRUE;
                    break;
                  case 0:
                    if( lastDteCharOut != -1 ) {
                      if( ModParsOut.dataType == dtHdlc || lastDteCharOut != 0 ) {
                        if( !modemCallback.IsNULL() )
                          modemCallback(*this, cbpOutBufEmpty);
                      }
                    }
                    waitData = TRUE;
                    break;
                  default:
                    switch( ModParsOut.dataType ) {
                      case dtHdlc:
                        t38data(ifp, ModParsOut.msgType, T38F(e_hdlc_data), PBYTEArray(b, count));
                        break;
                      case dtRaw:
                        t38data(ifp, ModParsOut.msgType, T38F(e_t4_non_ecm_data), PBYTEArray(b, count));
                        break;
                      default:
                        myPTRACE(1, "T38Engine::PreparePacket stOutData bad dataType=" << ModParsOut.dataType);
                        return FALSE;
                    }
                    lastDteCharOut = b[count - 1] & 0xFF;
                    countOut += count;
                }
              }
              break;
            ////////////////////////////////////////////////////
            case stOutHdlcFcs:
              if( stateModem != stmOutNoMoreData ) {
                myPTRACE(1, "T38Engine::PreparePacket stOutHdlcFcs stateModem(" << stateModem << ") != stmOutNoMoreData");
                return FALSE;
              }
              t38data(ifp, ModParsOut.msgType, T38F(e_hdlc_fcs_OK));
              if( moreFramesOut ) {
                stateOut = stOutHdlcFlagsWait;
                bufOut.Clean();		// reset eof
                stateModem = stmOutMoreData;
                if( !modemCallback.IsNULL() )
                  modemCallback(*this, callbackParamOut);
              } else {
                stateOut = stOutDataNoSig;
              }
              break;
            case stOutHdlcFlagsWait:
              stateOut = stOutData;
              lastDteCharOut = -1;
              countOut = 0;
              timeBeginOut = PTime();
              redo = TRUE;
              break;
            ////////////////////////////////////////////////////
            case stOutDataNoSig:
              if( stateModem != stmOutNoMoreData ) {
                myPTRACE(1, "T38Engine::PreparePacket stOutDataNoSig stateModem(" << stateModem << ") != stmOutNoMoreData");
                return FALSE;
              }
              switch( ModParsOut.dataType ) {
                case dtHdlc:
                  t38data(ifp, ModParsOut.msgType, T38F(e_hdlc_sig_end));
                  break;
                case dtRaw:
                  t38data(ifp, ModParsOut.msgType, T38F(e_t4_non_ecm_sig_end));
                  break;
                default:
                  myPTRACE(1, "T38Engine::PreparePacket stOutDataNoSig bad dataType=" << ModParsOut.dataType);
                  return FALSE;
              }
              stateOut = stOutNoSig;
              stateModem = stmIdle;
              if( !modemCallback.IsNULL() )
                modemCallback(*this, callbackParamOut);
              break;
            ////////////////////////////////////////////////////
            case stOutNoSig:
              t38indicator(ifp, T38I(e_no_signal));
              stateOut = stOutIdle;
              break;
            default:
              myPTRACE(1, "T38Engine::PreparePacket bad stateOut=" << stateOut);
              return FALSE;
          }
        } else {
          waitData = TRUE;
        }
      }
      if( !waitData ) 
        break;
      WaitOutDataReady();
      if( !IsT38Mode() )
        return FALSE;

      {
        PWaitAndSignal mutexWait(Mutex);
        if( stateOut == stOutData ) {
          myPTRACE(1, "T38Engine::PreparePacket DTE's data delay, reset " << countOut);
          countOut = 0;
          timeBeginOut = PTime() - PTimeInterval(msPerOut);	// no delay
        }
      }
    }
    if( !redo ) break;
  }
  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL T38Engine::HandlePacketLost(unsigned nLost)
{
  PWaitAndSignal mutexWaitIn(MutexIn);

  if (!IsT38Mode())
    return FALSE;

  myPTRACE(1, "T38Engine::HandlePacketLost " << nLost);
  PWaitAndSignal mutexWait(Mutex);

  ModStream *modStream = modStreamIn;
    
  if( modStream == NULL || modStream->lastBuf == NULL ) {
    modStream = modStreamInSaved;
  }
  if( !(modStream == NULL || modStream->lastBuf == NULL) ) {
    if( modStream->ModPars.msgType == T38D(e_v21) ) {
      modStream->SetDiag(diagBadFcs);
    }
  }
  return TRUE;
}
///////////////////////////////////////////////////////////////
BOOL T38Engine::HandlePacket(const T38_IFPPacket & ifp)
{
  PWaitAndSignal mutexWaitIn(MutexIn);
  if (!IsT38Mode())
    return FALSE;
  
  PWaitAndSignal mutexWait(Mutex);
  
  switch( ifp.m_type_of_msg.GetTag() ) {
    case T38_Type_of_msg::e_t30_indicator:
      if( modStreamIn != NULL && modStreamIn->PutEof(diagOutOfOrder) )
        myPTRACE(1, "T38Engine::HandlePacket indicator && modStreamIn->lastBuf != NULL");

      if( modStreamInSaved != NULL ) {
        myPTRACE(1, "T38Engine::HandlePacket indicator && modStreamInSaved != NULL, clean");
        delete modStreamInSaved;
        modStreamInSaved = NULL;
      }
      
      switch( (const T38_Type_of_msg_t30_indicator &)ifp.m_type_of_msg ) {
        case T38I(e_no_signal):
        case T38I(e_cng):
        case T38I(e_ced):
          isCarrierIn = 0;
          break;
        case T38I(e_v21_preamble):
        case T38I(e_v27_2400_training):
        case T38I(e_v27_4800_training):
        case T38I(e_v29_7200_training):
        case T38I(e_v29_9600_training):
        case T38I(e_v17_7200_short_training):
        case T38I(e_v17_7200_long_training):
        case T38I(e_v17_9600_short_training):
        case T38I(e_v17_9600_long_training):
        case T38I(e_v17_12000_short_training):
        case T38I(e_v17_12000_long_training):
        case T38I(e_v17_14400_short_training):
        case T38I(e_v17_14400_long_training):
          isCarrierIn = 1000 / msPerOut;	// 1 sec
          modStreamInSaved = new ModStream(GetModPars((const T38_Type_of_msg_t30_indicator &)ifp.m_type_of_msg, by_ind));
          modStreamInSaved->PushBuf();
            
          if( stateModem == stmInWaitData ) {
            if( modStreamIn != NULL ) {
              if( modStreamIn->ModPars.val == modStreamInSaved->ModPars.val ) {
                modStreamIn->Move(*modStreamInSaved);
                delete modStreamInSaved;
                modStreamInSaved = NULL;
              } else {
                myPTRACE(2, "T38Engine::HandlePacket modStreamIn->ModPars.val(" <<  modStreamIn->ModPars.val << ") != modStreamInSaved->ModPars.val(" << modStreamInSaved->ModPars.val << ")");
                modStreamIn->PushBuf();
                modStreamIn->PutEof(diagDiffSig);
              }
            } else {
              myPTRACE(1, "T38Engine::HandlePacket modStreamIn == NULL");
            }
            stateModem = stmInReadyData;
            if( !modemCallback.IsNULL() )
              modemCallback(*this, callbackParamIn);
          }
          break;
        default:
          myPTRACE(1, "T38Engine::HandlePacket type_of_msg bad !!! " << setprecision(2) << ifp);
      }
      break;
    case T38_Type_of_msg::e_data:
      {
        unsigned type_of_msg = (const T38_Type_of_msg_data &)ifp.m_type_of_msg;
        ModStream *modStream = modStreamIn;
        
        if( modStream == NULL || modStream->lastBuf == NULL ) {
          modStream = modStreamInSaved;
        }
        if( modStream == NULL || modStream->lastBuf == NULL ) {
          PTRACE(1, "T38Engine::HandlePacket lastBuf == NULL");
          break;
        }
        if( modStream->ModPars.msgType != type_of_msg ) {
          myPTRACE(1, "T38Engine::HandlePacket modStream->ModPars.msgType(" << modStream->ModPars.msgType << ") != type_of_msg(" << type_of_msg << ")");
          modStream->PutEof(diagOutOfOrder);
          if( stateModem == stmInRecvData )
            if( !modemCallback.IsNULL() )
              modemCallback(*this, callbackParamIn);
          break;
        }

        switch( type_of_msg ) {
          case T38D(e_v21):
          case T38D(e_v27_2400):
          case T38D(e_v27_4800):
          case T38D(e_v29_7200):
          case T38D(e_v29_9600):
          case T38D(e_v17_7200):
          case T38D(e_v17_9600):
          case T38D(e_v17_12000):
          case T38D(e_v17_14400):
            if( ifp.HasOptionalField(T38_IFPPacket::e_data_field) ) {
              PINDEX count = ifp.m_data_field.GetSize();
              for( PINDEX i = 0 ; i < count ; i++ ) {
                if( modStream == NULL ) {
                  PTRACE(1, "T38Engine::HandlePacket modStream == NULL");
                  break;
                }
                const T38_Data_Field_subtype &Data_Field = ifp.m_data_field[i];
                
                switch( Data_Field.m_field_type ) {	// Handle data
                  case T38F(e_hdlc_data):
                  case T38F(e_t4_non_ecm_data):
                  case T38F(e_hdlc_sig_end):
                  case T38F(e_hdlc_fcs_OK):
                  case T38F(e_hdlc_fcs_BAD):
                  case T38F(e_hdlc_fcs_OK_sig_end):
                  case T38F(e_hdlc_fcs_BAD_sig_end):
                  case T38F(e_t4_non_ecm_sig_end):
                    if( Data_Field.HasOptionalField(T38_Data_Field_subtype::e_field_data) )
                      modStream->PutData(Data_Field.m_field_data, Data_Field.m_field_data.GetSize());
                    break;
                  default:
                    myPTRACE(1, "T38Engine::HandlePacket field_type bad !!! " << setprecision(2) << ifp);
                }
                switch( Data_Field.m_field_type ) {	// Handle fcs_BAD
                  case T38F(e_hdlc_fcs_BAD):
                  case T38F(e_hdlc_fcs_BAD_sig_end):
                    modStream->SetDiag(diagBadFcs);
                    myPTRACE(1, "T38Engine::HandlePacket bad FCS");
                    break;
                }
                switch( Data_Field.m_field_type ) {	// Handle fcs
                  case T38F(e_hdlc_fcs_OK):
                  case T38F(e_hdlc_fcs_BAD):
                  case T38F(e_hdlc_fcs_OK_sig_end):
                  case T38F(e_hdlc_fcs_BAD_sig_end):
                    modStream->PutEof();
                    modStream->PushBuf();
                    break;
                }
                switch( Data_Field.m_field_type ) {	// Handle sig_end
                  case T38F(e_hdlc_fcs_OK_sig_end):
                  case T38F(e_hdlc_fcs_BAD_sig_end):
                  case T38F(e_hdlc_sig_end):
                  case T38F(e_t4_non_ecm_sig_end):
                    modStream->PutEof(diagNoCarrier);
                    modStream = NULL;
                    //isCarrierIn = 0;
                    break;
                }
              }
            }
            break;
          default:
            myPTRACE(1, "T38Engine::HandlePacket type_of_msg bad !!! " << setprecision(2) << ifp);
        }
        if( stateModem == stmInRecvData )
          if( !modemCallback.IsNULL() )
            modemCallback(*this, callbackParamIn);
        break;
      }
    default:
      myPTRACE(1, "T38Engine::HandlePacket Tag bad !!! " << setprecision(2) << ifp);
  }
  return TRUE;
}
///////////////////////////////////////////////////////////////

