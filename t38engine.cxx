/*
 * t38engine.cxx
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
 * $Log: t38engine.cxx,v $
 * Revision 1.14  2002-05-22 12:01:45  vfrolov
 * Implemented redundancy error protection scheme
 *
 * Revision 1.14  2002/05/22 12:01:45  vfrolov
 * Implemented redundancy error protection scheme
 *
 * Revision 1.13  2002/05/15 16:05:17  vfrolov
 * Changed algorithm of handling isCarrierIn
 * Removed delay after sending dtSilence
 *
 * Revision 1.12  2002/05/08 16:33:16  vfrolov
 * Adjusted post training delays
 *
 * Revision 1.11  2002/05/07 11:06:12  vfrolov
 * Discarded const from ModemCallbackWithUnlock()
 *
 * Revision 1.10  2002/05/07 10:15:38  vfrolov
 * Fixed dead lock on modemCallback
 *
 * Revision 1.9  2002/04/19 13:58:59  vfrolov
 * Added SendOnIdle()
 *
 * Revision 1.8  2002/03/01 09:02:04  vfrolov
 * Added Copyright header
 * Added name name to trace messages
 *
 * Revision 1.7  2002/02/11 16:46:18  vfrolov
 * Discarded transport arg from Originate() and Answer()
 * Thanks to Christopher Curtis
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
#include "transports.h"

#define new PNEW

#define T38I(t30_indicator) T38_Type_of_msg_t30_indicator::t30_indicator
#define T38D(msg_data) T38_Type_of_msg_data::msg_data
#define T38F(field_type) T38_Data_Field_subtype_field_type::field_type
#define msPerOut 30
#define msTimeout ((msPerOut) * 3)

#ifdef P_LINUX
  #define mySleep(ms) usleep((ms) * 1000L)
#else
  #define mySleep(ms) PThread::Sleep(ms)
#endif
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
MODPARS( T38Engine::dtHdlc,  3, T38I(e_v21_preamble),              900, T38D(e_v21),         300 ),
MODPARS( T38Engine::dtRaw,  24, T38I(e_v27_2400_training),        1100, T38D(e_v27_2400),   2400 ),
MODPARS( T38Engine::dtRaw,  48, T38I(e_v27_4800_training),         900, T38D(e_v27_4800),   4800 ),
MODPARS( T38Engine::dtRaw,  72, T38I(e_v29_7200_training),         300, T38D(e_v29_7200),   7200 ),
MODPARS( T38Engine::dtRaw,  73, T38I(e_v17_7200_long_training),   1500, T38D(e_v17_7200),   7200 ),
MODPARS( T38Engine::dtRaw,  74, T38I(e_v17_7200_short_training),   300, T38D(e_v17_7200),   7200 ),
MODPARS( T38Engine::dtRaw,  96, T38I(e_v29_9600_training),         300, T38D(e_v29_9600),   9600 ),
MODPARS( T38Engine::dtRaw,  97, T38I(e_v17_9600_long_training),   1500, T38D(e_v17_9600),   9600 ),
MODPARS( T38Engine::dtRaw,  98, T38I(e_v17_9600_short_training),   300, T38D(e_v17_9600),   9600 ),
MODPARS( T38Engine::dtRaw, 121, T38I(e_v17_12000_long_training),  1500, T38D(e_v17_12000), 12000 ),
MODPARS( T38Engine::dtRaw, 122, T38I(e_v17_12000_short_training),  300, T38D(e_v17_12000), 12000 ),
MODPARS( T38Engine::dtRaw, 145, T38I(e_v17_14400_long_training),  1500, T38D(e_v17_14400), 14400 ),
MODPARS( T38Engine::dtRaw, 146, T38I(e_v17_14400_short_training),  300, T38D(e_v17_14400), 14400 ),
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
  PTRACE(2, name << " T38Engine::T38Engine");
  stateModem = stmIdle;
  stateOut = stOutNoSig;
  onIdleOut = dtNone;
  delayRestOut = 0;
  
  modStreamIn = NULL;
  modStreamInSaved = NULL;
  
  in_redundancy = 0;
  ls_redundancy = 0;
  hs_redundancy = 0;
  
  T38Mode = TRUE;
  isCarrierIn = 0;
}

T38Engine::~T38Engine()
{
  PTRACE(1, name << " T38Engine::~T38Engine ");

  SignalOutDataReady();
  PWaitAndSignal mutexWaitIn(MutexIn);
  PWaitAndSignal mutexWaitOut(MutexOut);
  PWaitAndSignal mutexWaitModem(MutexModem);
  
  if( modStreamIn != NULL )
    delete modStreamIn;
  if( modStreamInSaved != NULL )
    delete modStreamInSaved;
  if( !modemCallback.IsNULL() )
    myPTRACE(1, name << " T38Engine::~T38Engine !modemCallback.IsNULL()");
}

BOOL T38Engine::Attach(const PNotifier &callback)
{
  PTRACE(1, name << " T38Engine::Attach");
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);
  if( !modemCallback.IsNULL() ) {
    myPTRACE(1, name << " T38Engine::Attach !modemCallback.IsNULL()");
    return FALSE;
  }
  modemCallback = callback;
  ResetModemState();
  return TRUE;
}

void T38Engine::Detach(const PNotifier &callback)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PTRACE(1, name << " T38Engine::Detach");
  PWaitAndSignal mutexWait(Mutex);
  if( modemCallback == callback ) {
    modemCallback = NULL;
    ResetModemState();
    SetT38Mode(FALSE);
  } else {
    myPTRACE(1, name << " T38Engine::Detach modemCallback != callback");
  }
}
///////////////////////////////////////////////////////////////
void T38Engine::SetRedundancy(int indication, int low_speed, int high_speed) {
  if (indication >= 0)
    in_redundancy = indication;
  if (low_speed >= 0)
    ls_redundancy = low_speed;
  if (high_speed >= 0)
    hs_redundancy = high_speed;

  myPTRACE(3, name << " T38Engine::SetRedundancy indication=" << in_redundancy
                                            << " low_speed=" << ls_redundancy
                                            << " high_speed=" << hs_redundancy);
}

BOOL T38Engine::Originate()
{
  if (!name.IsEmpty()) {
    PString old = PThread::Current()->GetThreadName();
    PThread::Current()->SetThreadName(name + "(tx):%0x");
    PTRACE(2, name << " T38Engine::Originate old ThreadName=" << old);
  }
  
  PTRACE(3, "T38\tOriginate, transport=" << *transport);

  long seq = -1;
  int maxRedundancy = 0;
  int repeated = 0;

  T38_UDPTLPacket udptl;
  udptl.m_error_recovery.SetTag(T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets);

  for (;;) {
    T38_IFPPacket ifp;

    int res = PreparePacket(ifp, maxRedundancy > 0);

    if (res > 0) {
      T38_UDPTLPacket_error_recovery &recovery = udptl.m_error_recovery;
      if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
        int nRedundancy = secondary.GetSize() + 1;
        if (nRedundancy > maxRedundancy)
          nRedundancy = maxRedundancy;
        if (nRedundancy < 0)
          nRedundancy = 0;
        secondary.SetSize(nRedundancy);
    
        if (nRedundancy > 0) {
          for (int i = nRedundancy - 1 ; i > 0 ; i--) {
            secondary[i] = secondary[i - 1];
          }
          secondary[0].SetValue(udptl.m_primary_ifp_packet.GetValue());
        }
      }
      else
        if (maxRedundancy > 0) {
          PTRACE(4, "T38\tNot implemented yet " << recovery.GetTagName());
        }

      udptl.m_seq_number = ++seq & 0xFFFF;
      udptl.m_primary_ifp_packet.EncodeSubType(ifp);

      /*
       * Calculate maxRedundancy for current ifp packet
       */
      maxRedundancy = hs_redundancy;
  
      switch( ifp.m_type_of_msg.GetTag() ) {
        case T38_Type_of_msg::e_t30_indicator:
          maxRedundancy = in_redundancy;
          break;
        case T38_Type_of_msg::e_data:
          switch( (const T38_Type_of_msg_data &)ifp.m_type_of_msg ) {
            case T38D(e_v21):
              maxRedundancy = ls_redundancy;
              break;
          }
          break;
      }

#if 0
      // recovery test
      if (seq % 2)
        continue;
#endif
    }
    else if (res < 0) {
      if (maxRedundancy <= 0)
        continue;
      maxRedundancy--;
#if 1
      /*
       * Optimise repeated packet each time
       */
      T38_UDPTLPacket_error_recovery &recovery = udptl.m_error_recovery;
      if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
        int nRedundancy = secondary.GetSize();
        if (nRedundancy > maxRedundancy)
          nRedundancy = maxRedundancy;
        if (nRedundancy < 0)
          nRedundancy = 0;
        secondary.SetSize(nRedundancy);
      }
      else {
        PTRACE(4, "T38\tNot implemented yet " << recovery.GetTagName());
        continue;
      }
#endif
      repeated++;
    }
    else
      break;

    PPER_Stream rawData;
    udptl.Encode(rawData);

#if PTRACING
    if (PTrace::CanTrace(4)) {
      PTRACE(4, "T38\tSending PDU:\n  ifp = "
             << setprecision(2) << ifp << "\n  UDPTL = "
             << setprecision(2) << udptl << "\n  "
             << setprecision(2) << rawData);
    }
    else {
      PTRACE(3, "T38\tSending PDU:"
                " seq=" << seq <<
                " type=" << ifp.m_type_of_msg.GetTagName());
    }
#endif

    if (!transport->WritePDU(rawData)) {
      PTRACE(1, "T38\tOriginate - WritePDU ERROR: " << transport->GetErrorText());
      break;
    }
  }

  PTRACE(2, "T38\tSend statistics: sequence=" << seq
      << " repeated=" << repeated);

  return FALSE;
}

BOOL T38Engine::Answer()
{
  if( !name.IsEmpty() ) {
    PString old = PThread::Current()->GetThreadName();
    PThread::Current()->SetThreadName(name + "(rx):%0x");
    PTRACE(2, name << " T38Engine::Answer old ThreadName=" << old);
  }
  
  PTRACE(3, "T38\tAnswer, transport=" << *transport);

  /* HACK HACK HACK -- need to figure out how to get the remote address
   * properly here */
  transport->SetPromiscuous(TRUE);

  int consecutiveBadPackets = 0;
  long expectedSequenceNumber = 0;
  int totalrecovered = 0;
  int totallost = 0;
  int repeated = 0;

  for (;;) {
    PPER_Stream rawData;
    if (!transport->ReadPDU(rawData)) {
      PTRACE(1, "T38\tError reading PDU: " << transport->GetErrorText(PChannel::LastReadError));
      break;
    }

    /* when we get the first packet, set the RemoteAddress and then turn off
     * promiscuous listening */
    if (expectedSequenceNumber == 0) {
      PTRACE(3, "T38\tReceived first packet, remote=" << transport->GetRemoteAddress());
      transport->SetPromiscuous(FALSE);
    }

    T38_UDPTLPacket udptl;
    if (udptl.Decode(rawData))
      consecutiveBadPackets = 0;
    else {
      consecutiveBadPackets++;
      PTRACE(2, "T38\tRaw data decode failure:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl);
      if (consecutiveBadPackets > 3) {
        PTRACE(1, "T38\tRaw data decode failed multiple times, aborting!");
        break;
      }
      continue;
    }

    long receivedSequenceNumber = (udptl.m_seq_number & 0xFFFF) + (expectedSequenceNumber & ~0xFFFFL);
    long lost = receivedSequenceNumber - expectedSequenceNumber;
    
    if (lost < -0x10000L/2) {
      lost += 0x10000L;
      receivedSequenceNumber += 0x10000L;
    }
    else if (lost > 0x10000L/2) {
      lost -= 0x10000L;
      receivedSequenceNumber -= 0x10000L;
    }
    
#if PTRACING
    if (PTrace::CanTrace(4)) {
      PTRACE(4, "T38\tReceived PDU:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl);
    }
#endif

    T38_IFPPacket ifp;

    if (lost < 0) {
      PTRACE(3, "T38\tRepeated packet");
      repeated++;
      continue;
    }
    else if(lost > 0) {
      const T38_UDPTLPacket_error_recovery &recovery = udptl.m_error_recovery;
      if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        const T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
        int nRedundancy = secondary.GetSize();
        if (lost > nRedundancy) {
          if (!HandlePacketLost(lost - nRedundancy))
            break;
          totallost += lost - nRedundancy;
          lost = nRedundancy;
        }
        else
          nRedundancy = lost;

        receivedSequenceNumber -= lost;
        
        for (int i = nRedundancy - 1 ; i >= 0 ; i--) {
          if (!secondary[i].DecodeSubType(ifp)) {
            PTRACE(2, "T38\tUDPTLPacket decode failure:\n  "
                   << setprecision(2) << rawData << "\n  UDPTL = "
                   << setprecision(2) << udptl);
            break;
          }
          PTRACE(2, "T38\tReceived recovery ifp seq=" << receivedSequenceNumber << "\n  "
                 << setprecision(2) << ifp);
                 
          if (!HandlePacket(ifp))
            goto done;
            
          ifp = T38_IFPPacket();
            
          totalrecovered++;
          lost--;
          receivedSequenceNumber++;
        }
        receivedSequenceNumber += lost;
      }
      else {
        PTRACE(4, "T38\tNot implemented yet " << recovery.GetTagName());
      }

      if (lost) {
        if (!HandlePacketLost(lost))
          break;
        totallost += lost;
      } 
    }

    if (!udptl.m_primary_ifp_packet.DecodeSubType(ifp)) {
      PTRACE(2, "T38\tUDPTLPacket decode failure:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl);
      continue;
    }

#if 0
    // recovery test
    expectedSequenceNumber = receivedSequenceNumber;
    continue;
#endif

#if PTRACING
    if (PTrace::CanTrace(4)) {
      PTRACE(4, "T38\tReceived ifp seq=" << receivedSequenceNumber << "\n  "
             << setprecision(2) << ifp);
    }
    else {
      PTRACE(3, "T38\tReceived PDU:"
                " seq=" << receivedSequenceNumber <<
                " type=" << ifp.m_type_of_msg.GetTagName());
    }
#endif

    if (!HandlePacket(ifp))
      break;

    expectedSequenceNumber = receivedSequenceNumber + 1;
  }

done:

  PTRACE(2, "T38\tReceive statistics: sequence=" << expectedSequenceNumber
      << " repeated=" << repeated
      << " recovered=" << totalrecovered
      << " lost=" << totallost);
  return FALSE;
}
///////////////////////////////////////////////////////////////
//
void T38Engine::ModemCallbackWithUnlock(INT extra)
{
  PNotifier callback = modemCallback;
  
  if (!callback.IsNULL()) {
    Mutex.Signal();
    callback(*this, extra);
    Mutex.Wait();
  }
}

void T38Engine::CleanUpOnTermination()
{
  myPTRACE(1, name << " T38Engine::CleanUpOnTermination");
  OpalT38Protocol::CleanUpOnTermination();
  SetT38Mode(FALSE);
}

void T38Engine::SetT38Mode(BOOL mode)
{
  PWaitAndSignal mutexWait(Mutex);
  T38Mode = mode;
  myPTRACE(1, name << " T38Engine::SetT38Mode T38Mode=" << (T38Mode ? "TRUE" : "FALSE"));
  SignalOutDataReady();
  if (!T38Mode)
    ModemCallbackWithUnlock(cbpReset);
}

void T38Engine::ResetModemState() {
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if( modStreamIn && modStreamIn->DeleteFirstBuf() )
    PTRACE(1, name << " T38Engine::ResetModemState modStreamIn->DeleteFirstBuf(), clean");

  bufOut.PutEof();
  if( stateModem != stmIdle ) {
    myPTRACE(1, name << " T38Engine::ResetModemState stateModem(" << stateModem << ") != stmIdle, reset");
    stateModem = stmIdle;
  }
  callbackParamIn = -1;
  callbackParamOut = -1;
}
///////////////////////////////////////////////////////////////
void T38Engine::SendOnIdle(int _dataType)
{
  onIdleOut = _dataType;
  SignalOutDataReady();
}

BOOL T38Engine::SendStart(int _dataType, int param) {
  PWaitAndSignal mutexWaitModem(MutexModem);
  if (!IsT38Mode())
    return FALSE;

  if (stateModem != stmIdle)  {
    myPTRACE(1, name << " T38Engine::SendStart stateModem(" << stateModem << ") != stmIdle");
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
    myPTRACE(1, name << " T38Engine::Send stateModem(" << stateModem << ") != stmOutMoreData");
    return -1;
  }

  PWaitAndSignal mutexWait(Mutex);
  int res = bufOut.PutData(pBuf, count);
  if (res < 0)
    myPTRACE(1, name << " T38Engine::Send res(" << res << ") < 0");

  SignalOutDataReady();
  return res;
}

BOOL T38Engine::SendStop(BOOL moreFrames, int _callbackParam) {
  PWaitAndSignal mutexWaitModem(MutexModem);
  if(!IsT38Mode())
    return FALSE;

  if (stateModem != stmOutMoreData ) {
    myPTRACE(1, name << " T38Engine::SendStop stateModem(" << stateModem << ") != stmOutMoreData");
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
    myPTRACE(1, name << " T38Engine::RecvWait stateModem(" << stateModem << ") != stmIdle");
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
      PTRACE(1, name << " T38Engine::RecvWait modStreamIn->DeleteFirstBuf(), clean");
  
    if( modStreamIn->bufQ.GetSize() > 0 ) {
      PTRACE(1, name << " T38Engine::RecvWait modStreamIn->bufQ.GetSize()=" << modStreamIn->bufQ.GetSize());
      if( ModParsIn.val == modStreamIn->ModPars.val ) {
        PTRACE(1, name << " T38Engine::RecvWait ModParsIn.val == modStreamIn->ModPars.val(" <<  modStreamIn->ModPars.val << ")");
        stateModem = stmInReadyData;
        ModemCallbackWithUnlock(_callbackParam);
        return TRUE;
      }
      delete modStreamIn;
      modStreamIn = NULL;
    }
  }

  modStreamIn = new ModStream(ModParsIn);
  
  if( modStreamInSaved != NULL ) {
    if( modStreamIn->ModPars.val == modStreamInSaved->ModPars.val ) {
      myPTRACE(2, name << " T38Engine::RecvWait modStreamIn->ModPars.val(" <<  modStreamIn->ModPars.val << ") == modStreamInSaved->ModPars.val");
      modStreamIn->Move(*modStreamInSaved);
      delete modStreamInSaved;
      modStreamInSaved = NULL;
    } else {
      myPTRACE(2, name << " T38Engine::RecvWait modStreamIn->ModPars.val(" <<  modStreamIn->ModPars.val << ") != modStreamInSaved->ModPars.val(" << modStreamInSaved->ModPars.val << ")");
      modStreamIn->PushBuf();
      modStreamIn->PutEof(diagDiffSig);
    }
    stateModem = stmInReadyData;
    ModemCallbackWithUnlock(callbackParamIn);
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
    myPTRACE(1, name << " T38Engine::RecvStart stateModem(" << stateModem << ") != stmInReadyData");
    return FALSE;
  }
  PWaitAndSignal mutexWait(Mutex);
  callbackParamIn = _callbackParam;
  
  if( modStreamIn != NULL ) {
    if( modStreamIn->PopBuf() ) {
      stateModem = stmInRecvData;
      return TRUE;
    }
    myPTRACE(1, name << " T38Engine::RecvStart can't receive firstBuf");
    delete modStreamIn;
    modStreamIn = NULL;
  } else {
    myPTRACE(1, name << " T38Engine::RecvStart modStreamIn == NULL");
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
    myPTRACE(1, name << " T38Engine::Recv stateModem(" << stateModem << ") != stmInRecvData");
    return -1;
  }
  PWaitAndSignal mutexWait(Mutex);
  if( modStreamIn == NULL ) {
    myPTRACE(1, name << " T38Engine::Recv modStreamIn == NULL");
    return -1;
  }
  return modStreamIn->GetData(pBuf, count);
}

int T38Engine::RecvDiag()
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);
  if( modStreamIn == NULL ) {
    myPTRACE(1, name << " T38Engine::RecvDiag modStreamIn == NULL");
    return diagError;
  }
  if( modStreamIn->firstBuf == NULL ) {
    myPTRACE(1, name << " T38Engine::RecvDiag modStreamIn->firstBuf == NULL");
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
    myPTRACE(1, name << " T38Engine::RecvStop stateModem(" << stateModem << ") != stmIn");
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
int T38Engine::PreparePacket(T38_IFPPacket & ifp, BOOL enableTimeout)
{
  PWaitAndSignal mutexWaitOut(MutexOut);

  if(!IsT38Mode())
    return 0;
  
  //myPTRACE(1, name << " T38Engine::PreparePacket begin stM=" << stateModem << " stO=" << stateOut);
  
  ifp = T38_IFPPacket();
  BOOL doDalay = TRUE;
  BOOL wasCarrierIn = FALSE;

  for(;;) {
    BOOL redo = FALSE;
    
    if (doDalay) {
      int outDelay;
      
      if (delayRestOut > 0)
        outDelay = delayRestOut;
      else switch( stateOut ) {
        case stOutIdle:			outDelay = msPerOut; break;

        case stOutCedWait:		outDelay = ModParsOut.lenInd; break;
        case stOutSilenceWait:		outDelay = ModParsOut.lenInd; break;
        case stOutIndWait:		outDelay = ModParsOut.lenInd; break;

        case stOutData:
          outDelay = (countOut*8*1000)/ModParsOut.br + msPerOut - (PTime() - timeBeginOut).GetMilliSeconds();
          break;

        case stOutHdlcFcs:		outDelay = msPerOut; break;
        case stOutHdlcFlagsWait:	outDelay = msPerOut*3; break;

        case stOutDataNoSig:		outDelay = msPerOut; break;

        case stOutNoSig:		outDelay = msPerOut; break;
        default:			outDelay = 0;
      }

      //myPTRACE(1, name << " T38Engine::PreparePacket outDelay=" << outDelay);

      if (outDelay > 0) {
        if (outDelay > msTimeout) {
          delayRestOut = outDelay - msTimeout;
          outDelay = msTimeout;
          mySleep(outDelay);
          return -1;
        }
        else
          mySleep(outDelay);
      }
    } else
      doDalay = TRUE;
      
    delayRestOut = 0;

    if (!IsT38Mode())
      return 0;

    for(;;) {
      BOOL waitData = FALSE;
      {
        PWaitAndSignal mutexWait(Mutex);
        if( isStateModemOut() || stateOut != stOutIdle ) {
          switch( stateOut ) {
            case stOutIdle:
              if (isCarrierIn) {
                myPTRACE(1, name << " T38Engine::PreparePacket isCarrierIn for dataType=" << ModParsOut.dataType);
                int waitms = 0;
                /*
                 * We can't to begin sending data while the carrier is detected because
                 * it's possible that all data (including indication) will be losted.
                 * It's too critical for image data because it's possible to receive
                 * MCF generated for previous page after sending small page that was
                 * not delivered.
                 */
                switch( ModParsOut.dataType ) {
                  case dtHdlc:		waitms = 500; break;	// it's can't be too long
                  case dtRaw:		waitms = 2000; break;	// it's can't be too short
                }
                
                if (waitms) {
                  if (!wasCarrierIn) {
                    wasCarrierIn = TRUE;
                    timeBeginOut = PTime() + PTimeInterval(1000);
                    redo = TRUE;
                    break;
                  } else if (timeBeginOut > PTime()) {
                    redo = TRUE;
                    break;
                  } else {
                    myPTRACE(1, name << " T38Engine::PreparePacket isCarrierIn expired");
                  }
                }
              }

              wasCarrierIn = FALSE;

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
                  myPTRACE(1, name << " T38Engine::PreparePacket bad dataType=" << ModParsOut.dataType);
                  return 0;
              }
              break;
            ////////////////////////////////////////////////////
            case stOutCedWait:
              stateOut = stOutNoSig;
              stateModem = stmIdle;
              ModemCallbackWithUnlock(callbackParamOut);
              redo = TRUE;
              break;
            ////////////////////////////////////////////////////
            case stOutSilenceWait:
              stateOut = stOutIdle;
              stateModem = stmIdle;
              ModemCallbackWithUnlock(callbackParamOut);
              doDalay = FALSE;
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
                        myPTRACE(1, name << " T38Engine::PreparePacket stOutData bad dataType=" << ModParsOut.dataType);
                        return 0;
                    }
                    redo = TRUE;
                    break;
                  case 0:
                    if (lastDteCharOut != -1)
                      if (ModParsOut.dataType == dtHdlc || lastDteCharOut != 0)
                        ModemCallbackWithUnlock(cbpOutBufEmpty);
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
                        myPTRACE(1, name << " T38Engine::PreparePacket stOutData bad dataType=" << ModParsOut.dataType);
                        return 0;
                    }
                    lastDteCharOut = b[count - 1] & 0xFF;
                    countOut += count;
                }
              }
              break;
            ////////////////////////////////////////////////////
            case stOutHdlcFcs:
              if( stateModem != stmOutNoMoreData ) {
                myPTRACE(1, name << " T38Engine::PreparePacket stOutHdlcFcs stateModem(" << stateModem << ") != stmOutNoMoreData");
                return 0;
              }
              t38data(ifp, ModParsOut.msgType, T38F(e_hdlc_fcs_OK));
              if( moreFramesOut ) {
                stateOut = stOutHdlcFlagsWait;
                bufOut.Clean();		// reset eof
                stateModem = stmOutMoreData;
                ModemCallbackWithUnlock(callbackParamOut);
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
                myPTRACE(1, name << " T38Engine::PreparePacket stOutDataNoSig stateModem(" << stateModem << ") != stmOutNoMoreData");
                return 0;
              }
              switch( ModParsOut.dataType ) {
                case dtHdlc:
                  t38data(ifp, ModParsOut.msgType, T38F(e_hdlc_sig_end));
                  break;
                case dtRaw:
                  t38data(ifp, ModParsOut.msgType, T38F(e_t4_non_ecm_sig_end));
                  break;
                default:
                  myPTRACE(1, name << " T38Engine::PreparePacket stOutDataNoSig bad dataType=" << ModParsOut.dataType);
                  return 0;
              }
              stateOut = stOutNoSig;
              stateModem = stmIdle;
              ModemCallbackWithUnlock(callbackParamOut);
              break;
            ////////////////////////////////////////////////////
            case stOutNoSig:
              t38indicator(ifp, T38I(e_no_signal));
              stateOut = stOutIdle;
              break;
            default:
              myPTRACE(1, name << " T38Engine::PreparePacket bad stateOut=" << stateOut);
              return 0;
          }
        } else {
          switch( onIdleOut ) {
            case dtCng:
              t38indicator(ifp, T38I(e_cng));
              break;
            case dtSilence:
              t38indicator(ifp, T38I(e_no_signal));
              break;
            default:
              waitData = TRUE;
          }
          onIdleOut = dtNone;
        }
      }
      if (!waitData)
        break;
      if (enableTimeout)
        if (!WaitOutDataReady(msTimeout))
          return -1;
      else
        WaitOutDataReady();
      if (!IsT38Mode())
        return 0;

      {
        PWaitAndSignal mutexWait(Mutex);
        if( stateOut == stOutData ) {
          myPTRACE(1, name << " T38Engine::PreparePacket DTE's data delay, reset " << countOut);
          countOut = 0;
          timeBeginOut = PTime() - PTimeInterval(msPerOut);
          doDalay = FALSE;
        }
      }
    }
    if( !redo ) break;
  }
  return 1;
}
///////////////////////////////////////////////////////////////
BOOL T38Engine::HandlePacketLost(unsigned nLost)
{
  PWaitAndSignal mutexWaitIn(MutexIn);

  if (!IsT38Mode())
    return FALSE;

  myPTRACE(1, name << " T38Engine::HandlePacketLost " << nLost);
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
        myPTRACE(1, name << " T38Engine::HandlePacket indicator && modStreamIn->lastBuf != NULL");

      if( modStreamInSaved != NULL ) {
        myPTRACE(1, name << " T38Engine::HandlePacket indicator && modStreamInSaved != NULL, clean");
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
          isCarrierIn = 1;
          modStreamInSaved = new ModStream(GetModPars((const T38_Type_of_msg_t30_indicator &)ifp.m_type_of_msg, by_ind));
          modStreamInSaved->PushBuf();
            
          if( stateModem == stmInWaitData ) {
            if( modStreamIn != NULL ) {
              if( modStreamIn->ModPars.val == modStreamInSaved->ModPars.val ) {
                modStreamIn->Move(*modStreamInSaved);
                delete modStreamInSaved;
                modStreamInSaved = NULL;
              } else {
                myPTRACE(2, name << " T38Engine::HandlePacket modStreamIn->ModPars.val(" <<  modStreamIn->ModPars.val << ") != modStreamInSaved->ModPars.val(" << modStreamInSaved->ModPars.val << ")");
                modStreamIn->PushBuf();
                modStreamIn->PutEof(diagDiffSig);
              }
            } else {
              myPTRACE(1, name << " T38Engine::HandlePacket modStreamIn == NULL");
            }
            stateModem = stmInReadyData;
            ModemCallbackWithUnlock(callbackParamIn);
          }
          break;
        default:
          myPTRACE(1, name << " T38Engine::HandlePacket type_of_msg bad !!! " << setprecision(2) << ifp);
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
          PTRACE(1, name << " T38Engine::HandlePacket lastBuf == NULL");
          break;
        }
        if( modStream->ModPars.msgType != type_of_msg ) {
          myPTRACE(1, name << " T38Engine::HandlePacket modStream->ModPars.msgType(" << modStream->ModPars.msgType << ") != type_of_msg(" << type_of_msg << ")");
          modStream->PutEof(diagOutOfOrder);
          if( stateModem == stmInRecvData )
            ModemCallbackWithUnlock(callbackParamIn);
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
                  PTRACE(1, name << " T38Engine::HandlePacket modStream == NULL");
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
                    myPTRACE(1, name << " T38Engine::HandlePacket field_type bad !!! " << setprecision(2) << ifp);
                }
                switch( Data_Field.m_field_type ) {	// Handle fcs_BAD
                  case T38F(e_hdlc_fcs_BAD):
                  case T38F(e_hdlc_fcs_BAD_sig_end):
                    modStream->SetDiag(diagBadFcs);
                    myPTRACE(1, name << " T38Engine::HandlePacket bad FCS");
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
            myPTRACE(1, name << " T38Engine::HandlePacket type_of_msg bad !!! " << setprecision(2) << ifp);
        }
        if( stateModem == stmInRecvData )
          ModemCallbackWithUnlock(callbackParamIn);
        break;
      }
    default:
      myPTRACE(1, name << " T38Engine::HandlePacket Tag bad !!! " << setprecision(2) << ifp);
  }
  return TRUE;
}
///////////////////////////////////////////////////////////////

