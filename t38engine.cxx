/*
 * t38engine.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2010 Vyacheslav Frolov
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
 * Revision 1.73  2010-10-12 16:46:25  vfrolov
 * Implemented fake streams
 *
 * Revision 1.73  2010/10/12 16:46:25  vfrolov
 * Implemented fake streams
 *
 * Revision 1.72  2010/10/08 06:09:53  vfrolov
 * Added parameter assert to SetPreparePacketTimeout()
 *
 * Revision 1.71  2010/10/06 16:54:19  vfrolov
 * Redesigned engine opening/closing
 *
 * Revision 1.70  2010/09/22 15:39:19  vfrolov
 * Moved ResetModemState() to EngineBase
 * Replaced _ResetModemState() by OnResetModemState()
 *
 * Revision 1.69  2010/09/08 17:22:23  vfrolov
 * Redesigned modem engine (continue)
 *
 * Revision 1.68  2010/04/22 15:41:30  vfrolov
 * Fixed +FRS delay if remote does not send no-signal indicator
 *
 * Revision 1.67  2010/03/23 08:58:14  vfrolov
 * Fixed issues with +FTS and +FRS
 *
 * Revision 1.66  2010/03/18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.65  2010/02/27 11:11:54  vfrolov
 * Added missing redo
 *
 * Revision 1.64  2010/01/28 10:27:03  vfrolov
 * Added handling T.38 CED indication
 *
 * Revision 1.63  2009/12/02 09:06:42  vfrolov
 * Added a short delay after transmitting of signal before call clearing
 *
 * Revision 1.62  2009/11/26 07:21:37  vfrolov
 * Added delay between transmitting of signals
 *
 * Revision 1.61  2009/11/19 14:48:28  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.60  2009/11/19 11:18:16  vfrolov
 * Added handling T.38 CED indication
 *
 * Revision 1.59  2009/11/18 19:08:47  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.58  2009/11/10 09:56:54  vfrolov
 * Fixed isCarrierIn handling
 *
 * Revision 1.57  2009/11/10 08:13:38  vfrolov
 * Fixed race condition on re-opening T38Engine
 *
 * Revision 1.56  2009/11/06 10:19:29  vfrolov
 * Fixed indication handling after re-opening T38Engine
 *
 * Revision 1.55  2009/10/27 18:53:49  vfrolov
 * Added ability to re-open T38Engine
 * Added ability to prepare IFP packets with adaptive delay/period
 *
 * Revision 1.54  2009/07/27 16:21:24  vfrolov
 * Moved h323lib specific code to h323lib directory
 *
 * Revision 1.53  2009/07/03 09:12:04  vfrolov
 * Included opal/buildopts.h
 *
 * Revision 1.52  2009/07/02 15:15:43  vfrolov
 * Fixed handling 5 sec buffer empty event for +FTM command
 *
 * Revision 1.51  2009/07/02 13:03:22  vfrolov
 * Fixed aborting +FTM immediately after CONNECT if no data in the buffer
 *
 * Revision 1.50  2009/02/05 14:15:18  vfrolov
 * Added missing cbpOutBufNoFull notification (for ECM)
 *
 * Revision 1.49  2009/01/27 14:00:50  vfrolov
 * Added missing startedTimeOutBufEmpty initialization
 *
 * Revision 1.48  2008/09/24 14:51:45  frolov
 * Added 5 sec. timeout  for DCE's transmit buffer empty
 *
 * Revision 1.47  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.46  2007/05/24 17:03:54  vfrolov
 * Added more PTRACING checks
 *
 * Revision 1.45  2007/05/10 10:40:33  vfrolov
 * Added ability to continuously resend last UDPTL packet
 *
 * Revision 1.44  2007/05/03 09:21:47  vfrolov
 * Added compile time optimization for original ASN.1 sequence
 * in T.38 (06/98) Annex A or for CORRIGENDUM No. 1 fix
 *
 * Revision 1.43  2007/04/26 13:57:17  vfrolov
 * Changed logging for OPAL
 *
 * Revision 1.42  2007/04/11 14:19:15  vfrolov
 * Changed for OPAL
 *
 * Revision 1.41  2007/03/23 10:14:36  vfrolov
 * Implemented voice mode functionality
 *
 * Revision 1.40  2006/12/11 11:19:48  vfrolov
 * Fixed race condition with modem Callback
 *
 * Revision 1.39  2006/12/07 10:50:39  vfrolov
 * Fixed possible dead lock
 *
 * Revision 1.38  2006/10/20 10:04:00  vfrolov
 * Added code for ignoring repeated indicators
 * Added code for sending repeated indicators (disabled by default)
 *
 * Revision 1.37  2006/07/05 04:37:17  csoutheren
 * Applied 1488904 - SetPromiscuous(AcceptFromLastReceivedOnly) for T.38
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.36  2005/07/21 06:49:02  vfrolov
 * Added missing CompleteEncoding()
 *
 * Revision 1.35  2005/07/18 11:39:47  vfrolov
 * Changed for OPAL
 *
 * Revision 1.34  2005/03/03 16:09:06  vfrolov
 * Fixed memory leak
 *
 * Revision 1.33  2005/02/04 10:18:49  vfrolov
 * Fixed warnings for No Trace build
 *
 * Revision 1.32  2005/02/03 11:32:12  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.31  2004/08/24 16:12:10  vfrolov
 * Fixed bit counter overflow
 *
 * Revision 1.30  2004/07/06 16:07:24  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.29  2004/06/18 15:06:29  vfrolov
 * Fixed race condition by adding mutex for modemCallback
 *
 * Revision 1.28  2004/03/01 17:10:32  vfrolov
 * Fixed duplicated mutexes
 * Added volatile to T38Mode
 *
 * Revision 1.27  2003/12/04 16:09:58  vfrolov
 * Implemented FCS generation
 * Implemented ECM support
 *
 * Revision 1.26  2003/01/08 16:55:52  vfrolov
 * Added cbpOutBufNoFull and isOutBufFull()
 * Added data speed tracing
 * Fixed "thread did not terminate" (added msMaxOutDelay)
 * Discarded useless name tracing
 *
 * Revision 1.25  2002/12/30 12:49:46  vfrolov
 * Added tracing thread's CPU usage (Linux only)
 *
 * Revision 1.24  2002/12/20 10:13:08  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.23  2002/12/19 14:19:20  vfrolov
 * Added missing brackets (fixed all CPU usage reported by Markus Storm)
 *
 * Revision 1.22  2002/12/19 11:54:43  vfrolov
 * Removed DecodeIFPPacket() and utilized HandleRawIFP()
 *
 * Revision 1.21  2002/11/28 09:17:31  vfrolov
 * Added missing const
 *
 * Revision 1.20  2002/11/21 07:16:46  robertj
 * Changed promiscuous mode to be three way. Fixes race condition in gkserver
 *   which can cause crashes or more PDUs to be sent to the wrong place.
 *
 * Revision 1.19  2002/11/18 23:12:17  craigs
 * Removed reference to t38old.h
 *
 * Revision 1.18  2002/11/18 23:01:58  craigs
 * Changed name of pre CORRIGENDUM ASN
 *
 * Revision 1.17  2002/11/18 22:57:53  craigs
 * Added patches from Vyacheslav Frolov for CORRIGENDUM
 *
 * Revision 1.16  2002/11/15 07:43:52  vfrolov
 * Do not wait no-signal if received *-sig-end
 * Fixed compiler warnings
 *
 * Revision 1.15  2002/05/22 15:21:26  vfrolov
 * Added missed enableTimeout check
 * Fixed bad ifp tracing bug
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

#include <ptlib.h>
#include <opal_config.h>
#include <asn/t38.h>

#include "pmutils.h"
#include "t38engine.h"

#define new PNEW

#define T38I(t30_indicator) T38_Type_of_msg_t30_indicator::t30_indicator
#define T38D(msg_data) T38_Type_of_msg_data::msg_data
#define T38F(field_type) T38_Data_Field_subtype_field_type::field_type
#define msMaxOutDelay (msPerOut*5)

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
  stOutDataNoSig,
  stOutNoSig,
};
///////////////////////////////////////////////////////////////
enum StateModem {
  stmIdle,

  stmOutMoreData,
  stmOutNoMoreData,

  stmInWaitSilence,

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
    PBoolean DeleteFirstBuf();
    PBoolean PopBuf();
    int GetData(void *pBuf, PINDEX count);
    int PutData(const void *pBuf, PINDEX count);
    PBoolean SetDiag(int diag);
    PBoolean PutEof(int diag = 0);
    void Move(ModStream &from);

    DataStream *firstBuf;
    DataStreamQ bufQ;
    DataStream *lastBuf;	// if not NULL then shold be in bufQ or firstBuf
    MODPARS ModPars;

    HDLC hdlc;
};

ModStream::ModStream(const MODPARS &_ModPars) : firstBuf(NULL), lastBuf(NULL), ModPars(_ModPars)
{
}

ModStream::~ModStream()
{
  if (firstBuf != NULL) {
    myPTRACE(1, "T38Modem\tModStream::~ModStream firstBuf != NULL, clean");
    delete firstBuf;
  }
  PTRACE_IF(1, bufQ.GetSize() > 0,
    "T38Modem\tModStream::~ModStream bufQ.GetSize()=" << bufQ.GetSize() << ", clean");
}

void ModStream::PushBuf()
{
  lastBuf = new DataStream();
  bufQ.Enqueue(lastBuf);
}

PBoolean ModStream::DeleteFirstBuf()
{
  if (firstBuf != NULL) {
    if (lastBuf == firstBuf)
      lastBuf = NULL;
    delete firstBuf;
    firstBuf = NULL;
    return TRUE;
  }
  return FALSE;
}

PBoolean ModStream::PopBuf()
{
  if (DeleteFirstBuf()) {
    myPTRACE(1, "T38Modem\tModStream::PopBuf DeleteFirstBuf(), clean");
  }
  firstBuf = bufQ.Dequeue();
  if (!firstBuf)
    return FALSE;

  if (ModPars.dataType == EngineBase::dtRaw && ModPars.dataTypeT38 == EngineBase::dtHdlc) {
    hdlc = HDLC();
    hdlc.PutHdlcData(firstBuf);
    hdlc.GetRawStart(10);
  }
  return TRUE;
}

int ModStream::GetData(void *pBuf, PINDEX count)
{
  if (firstBuf == NULL) {
    myPTRACE(1, "T38Modem\tModStream::GetData firstBuf == NULL");
    return -1;
  }

  if (ModPars.dataType == EngineBase::dtRaw && ModPars.dataTypeT38 == EngineBase::dtHdlc) {
    int len;

    while ((len = hdlc.GetData(pBuf, count)) < 0) {
      DataStream *_firstBuf;
      if ((_firstBuf = bufQ.Dequeue()) != NULL) {
        DeleteFirstBuf();
        firstBuf = _firstBuf;
        hdlc.PutHdlcData(firstBuf);
        hdlc.GetRawStart();
      } else {
        if ((firstBuf->GetDiag() & EngineBase::diagNoCarrier) == 0) {
          DeleteFirstBuf();
          return 0;
        }
        return -1;
      }
    }
    return len;
  }

  if (ModPars.dataTypeT38 != ModPars.dataType) {
    myPTRACE(1, "T38Modem\tModStream::GetData ModPars.dataType("
      << ModPars.dataType
      << ") != ModPars.dataTypeT38("
      << ModPars.dataTypeT38
      << ")");
    return -1;
  }

  return firstBuf->GetData(pBuf, count);
}

int ModStream::PutData(const void *pBuf, PINDEX count)
{
  if( lastBuf == NULL ) {
    myPTRACE(1, "T38Modem\tModStream::PutData lastBuf == NULL");
    return -1;
  }
  return lastBuf->PutData(pBuf, count);
}

PBoolean ModStream::SetDiag(int diag)
{
  if( lastBuf == NULL )
    return FALSE;
  lastBuf->SetDiag(lastBuf->GetDiag() | diag);
  return TRUE;
}

PBoolean ModStream::PutEof(int diag)
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
MODPARS::MODPARS(int _val, unsigned _ind, int _lenInd, unsigned _msgType, int _br)
      : dataType(EngineBase::dtCed), dataTypeT38(EngineBase::dtCed),
        val(_val), ind(_ind), lenInd(_lenInd),
        msgType(_msgType), br(_br)
{
}

static const MODPARS mods[] = {
MODPARS(   3, T38I(e_v21_preamble),              900, T38D(e_v21),         300 ),
MODPARS(  24, T38I(e_v27_2400_training),        1100, T38D(e_v27_2400),   2400 ),
MODPARS(  48, T38I(e_v27_4800_training),         900, T38D(e_v27_4800),   4800 ),
MODPARS(  72, T38I(e_v29_7200_training),         300, T38D(e_v29_7200),   7200 ),
MODPARS(  73, T38I(e_v17_7200_long_training),   1500, T38D(e_v17_7200),   7200 ),
MODPARS(  74, T38I(e_v17_7200_short_training),   300, T38D(e_v17_7200),   7200 ),
MODPARS(  96, T38I(e_v29_9600_training),         300, T38D(e_v29_9600),   9600 ),
MODPARS(  97, T38I(e_v17_9600_long_training),   1500, T38D(e_v17_9600),   9600 ),
MODPARS(  98, T38I(e_v17_9600_short_training),   300, T38D(e_v17_9600),   9600 ),
MODPARS( 121, T38I(e_v17_12000_long_training),  1500, T38D(e_v17_12000), 12000 ),
MODPARS( 122, T38I(e_v17_12000_short_training),  300, T38D(e_v17_12000), 12000 ),
MODPARS( 145, T38I(e_v17_14400_long_training),  1500, T38D(e_v17_14400), 14400 ),
MODPARS( 146, T38I(e_v17_14400_short_training),  300, T38D(e_v17_14400), 14400 ),
};

static const MODPARS invalidMods;

enum GetModParsBy {
  by_val,
  by_ind,
};

static const MODPARS &GetModPars(int key, enum GetModParsBy by = by_val) {
  for( PINDEX i = 0 ; i < PINDEX(sizeof(mods)/sizeof(mods[0])) ; i++ ) {
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
static void t38indicator(T38_IFP &ifp, unsigned type)
{
    ifp.m_type_of_msg.SetTag(T38_Type_of_msg::e_t30_indicator);
    (T38_Type_of_msg_t30_indicator &)ifp.m_type_of_msg = type;
}

#ifdef OPTIMIZE_CORRIGENDUM_IFP
  #define T38_DATA_FIELD T38_Data_Field_subtype
#else
  #define T38_DATA_FIELD T38_PreCorrigendum_Data_Field_subtype
#endif

static T38_DATA_FIELD &t38data(T38_IFP &ifp, unsigned type, unsigned field_type)
{
    ifp.m_type_of_msg.SetTag(T38_Type_of_msg::e_data);
    (T38_Type_of_msg_data &)ifp.m_type_of_msg = type;

    ifp.IncludeOptionalField(T38_IFPPacket::e_data_field);
    ifp.m_data_field.SetSize(ifp.m_data_field.GetSize()+1);
    T38_DATA_FIELD &Data_Field = ifp.m_data_field[0];
    Data_Field.m_field_type = field_type;
    return Data_Field;
}

static void t38data(T38_IFP &ifp, unsigned type, unsigned field_type, const PBYTEArray &data)
{
    T38_DATA_FIELD &Data_Field = t38data(ifp, type, field_type);

    if( data.GetSize() > 0 ) {
        Data_Field.IncludeOptionalField(T38_Data_Field_subtype::e_field_data);
        Data_Field.m_field_data = data;
    }
}
///////////////////////////////////////////////////////////////
class FakePreparePacketThread : public PThread
{
    PCLASSINFO(FakePreparePacketThread, PThread);
  public:
    FakePreparePacketThread(T38Engine &engine)
      : PThread(30000,NoAutoDeleteThread)
      , t38engine(engine)
    {
      myPTRACE(3, "T38Modem\t" << t38engine.Name() << " FakePreparePacketThread");
      t38engine.AddReference();
    }

    ~FakePreparePacketThread()
    {
      myPTRACE(3, "T38Modem\t" << t38engine.Name() << " ~FakePreparePacketThread");
      ReferenceObject::DelPointer(&t38engine);
    }

  protected:
    virtual void Main();

    T38Engine &t38engine;
};

void FakePreparePacketThread::Main()
{
  myPTRACE(3, "T38Modem\t" << t38engine.Name() << " FakePreparePacketThread::Main started");

  t38engine.OpenOut(EngineBase::HOWNEROUT(this), TRUE);
  t38engine.SetPreparePacketTimeout(EngineBase::HOWNEROUT(this), -1);

#if PTRACING
  unsigned long count = 0;
#endif

  for (;;) {
    T38_IFP ifp;
    int res;

    res = t38engine.PreparePacket(EngineBase::HOWNEROUT(this), ifp);

    if (res == 0)
      break;

#if PTRACING
    if (res > 0) {
      count++;
      myPTRACE(4, "T38Modem\t" << t38engine.Name() << " FakePreparePacketThread::Main ifp = " << setprecision(2) << ifp);
    }
#endif
  }

  t38engine.CloseOut(EngineBase::HOWNEROUT(this));

  myPTRACE(3, "T38Modem\t" << t38engine.Name() << " FakePreparePacketThread::Main stopped, faked out " << count << " IFP packets");
}
///////////////////////////////////////////////////////////////
T38Engine::T38Engine(const PString &_name)
  : EngineBase(_name + " T38Engine")
  , bufOut(2048)
  , preparePacketTimeout(-1)
  , preparePacketPeriod(-1)
  , preparePacketDelay()
  , stateOut(stOutNoSig)
  , onIdleOut(dtNone)
  , callbackParamOut(cbpReset)
  , ModParsOut()
  , delaySignalOut(FALSE)
  , startedTimeOutBufEmpty(FALSE)
  , timeOutBufEmpty()
  , timeDelayEndOut()
  , timeBeginOut()
  , countOut(0)
  , moreFramesOut(FALSE)
  , hdlcOut()
  , callbackParamIn(cbpReset)
  , isCarrierIn(0)
#if PTRACING
  , timeBeginIn()
#endif
  , countIn(0)
  , t30()
  , modStreamIn(NULL)
  , modStreamInSaved(NULL)
  , stateModem(stmIdle)
{
  myPTRACE(2, "T38Modem\t" << name << " T38Engine");
}

T38Engine::~T38Engine()
{
  myPTRACE(1, "T38Modem\t" << name << " ~T38Engine");

  if (modStreamIn != NULL)
    delete modStreamIn;

  if (modStreamInSaved != NULL)
    delete modStreamInSaved;
}

void T38Engine::OnOpenIn()
{
  EngineBase::OnOpenIn();
}

void T38Engine::OnOpenOut()
{
  EngineBase::OnOpenOut();
}

void T38Engine::OnCloseIn()
{
  EngineBase::OnCloseIn();
  SignalOutDataReady();
}

void T38Engine::OnCloseOut()
{
  EngineBase::OnCloseOut();
  SignalOutDataReady();
}

void T38Engine::OnChangeEnableFakeIn()
{
  EngineBase::OnChangeEnableFakeIn();

  if (IsOpenIn() || !isEnableFakeIn)
    return;

  isCarrierIn = 0;

  if (modStreamInSaved != NULL) {
    myPTRACE(1, "T38Modem\t" << name << " OnChangeEnableFakeIn modStreamInSaved != NULL, clean");
    delete modStreamInSaved;
    modStreamInSaved = NULL;
  }

  if (modStreamIn != NULL && modStreamIn->lastBuf != NULL) {
    myPTRACE(1, "T38Modem\t" << name << " OnChangeEnableFakeIn modStreamIn->lastBuf != NULL");
    modStreamIn->PutEof((countIn == 0 ? 0 : diagOutOfOrder) | diagNoCarrier);

    if (stateModem == stmInRecvData) {
      ModemCallbackWithUnlock(callbackParamIn);

      if (IsOpenIn() || !isEnableFakeIn)
        return;
    }
  }

  if (stateModem == stmInWaitSilence) {
    stateModem = stmIdle;
    ModemCallbackWithUnlock(callbackParamIn);

    //if (IsOpenIn() || !isEnableFakeIn)
    //  return;
  }
}

void T38Engine::OnChangeEnableFakeOut()
{
  EngineBase::OnChangeEnableFakeOut();
  SignalOutDataReady();

  if (IsOpenOut() || !isEnableFakeOut)
    return;

  if (stateModem != stmOutMoreData && stateModem != stmOutNoMoreData)
    return;

  (new FakePreparePacketThread(*this))->Resume();
}

void T38Engine::OnAttach()
{
  EngineBase::OnAttach();
}

void T38Engine::OnDetach()
{
  EngineBase::OnDetach();
  SignalOutDataReady();
}

void T38Engine::OnChangeModemClass()
{
  EngineBase::OnChangeModemClass();
}
///////////////////////////////////////////////////////////////
//
void T38Engine::OnResetModemState() {
  EngineBase::OnResetModemState();

  if (modStreamIn && modStreamIn->DeleteFirstBuf()) {
    myPTRACE(1, "T38Modem\t" << name << " T38Engine::OnResetModemState modStreamIn->DeleteFirstBuf(), clean");
  }

  bufOut.PutEof();
  if (stateModem != stmIdle) {
    if (!isStateModemOut()) {
      myPTRACE(1, "T38Modem\t" << name << " T38Engine::OnResetModemState stateModem(" << stateModem << ") != stmIdle, reset");
      stateModem = stmIdle;
    } else
      myPTRACE(1, "T38Modem\t" << name << " T38Engine::OnResetModemState stateModem(" << stateModem << ") != stmIdle");
  }

  onIdleOut = dtNone;
  callbackParamIn = cbpReset;
  callbackParamOut = cbpReset;
}

PBoolean T38Engine::isOutBufFull() const
{
  PWaitAndSignal mutexWait(Mutex);
  return bufOut.isFull();
}
///////////////////////////////////////////////////////////////
void T38Engine::SendOnIdle(DataType _dataType)
{
  myPTRACE(2, "T38Modem\t" << name << " SendOnIdle " << _dataType);

  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  onIdleOut = _dataType;
  SignalOutDataReady();
}

PBoolean T38Engine::SendStart(DataType _dataType, int param)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return FALSE;

  if (stateModem != stmIdle)  {
    myPTRACE(1, "T38Modem\t" << name << " SendStart stateModem(" << stateModem << ") != stmIdle");
    return FALSE;
  }

  PWaitAndSignal mutexWait(Mutex);

  if (modStreamIn != NULL) {
    delete modStreamIn;
    modStreamIn = NULL;
  }

  if (modStreamInSaved != NULL && _dataType != dtSilence) {
    delete modStreamInSaved;
    modStreamInSaved = NULL;
  }

  ModParsOut = invalidMods;

  switch( _dataType ) {
    case dtCed:
      ModParsOut.dataTypeT38 = ModParsOut.dataType = _dataType;
      ModParsOut.ind = T38I(e_ced);
      ModParsOut.lenInd = param;
      break;
    case dtSilence:
      ModParsOut.dataTypeT38 = ModParsOut.dataType = _dataType;
      ModParsOut.ind = T38I(e_no_signal);
      ModParsOut.lenInd = param;
      break;
    case dtHdlc:
    case dtRaw:
      ModParsOut = GetModPars(param);
      ModParsOut.dataType = _dataType;
      ModParsOut.dataTypeT38 =
          (ModParsOut.msgType == T38D(e_v21) || t30.hdlcOnly()) ? dtHdlc : dtRaw;
      if (!ModParsOut.IsModValid())
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

int T38Engine::Send(const void *pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return -1;

  if (stateModem != stmOutMoreData) {
    myPTRACE(1, "T38Modem\t" << name << " Send stateModem(" << stateModem << ") != stmOutMoreData");
    return -1;
  }

  PWaitAndSignal mutexWait(Mutex);
  int res = bufOut.PutData(pBuf, count);
  if (res < 0) {
    myPTRACE(1, "T38Modem\t" << name << " Send res(" << res << ") < 0");
  }

  SignalOutDataReady();
  return res;
}

PBoolean T38Engine::SendStop(PBoolean moreFrames, int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if(!IsModemOpen())
    return FALSE;

  if (stateModem != stmOutMoreData ) {
    myPTRACE(1, "T38Modem\t" << name << " SendStop stateModem(" << stateModem << ") != stmOutMoreData");
    return FALSE;
  }

  PWaitAndSignal mutexWait(Mutex);
  bufOut.PutEof();
  stateModem = stmOutNoMoreData;
  moreFramesOut = moreFrames;
  callbackParamOut = _callbackParam;

  myPTRACE(3, "T38Modem\t" << name << " SendStop moreFramesOut=" << moreFramesOut
                 << " callbackParamOut=" << callbackParamOut);

  SignalOutDataReady();
  return TRUE;
}
///////////////////////////////////////////////////////////////
PBoolean T38Engine::RecvWait(DataType _dataType, int param, int _callbackParam, PBoolean &done)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return FALSE;

  if( stateModem != stmIdle ) {
    myPTRACE(1, "T38Modem\t" << name << " RecvWait stateModem(" << stateModem << ") != stmIdle");
    return FALSE;
  }

  PWaitAndSignal mutexWait(Mutex);
  switch( _dataType ) {
    case dtHdlc:
    case dtRaw:
      break;
    case dtSilence:
      if (modStreamIn != NULL) {
        delete modStreamIn;
        modStreamIn = NULL;

        if (isCarrierIn && !modStreamInSaved) {
          callbackParamIn = _callbackParam;
          stateModem = stmInWaitSilence;
          return TRUE;
        }
      }

      done = TRUE;
      return TRUE;
    default:
      return FALSE;
  }

  const MODPARS &ModParsIn = GetModPars(param);

  if (!ModParsIn.IsModValid())
    return FALSE;

  callbackParamIn = _callbackParam;

  if (modStreamIn != NULL) {
    if (modStreamIn->DeleteFirstBuf()) {
      myPTRACE(1, "T38Modem\t" << name << " RecvWait modStreamIn->DeleteFirstBuf(), clean");
    }

    if (modStreamIn->bufQ.GetSize() > 0) {
      myPTRACE(1, "T38Modem\t" << name << " RecvWait modStreamIn->bufQ.GetSize()=" << modStreamIn->bufQ.GetSize());
      if (ModParsIn.IsEqual(modStreamIn->ModPars)) {
        myPTRACE(1, "T38Modem\t" << name << " RecvWait ModParsIn == modStreamIn->ModPars("
          << modStreamIn->ModPars.val
          << ")");
        modStreamIn->ModPars.dataType = _dataType;
        modStreamIn->ModPars.dataTypeT38 =
            (modStreamIn->ModPars.msgType == T38D(e_v21) || t30.hdlcOnly()) ? dtHdlc : dtRaw;
        stateModem = stmInReadyData;
        done = TRUE;
        return TRUE;
      }
    }
    delete modStreamIn;
  }

  modStreamIn = new ModStream(ModParsIn);
  modStreamIn->ModPars.dataType = _dataType;
  modStreamIn->ModPars.dataTypeT38 =
      (modStreamIn->ModPars.msgType == T38D(e_v21) || t30.hdlcOnly()) ? dtHdlc : dtRaw;

  if (modStreamInSaved != NULL) {
    if (modStreamIn->ModPars.IsEqual(modStreamInSaved->ModPars)) {
      myPTRACE(2, "T38Modem\t" << name << " RecvWait modStreamIn->ModPars == modStreamInSaved->ModPars("
        << modStreamInSaved->ModPars.val
        << ")");
      modStreamIn->Move(*modStreamInSaved);
      delete modStreamInSaved;
      modStreamInSaved = NULL;
    } else {
      myPTRACE(2, "T38Modem\t" << name << " RecvWait modStreamIn->ModPars("
        << modStreamIn->ModPars.val
        << ") != modStreamInSaved->ModPars("
        << modStreamInSaved->ModPars.val
        << ")");
      modStreamIn->PushBuf();
      modStreamIn->PutEof(diagDiffSig);
    }
    stateModem = stmInReadyData;
    done = TRUE;
    return TRUE;
  }

  stateModem = stmInWaitData;
  return TRUE;
}

PBoolean T38Engine::RecvStart(int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return FALSE;

  if (stateModem != stmInReadyData ) {
    myPTRACE(1, "T38Modem\t" << name << " RecvStart stateModem(" << stateModem << ") != stmInReadyData");
    return FALSE;
  }
  PWaitAndSignal mutexWait(Mutex);
  callbackParamIn = _callbackParam;

  if (modStreamIn != NULL) {
    if (modStreamIn->PopBuf()) {
      if (modStreamIn->ModPars.msgType == T38D(e_v21))
        t30.v21Begin();
      stateModem = stmInRecvData;
      return TRUE;
    }
    myPTRACE(1, "T38Modem\t" << name << " RecvStart can't receive firstBuf");
    delete modStreamIn;
    modStreamIn = NULL;
  } else {
    myPTRACE(1, "T38Modem\t" << name << " RecvStart modStreamIn == NULL");
  }

  stateModem = stmIdle;
  return FALSE;
}

int T38Engine::Recv(void *pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return -1;

  if( stateModem != stmInRecvData ) {
    myPTRACE(1, "T38Modem\t" << name << " Recv stateModem(" << stateModem << ") != stmInRecvData");
    return -1;
  }
  PWaitAndSignal mutexWait(Mutex);
  if( modStreamIn == NULL ) {
    myPTRACE(1, "T38Modem\t" << name << " Recv modStreamIn == NULL");
    return -1;
  }

  int len = modStreamIn->GetData(pBuf, count);

  if (modStreamIn->ModPars.msgType == T38D(e_v21)) {
    if (len > 0)
      t30.v21Data(pBuf, len);
    else
    if (len < 0)
      t30.v21End(FALSE);
  }

  return len;
}

int T38Engine::RecvDiag() const
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);
  if( modStreamIn == NULL ) {
    myPTRACE(1, "T38Modem\t" << name << " RecvDiag modStreamIn == NULL");
    return diagError;
  }
  if( modStreamIn->firstBuf == NULL ) {
    myPTRACE(1, "T38Modem\t" << name << " RecvDiag modStreamIn->firstBuf == NULL");
    return diagError;
  }
  return modStreamIn->firstBuf->GetDiag();
}

void T38Engine::RecvStop()
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if(!IsModemOpen())
    return;

  if(!isStateModemIn()) {
    myPTRACE(1, "T38Modem\t" << name << " RecvStop stateModem(" << stateModem << ") in not receiving data state");
    return;
  }

  PWaitAndSignal mutexWait(Mutex);

  if (modStreamIn)
    modStreamIn->DeleteFirstBuf();

  if (isStateModemIn())
    stateModem = stmIdle;
}
///////////////////////////////////////////////////////////////
PBoolean T38Engine::SendingNotCompleted() const
{
  PWaitAndSignal mutexWait(Mutex);

  if (hOwnerOut == NULL)
    return FALSE;

  if (stateOut != stOutIdle)
    return TRUE;

  if (delaySignalOut && timeBeginOut > PTime())
    return TRUE;

  return FALSE;
}
///////////////////////////////////////////////////////////////
void T38Engine::SetPreparePacketTimeout(HOWNEROUT hOwner, int timeout, int period)
{
  PAssert((timeout == 0 && period > 0) || (timeout != 0 && period < 0), "Invalid timeout/period");

  if (hOwnerOut != hOwner)
    return;

  PWaitAndSignal mutexWait(Mutex);

  if (hOwnerOut != hOwner)
    return;

  preparePacketTimeout = timeout;
  preparePacketPeriod = period;

  if (preparePacketPeriod > 0)
    preparePacketDelay.Restart();
}
///////////////////////////////////////////////////////////////
int T38Engine::PreparePacket(HOWNEROUT hOwner, T38_IFP & ifp)
{
  if (hOwnerOut != hOwner || !IsModemOpen())
    return 0;

  PWaitAndSignal mutexWait(MutexOut);

  if (hOwnerOut != hOwner || !IsModemOpen())
    return 0;

  {
    PWaitAndSignal mutexWait(Mutex);

    if (hOwnerOut != hOwner || !IsModemOpen())
      return 0;

    if (firstOut) {
      firstOut = FALSE;
      ModemCallbackWithUnlock(cbpUpdateState);

      if (hOwnerOut != hOwner || !IsModemOpen())
        return FALSE;

      preparePacketDelay.Restart();
    }
  }

  //myPTRACE(1, "T38Modem\t" << name << " PreparePacket begin stM=" << stateModem << " stO=" << stateOut);

  ifp = T38_IFP();
  PBoolean doDelay = TRUE;
  PTime preparePacketTimeoutEnd = (preparePacketTimeout > 0 ? (PTime() + preparePacketTimeout) : PTime(0));

  if (preparePacketPeriod > 0) {
    preparePacketDelay.Delay(preparePacketPeriod);

    if (hOwnerOut != hOwner || !IsModemOpen())
      return 0;
  }

  for(;;) {
    PBoolean redo = FALSE;

    if (doDelay) {
      //myPTRACE(1, "T38Modem\t" << name << " +++++ stM=" << stateModem << " stO=" << stateOut << " "
      //       << timeDelayEndOut.AsString("hh:mm:ss.uuu\t", PTime::Local));

      for (;;) {
        PTimeInterval delay = timeDelayEndOut - PTime();

        if (delay.GetMilliSeconds() <= 0)
          break;

        if (preparePacketTimeout >= 0) {
          if (preparePacketTimeout == 0)
            return -1;

          PTimeInterval timeout = preparePacketTimeoutEnd - PTime();

          if (timeout.GetMilliSeconds() <= 0)
            return -1;

          if (delay.GetMilliSeconds() > timeout.GetMilliSeconds())
            delay = timeout;
        }

        if (delay.GetMilliSeconds() > msMaxOutDelay)
          delay = msMaxOutDelay;

        mySleep(delay.GetMilliSeconds());

        if (hOwnerOut != hOwner || !IsModemOpen())
          return 0;
      }
    } else {
      doDelay = TRUE;
    }

    if (hOwnerOut != hOwner || !IsModemOpen())
      return 0;

    for(;;) {
      PBoolean waitData = FALSE;
      {
        PWaitAndSignal mutexWait(Mutex);

        if (hOwnerOut != hOwner || !IsModemOpen())
          return 0;

        if (isStateModemOut() || stateOut != stOutIdle) {
          switch (stateOut) {
            case stOutIdle:
              if (delaySignalOut) {
                if (ModParsOut.dataType != dtSilence && timeBeginOut > PTime()) {
                  redo = TRUE;
                  myPTRACE(4, "T38Modem\t" << name << " PreparePacket delaySignalOut");
                  break;
                }

                delaySignalOut = FALSE;
              }

              if (isCarrierIn) {
                myPTRACE(3, "T38Modem\t" << name << " PreparePacket isCarrierIn=" << isCarrierIn
                                << " for dataType=" << ModParsOut.dataType);

                /*
                 * We can't to begin sending data while the carrier is detected because
                 * it's possible that all data (including indication) will be losted.
                 * It's too critical for image data because it's possible to receive
                 * MCF generated for previous page after sending small page that was
                 * not delivered.
                 */

                int waitms;

                switch (ModParsOut.dataType) {
                  case dtHdlc:      waitms = 500;     break; // it's can't be too long
                  case dtRaw:       waitms = 2000;    break; // it's can't be too short
                  default:          waitms = 0;       break;
                }

                if (waitms) {
                  if (isCarrierIn == 1) {
                    isCarrierIn = 2;
                    timeBeginOut = PTime() + PTimeInterval(waitms);
                    redo = TRUE;
                    break;
                  } else if (timeBeginOut > PTime()) {
                    redo = TRUE;
                    break;
                  } else {
                    myPTRACE(1, "T38Modem\t" << name << " PreparePacket isCarrierIn expired");
                    isCarrierIn = 0;
                  }
                }
              }

              switch (ModParsOut.dataTypeT38) {
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
                  myPTRACE(1, "T38Modem\t" << name << " PreparePacket bad dataTypeT38=" << ModParsOut.dataTypeT38);
                  return 0;
              }
              break;
            ////////////////////////////////////////////////////
            case stOutCedWait:
              stateOut = stOutNoSig;
              stateModem = stmIdle;
              ModemCallbackWithUnlock(callbackParamOut);

              if (hOwnerOut != hOwner || !IsModemOpen())
                return 0;

              redo = TRUE;
              break;
            ////////////////////////////////////////////////////
            case stOutSilenceWait:
              stateOut = stOutIdle;
              stateModem = stmIdle;
              ModemCallbackWithUnlock(callbackParamOut);

              if (hOwnerOut != hOwner || !IsModemOpen())
                return 0;

              doDelay = FALSE;
              redo = TRUE;
              break;
            ////////////////////////////////////////////////////
            case stOutIndWait:
              stateOut = stOutData;
              countOut = 0;
              startedTimeOutBufEmpty = FALSE;
              timeBeginOut = PTime();
              hdlcOut = HDLC();
              if (ModParsOut.msgType == T38D(e_v21))
                t30.v21Begin();

              switch (ModParsOut.dataType) {
                case dtHdlc:
                  hdlcOut.PutHdlcData(&bufOut);
                  break;
                case dtRaw:
                  hdlcOut.PutRawData(&bufOut);
                  break;
                default:
                  myPTRACE(1, "T38Modem\t" << name << " PreparePacket bad dataType=" << ModParsOut.dataType);
                  return 0;
              }

              switch (ModParsOut.dataTypeT38) {
                case dtHdlc:
                  hdlcOut.GetHdlcStart(TRUE);
                  break;
                case dtRaw:
                  if (ModParsOut.dataType == dtHdlc) {
                    myPTRACE(1, "T38Modem\t" << name << " PreparePacket sending dtHdlc like dtRaw not implemented");
                    return 0;
                  }
                  hdlcOut.GetRawStart();
                  break;
                default:
                  myPTRACE(1, "T38Modem\t" << name << " PreparePacket bad dataTypeT38=" << ModParsOut.dataTypeT38);
                  return 0;
              }

              redo = TRUE;
              break;
            ////////////////////////////////////////////////////
            case stOutData:
              {
                BYTE b[(msPerOut * 14400)/(8*1000)];
                PINDEX len = (msPerOut * ModParsOut.br)/(8*1000);
                if (len > PINDEX(sizeof(b)))
                  len = sizeof(b);
                PBoolean wasFull = bufOut.isFull();
                int count = hdlcOut.GetData(b, len);
                if (wasFull && !bufOut.isFull()) {
                  ModemCallbackWithUnlock(cbpOutBufNoFull);

                  if (hOwnerOut != hOwner || !IsModemOpen())
                    return 0;
                }

                switch( count ) {
                  case -1:
                    startedTimeOutBufEmpty = FALSE;

                    switch (ModParsOut.dataTypeT38) {
                      case dtHdlc:
                        stateOut = stOutHdlcFcs;
                        break;
                      case dtRaw:
                        stateOut = stOutDataNoSig;
                        break;
                      default:
                        myPTRACE(1, "T38Modem\t" << name << " PreparePacket stOutData bad dataTypeT38="
                            << ModParsOut.dataTypeT38);
                        return 0;
                    }
                    redo = TRUE;
                    break;
                  case 0:
                    if (hdlcOut.getLastChar() != -1 &&
                        (ModParsOut.dataType == dtHdlc || hdlcOut.getLastChar() != 0))
                    {
                      ModemCallbackWithUnlock(cbpOutBufEmpty);

                      if (hOwnerOut != hOwner || !IsModemOpen())
                        return 0;
                    }
                    else
                    if (!startedTimeOutBufEmpty) {
                      timeOutBufEmpty = PTime() + PTimeInterval(5000);
                      startedTimeOutBufEmpty = TRUE;
                    }
                    else
                    if (timeOutBufEmpty <= PTime()) {
                      ModemCallbackWithUnlock(cbpOutBufEmpty);

                      if (hOwnerOut != hOwner || !IsModemOpen())
                        return 0;
                    }
                    waitData = TRUE;
                    break;
                  default:
                    startedTimeOutBufEmpty = FALSE;

                    switch (ModParsOut.dataTypeT38) {
                      case dtHdlc:
                        if (ModParsOut.msgType == T38D(e_v21)) {
                          t30.v21Data(b, count);
                        }
                        t38data(ifp, ModParsOut.msgType, T38F(e_hdlc_data), PBYTEArray(b, count));
                        break;
                      case dtRaw:
                        t38data(ifp, ModParsOut.msgType, T38F(e_t4_non_ecm_data), PBYTEArray(b, count));
                        break;
                      default:
                        myPTRACE(1, "T38Modem\t" << name << " PreparePacket stOutData bad dataTypeT38="
                            << ModParsOut.dataTypeT38);
                        return 0;
                    }
                    countOut += count;
                }
              }
              break;
            ////////////////////////////////////////////////////
            case stOutHdlcFcs:
              if (ModParsOut.msgType == T38D(e_v21)) {
                t30.v21End(TRUE);
                t30.v21Begin();
              }

              if (ModParsOut.dataType == dtRaw) {
                PBoolean wasFull = bufOut.isFull();

                if (countOut)
                  t38data(ifp, ModParsOut.msgType, hdlcOut.isFcsOK() ? T38F(e_hdlc_fcs_OK) : T38F(e_hdlc_fcs_BAD));
                else
                  redo = TRUE;

                hdlcOut.GetHdlcStart(FALSE);
                countOut = 0;

                if (hdlcOut.GetData(NULL, 0) != -1)
                  stateOut = stOutData;
                else
                  stateOut = stOutDataNoSig;

                if (wasFull && !bufOut.isFull()) {
                  ModemCallbackWithUnlock(cbpOutBufNoFull);

                  if (hOwnerOut != hOwner || !IsModemOpen())
                    return 0;
                }
              } else {
                if( stateModem != stmOutNoMoreData ) {
                  myPTRACE(1, "T38Modem\t" << name << " PreparePacket stOutHdlcFcs stateModem("
                      << stateModem << ") != stmOutNoMoreData");
                  return 0;
                }

                if (countOut)
                  t38data(ifp, ModParsOut.msgType, T38F(e_hdlc_fcs_OK));
                else
                  redo = TRUE;

                countOut = 0;
                bufOut.Clean();		// reset eof
                hdlcOut.PutHdlcData(&bufOut);
                hdlcOut.GetHdlcStart(FALSE);
                if (moreFramesOut) {
                  stateOut = stOutData;
                  stateModem = stmOutMoreData;
                  ModemCallbackWithUnlock(callbackParamOut);

                  if (hOwnerOut != hOwner || !IsModemOpen())
                    return 0;
                } else {
                  stateOut = stOutDataNoSig;
                }
              }
              break;
            ////////////////////////////////////////////////////
            case stOutDataNoSig:
#if PTRACING
              if (myCanTrace(3) || (myCanTrace(2) && ModParsOut.dataType == dtRaw)) {
                PInt64 msTime = (PTime() - timeBeginOut).GetMilliSeconds();
                myPTRACE(2, "T38Modem\t" << name << " Sent " << hdlcOut.getRawCount() << " bytes in " << msTime << " ms ("
                  << (PInt64(hdlcOut.getRawCount()) * 8 * 1000)/(msTime ? msTime : 1) << " bits/s)");
              }
#endif
              if( stateModem != stmOutNoMoreData ) {
                myPTRACE(1, "T38Modem\t" << name << " PreparePacket stOutDataNoSig stateModem("
                     << stateModem << ") != stmOutNoMoreData");
                return 0;
              }
              switch (ModParsOut.dataTypeT38) {
                case dtHdlc:
                  t38data(ifp, ModParsOut.msgType, T38F(e_hdlc_sig_end));
                  break;
                case dtRaw:
                  t38data(ifp, ModParsOut.msgType, T38F(e_t4_non_ecm_sig_end));
                  break;
                default:
                  myPTRACE(1, "T38Modem\t" << name << " PreparePacket stOutDataNoSig bad dataTypeT38="
                      << ModParsOut.dataTypeT38);
                  return 0;
              }
              stateOut = stOutNoSig;
              stateModem = stmIdle;
              ModemCallbackWithUnlock(callbackParamOut);

              if (hOwnerOut != hOwner || !IsModemOpen())
                return 0;

              break;
            ////////////////////////////////////////////////////
            case stOutNoSig:
              t38indicator(ifp, T38I(e_no_signal));
              stateOut = stOutIdle;
              delaySignalOut = TRUE;
              timeBeginOut = PTime() + PTimeInterval(75);
              break;
            default:
              myPTRACE(1, "T38Modem\t" << name << " PreparePacket bad stateOut=" << stateOut);
              return 0;
          }
        } else {
          switch (onIdleOut) {
            case dtCng:
              t38indicator(ifp, T38I(e_cng));
              break;
            default:
              myPTRACE(1, "T38Modem\t" << name << " SendOnIdle dataType(" << onIdleOut << ") is not supported");
            case dtNone:
              waitData = TRUE;
          }
          onIdleOut = dtNone;
        }
      }

      if (!waitData)
        break;

      if (preparePacketTimeout >= 0) {
        if (preparePacketTimeout == 0)
          return -1;

        PTimeInterval timeout = preparePacketTimeoutEnd - PTime();

        if (timeout.GetMilliSeconds() <= 0 || !WaitOutDataReady(timeout.GetMilliSeconds()))
          return -1;
      } else {
        if (startedTimeOutBufEmpty) {
          PInt64 timeout = (timeOutBufEmpty - PTime()).GetMilliSeconds() + 1;

          if (timeout > 0)
            WaitOutDataReady(timeout);
        } else {
          WaitOutDataReady();
        }
      }

      if (hOwnerOut != hOwner || !IsModemOpen())
        return 0;

      {
        PWaitAndSignal mutexWait(Mutex);

        if (hOwnerOut != hOwner || !IsModemOpen())
          return 0;

        if (stateOut == stOutData) {
#if PTRACING
          if (myCanTrace(3) || (myCanTrace(2) && ModParsOut.dataType == dtRaw)) {
            PInt64 msTime = (PTime() - timeBeginOut).GetMilliSeconds();
            myPTRACE(2, "T38Modem\t" << name << " Sent " << hdlcOut.getRawCount() << " bytes in " << msTime << " ms ("
              << (PInt64(hdlcOut.getRawCount()) * 8 * 1000)/(msTime ? msTime : 1) << " bits/s)");
          }
#endif
          myPTRACE(1, "T38Modem\t" << name << " PreparePacket DTE's data delay, reset " << hdlcOut.getRawCount());
          hdlcOut.resetRawCount();
          timeBeginOut = PTime() - PTimeInterval(msPerOut);
          doDelay = FALSE;
        }
      }
    }

    switch (stateOut) {
      case stOutIdle:          timeDelayEndOut = PTime() + msPerOut; break;
      case stOutCedWait:       timeDelayEndOut = PTime() + ModParsOut.lenInd; break;
      case stOutSilenceWait:   timeDelayEndOut = PTime() + ModParsOut.lenInd; break;
      case stOutIndWait:       timeDelayEndOut = PTime() + ModParsOut.lenInd; break;
      case stOutData:
      case stOutHdlcFcs:
        timeDelayEndOut = timeBeginOut + (PInt64(hdlcOut.getRawCount()) * 8 * 1000)/ModParsOut.br + msPerOut;
        break;
      case stOutDataNoSig:     timeDelayEndOut = PTime() + msPerOut; break;
      case stOutNoSig:         timeDelayEndOut = PTime() + msPerOut; break;
      default:                 timeDelayEndOut = PTime();
    }

    if (!redo)
      break;
  }

  return 1;
}
///////////////////////////////////////////////////////////////
PBoolean T38Engine::HandlePacketLost(HOWNERIN hOwner, unsigned myPTRACE_PARAM(nLost))
{
  myPTRACE(1, "T38Modem\t" << name << " HandlePacketLost " << nLost);

  if (hOwnerIn != hOwner || !IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexWait(Mutex);

  if (hOwnerIn != hOwner || !IsModemOpen())
    return FALSE;

  ModStream *modStream = modStreamIn;

  if( modStream == NULL || modStream->lastBuf == NULL ) {
    modStream = modStreamInSaved;
  }

  if( !(modStream == NULL || modStream->lastBuf == NULL) ) {
    // LXK Fix: Add check for hdlc so lost high speed hdlc data packets handled as well as v.21
    if(( modStream->ModPars.msgType == T38D(e_v21)) || (modStream->ModPars.dataType == dtHdlc)) {
      modStream->SetDiag(diagBadFcs);
    }
  }
  return TRUE;
}
///////////////////////////////////////////////////////////////
PBoolean T38Engine::HandlePacket(HOWNERIN hOwner, const T38_IFP & ifp)
{
#if PTRACING
  if (PTrace::CanTrace(3)) {
    myPTRACE(3, "T38Modem\t" << name << " HandlePacket Received ifp\n  "
             << setprecision(2) << ifp);
  }
  else {
    myPTRACE(2, "T38Modem\t" << name << " HandlePacket Received ifp type=" << ifp.m_type_of_msg.GetTagName());
  }
#endif

  if (hOwnerIn != hOwner || !IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexWait(Mutex);

  if (hOwnerIn != hOwner || !IsModemOpen())
    return FALSE;

  switch (ifp.m_type_of_msg.GetTag()) {
    case T38_Type_of_msg::e_t30_indicator: {
      T38_Type_of_msg_t30_indicator type_of_msg = ifp.m_type_of_msg;

      if (((modStreamIn != NULL) && (modStreamIn->lastBuf != NULL &&
            modStreamIn->ModPars.ind == type_of_msg)) ||
          ((modStreamInSaved != NULL) && (modStreamInSaved->lastBuf != NULL &&
            modStreamInSaved->ModPars.ind == type_of_msg)))
      {
        myPTRACE(3, "T38Modem\t" << name << " HandlePacket ignored repeated indicator " << type_of_msg);
        break;
      }

      if (modStreamIn != NULL && modStreamIn->lastBuf != NULL) {
        myPTRACE(1, "T38Modem\t" << name << " HandlePacket indicator && modStreamIn->lastBuf != NULL");

        if (firstIn && countIn == 0 && type_of_msg == T38I(e_no_signal)) {
          myPTRACE(1, "T38Modem\t" << name << " HandlePacket ignored first indicator " << type_of_msg);
          break;
        } else {
          modStreamIn->PutEof(diagOutOfOrder | diagNoCarrier);
          myPTRACE(1, "T38Modem\t" << name << " HandlePacket out of order " << type_of_msg);

          if (stateModem == stmInRecvData) {
            ModemCallbackWithUnlock(callbackParamIn);

            if (hOwnerIn != hOwner || !IsModemOpen())
              return FALSE;
          }
        }
      }

      if (modStreamInSaved != NULL) {
        myPTRACE(1, "T38Modem\t" << name << " HandlePacket indicator && modStreamInSaved != NULL, clean");
        delete modStreamInSaved;
        modStreamInSaved = NULL;
      }

      switch (type_of_msg) {
        case T38I(e_no_signal):
          isCarrierIn = 0;

          if (stateModem == stmInWaitSilence) {
            stateModem = stmIdle;
            ModemCallbackWithUnlock(callbackParamIn);

            if (hOwnerIn != hOwner || !IsModemOpen())
              return FALSE;
          }
          break;
        case T38I(e_ced):
          OnUserInput('a');
          isCarrierIn = 0;

          if (stateModem == stmInWaitSilence) {
            stateModem = stmIdle;
            ModemCallbackWithUnlock(callbackParamIn);

            if (hOwnerIn != hOwner || !IsModemOpen())
              return FALSE;
          }
          break;
        case T38I(e_cng):
          OnUserInput('c');
          isCarrierIn = 0;

          if (stateModem == stmInWaitSilence) {
            stateModem = stmIdle;
            ModemCallbackWithUnlock(callbackParamIn);

            if (hOwnerIn != hOwner || !IsModemOpen())
              return FALSE;
          }
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
          modStreamInSaved = new ModStream(GetModPars(type_of_msg, by_ind));
          modStreamInSaved->PushBuf();
          countIn = 0;

          if (stateModem == stmInWaitSilence) {
            stateModem = stmIdle;
            ModemCallbackWithUnlock(callbackParamIn);

            if (hOwnerIn != hOwner || !IsModemOpen())
              return FALSE;
          }
          else
          if (stateModem == stmInWaitData) {
            if (modStreamIn != NULL) {
              if (modStreamIn->ModPars.IsEqual(modStreamInSaved->ModPars)) {
                modStreamIn->Move(*modStreamInSaved);
                delete modStreamInSaved;
                modStreamInSaved = NULL;
              } else {
                myPTRACE(2, "T38Modem\t" << name << "  T38Engine::HandlePacket modStreamIn->ModPars("
                  << modStreamIn->ModPars.val
                  << ") != modStreamInSaved->ModPars("
                  << modStreamInSaved->ModPars.val
                  << ")");
                modStreamIn->PushBuf();
                modStreamIn->PutEof(diagDiffSig);
              }
            } else {
              myPTRACE(1, "T38Modem\t" << name << " HandlePacket modStreamIn == NULL");
            }
            stateModem = stmInReadyData;
            ModemCallbackWithUnlock(callbackParamIn);

            if (hOwnerIn != hOwner || !IsModemOpen())
              return FALSE;
          }
          break;
        default:
          myPTRACE(1, "T38Modem\t" << name << " HandlePacket type_of_msg is bad !!! " << setprecision(2) << ifp);
      }
      break;
    }
    case T38_Type_of_msg::e_data: {
        unsigned type_of_msg = (T38_Type_of_msg_data)ifp.m_type_of_msg;
        ModStream *modStream = modStreamIn;

        if (modStream == NULL || modStream->lastBuf == NULL)
          modStream = modStreamInSaved;

        if (modStream == NULL || modStream->lastBuf == NULL) {
          myPTRACE(1, "T38Modem\t" << name << " HandlePacket lastBuf == NULL");
          modStream = NULL;
        }
        else
        if (modStream->ModPars.msgType != type_of_msg) {
          myPTRACE(1, "T38Modem\t" << name << " HandlePacket modStream->ModPars.msgType("
              << modStream->ModPars.msgType << ") != type_of_msg(" << type_of_msg << ")");
          modStream->PutEof(diagOutOfOrder | diagNoCarrier);
          modStream = NULL;
        }

        if (ifp.HasOptionalField(T38_IFPPacket::e_data_field)) {
          PINDEX count = ifp.m_data_field.GetSize();
          for (PINDEX i = 0 ; i < count ; i++) {
                PTRACE_IF(4, modStream == NULL, "T38Modem\t" << name << " HandlePacket modStream == NULL");

                const T38_DATA_FIELD &Data_Field = ifp.m_data_field[i];

                switch (Data_Field.m_field_type) {  // Handle data
                  case T38F(e_hdlc_data):
                  case T38F(e_t4_non_ecm_data):
                  case T38F(e_hdlc_sig_end):
                  case T38F(e_hdlc_fcs_OK):
                  case T38F(e_hdlc_fcs_BAD):
                  case T38F(e_hdlc_fcs_OK_sig_end):
                  case T38F(e_hdlc_fcs_BAD_sig_end):
                  case T38F(e_t4_non_ecm_sig_end):
                    if (Data_Field.HasOptionalField(T38_Data_Field_subtype::e_field_data)) {
                      int size = Data_Field.m_field_data.GetSize();
                      if(modStream != NULL)
                        modStream->PutData(Data_Field.m_field_data, size);
#if PTRACING
                      if (!countIn)
                        timeBeginIn = PTime();
#endif
                      countIn += size;
                    }
                    break;
                  default:
                    myPTRACE(1, "T38Modem\t" << name << " HandlePacket field_type bad !!! " << setprecision(2) << ifp);
                }

                switch (Data_Field.m_field_type) {  // Handle fcs
                  case T38F(e_hdlc_fcs_BAD):
                  case T38F(e_hdlc_fcs_BAD_sig_end):
                    if(modStream != NULL)
                      modStream->SetDiag(diagBadFcs);
                    myPTRACE(1, "T38Modem\t" << name << " HandlePacket bad FCS");
                  case T38F(e_hdlc_fcs_OK):
                  case T38F(e_hdlc_fcs_OK_sig_end):
                    if(modStream != NULL) {
                      modStream->PutEof();
                      modStream->PushBuf();
                    }
                    break;
                }
                switch( Data_Field.m_field_type ) {	// Handle sig_end
                  case T38F(e_t4_non_ecm_sig_end):
#if PTRACING
                    if (myCanTrace(2)) {
                      PInt64 msTime = (PTime() - timeBeginIn).GetMilliSeconds();
                      myPTRACE(2, "T38Modem\t" << name << " Received " << countIn << " bytes in " << msTime << " ms ("
                        << (PInt64(countIn) * 8 * 1000)/(msTime ? msTime : 1) << " bits/s)");
                    }
#endif
                  case T38F(e_hdlc_fcs_OK_sig_end):
                  case T38F(e_hdlc_fcs_BAD_sig_end):
                  case T38F(e_hdlc_sig_end):
                    if(modStream != NULL) {
                      modStream->PutEof(diagNoCarrier);
                      modStream = NULL;
                    }

                    isCarrierIn = 0;

                    if (stateModem == stmInWaitSilence) {
                      stateModem = stmIdle;
                      ModemCallbackWithUnlock(callbackParamIn);

                      if (hOwnerIn != hOwner || !IsModemOpen())
                        return FALSE;
                    }
                    break;
                }
          }
        }

        if (stateModem == stmInRecvData) {
          ModemCallbackWithUnlock(callbackParamIn);

          if (hOwnerIn != hOwner || !IsModemOpen())
            return FALSE;
        }

        break;
    }
    default:
      myPTRACE(1, "T38Modem\t" << name << " HandlePacket Tag is bad !!! " << setprecision(2) << ifp);
  }

  if (!firstIn) {
    firstIn = FALSE;
    ModemCallbackWithUnlock(cbpUpdateState);

    if (hOwnerIn != hOwner || !IsModemOpen())
      return FALSE;
  }

  return TRUE;
}
///////////////////////////////////////////////////////////////

