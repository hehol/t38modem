/*
 * t38engine.h
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
 * $Log: t38engine.h,v $
 * Revision 1.37  2010-03-18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.37  2010/03/18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.36  2009/12/02 09:06:42  vfrolov
 * Added a short delay after transmitting of signal before call clearing
 *
 * Revision 1.35  2009/11/26 07:21:37  vfrolov
 * Added delay between transmitting of signals
 *
 * Revision 1.34  2009/11/19 14:48:28  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.33  2009/11/18 19:08:47  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.32  2009/11/10 08:13:38  vfrolov
 * Fixed race condition on re-opening T38Engine
 *
 * Revision 1.31  2009/11/06 10:19:29  vfrolov
 * Fixed indication handling after re-opening T38Engine
 *
 * Revision 1.30  2009/10/27 18:53:49  vfrolov
 * Added ability to re-open T38Engine
 * Added ability to prepare IFP packets with adaptive delay/period
 *
 * Revision 1.29  2009/07/27 16:21:24  vfrolov
 * Moved h323lib specific code to h323lib directory
 *
 * Revision 1.28  2008/09/24 14:51:45  frolov
 * Added 5 sec. timeout  for DCE's transmit buffer empty
 *
 * Revision 1.27  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.26  2007/05/10 10:40:33  vfrolov
 * Added ability to continuously resend last UDPTL packet
 *
 * Revision 1.25  2007/05/03 09:21:47  vfrolov
 * Added compile time optimization for original ASN.1 sequence
 * in T.38 (06/98) Annex A or for CORRIGENDUM No. 1 fix
 *
 * Revision 1.24  2007/04/11 14:19:15  vfrolov
 * Changed for OPAL
 *
 * Revision 1.23  2007/03/23 10:14:36  vfrolov
 * Implemented voice mode functionality
 *
 * Revision 1.22  2006/12/22 12:51:00  vfrolov
 * Fixed class of MutexModemCallback
 * Added volatile to isCarrierIn
 * Removed stateIn
 *
 * Revision 1.21  2006/12/11 11:19:48  vfrolov
 * Fixed race condition with modem Callback
 *
 * Revision 1.20  2005/07/20 12:42:36  vfrolov
 * Done less strict comparison in MODPARS::IsEqual()
 *
 * Revision 1.19  2005/07/18 11:39:48  vfrolov
 * Changed for OPAL
 *
 * Revision 1.18  2004/06/18 15:06:29  vfrolov
 * Fixed race condition by adding mutex for modemCallback
 *
 * Revision 1.17  2004/03/01 17:10:39  vfrolov
 * Fixed duplicated mutexes
 * Added volatile to T38Mode
 *
 * Revision 1.16  2003/12/04 16:10:05  vfrolov
 * Implemented FCS generation
 * Implemented ECM support
 *
 * Revision 1.15  2003/01/08 16:46:28  vfrolov
 * Added cbpOutBufNoFull and isOutBufFull()
 * Added data speed tracing
 *
 * Revision 1.14  2002/12/19 11:54:47  vfrolov
 * Removed DecodeIFPPacket() and utilized HandleRawIFP()
 *
 * Revision 1.13  2002/11/28 09:17:36  vfrolov
 * Added missing const
 *
 * Revision 1.12  2002/11/18 22:57:53  craigs
 * Added patches from Vyacheslav Frolov for CORRIGENDUM
 *
 * Revision 1.11  2002/05/22 12:01:50  vfrolov
 * Implemented redundancy error protection scheme
 *
 * Revision 1.10  2002/05/07 11:06:18  vfrolov
 * Discarded const from ModemCallbackWithUnlock()
 *
 * Revision 1.9  2002/05/07 10:15:44  vfrolov
 * Fixed dead lock on modemCallback
 *
 * Revision 1.8  2002/04/19 14:29:40  vfrolov
 * Added Copyright header
 *
 * Revision 1.7  2002/04/19 13:59:04  vfrolov
 * Added SendOnIdle()
 *
 * Revision 1.6  2002/02/11 16:46:21  vfrolov
 * Discarded transport arg from Originate() and Answer()
 * Thanks to Christopher Curtis
 *
 * Revision 1.5  2002/01/10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.4  2002/01/06 03:48:45  craigs
 * Added changes to support efax 0.9
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.3  2002/01/03 21:36:42  craigs
 * Added additional logic to work with efax
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.2  2002/01/01 23:59:52  craigs
 * Lots of additional implementation thanks to Vyacheslav Frolov
 *
 */

#ifndef _T38ENGINE_H
#define _T38ENGINE_H

#include "pmutils.h"
#include <ptclib/delaychan.h>
#include "hdlc.h"
#include "t30.h"
#include "enginebase.h"

///////////////////////////////////////////////////////////////
class MODPARS
{
  public:
    MODPARS(
          int _val = -1,
          unsigned _ind = unsigned(-1),
          int _lenInd = -1,
          unsigned _msgType = unsigned(-1),
          int _br = -1
    );

    PBoolean IsModValid() const { return val >= 0; }
    PBoolean IsEqual(const MODPARS &mp) const { return msgType == mp.msgType; }

    EngineBase::DataType dataType;
    EngineBase::DataType dataTypeT38;
    int val;
    unsigned ind;
    int lenInd;
    unsigned msgType;
    int br;
};
///////////////////////////////////////////////////////////////
#ifdef OPTIMIZE_CORRIGENDUM_IFP
  #define T38_IFP       T38_IFPPacket
  #define T38_IFP_NAME  "IFP"
#else
  #define T38_IFP       T38_PreCorrigendum_IFPPacket
  #define T38_IFP_NAME  "Pre-corrigendum IFP"
#endif
///////////////////////////////////////////////////////////////
class ModStream;
class T38_IFP;

class T38Engine : public EngineBase
{
  PCLASSINFO(T38Engine, EngineBase);

  public:

  /**@name Construction */
  //@{
    T38Engine(const PString &_name = "");
    ~T38Engine();
  //@}

  /**@name Modem API */
  //@{
    void ResetModemState();
    PBoolean isOutBufFull() const;

    void SendOnIdle(DataType _dataType);
    PBoolean SendStart(DataType _dataType, int param);
    int Send(const void *pBuf, PINDEX count);
    PBoolean SendStop(PBoolean moreFrames, int _callbackParam);

    PBoolean RecvWait(DataType _dataType, int param, int _callbackParam, PBoolean &done);
    PBoolean RecvStart(int _callbackParam);
    int Recv(void *pBuf, PINDEX count);
    int RecvDiag();
    void RecvStop();

    PBoolean SendingNotCompleted() const;
  //@}

    void Close() { CloseIn(); CloseOut(); }

    /**Prepare outgoing T.38 packet.

       If returns  0, then the writing loop should be terminated.
       If returns >0, then the ifp packet is correct and should be sent.
       If returns <0, then the ifp packet is not correct (timeout).
      */
    int PreparePacket(T38_IFP & ifp);

    void SetPreparePacketTimeout(int timeout, int period = -1) {
      if (timeout == 0 && period == -1)
        period = 20;

      preparePacketTimeout = timeout;
      preparePacketPeriod = period;

      if (preparePacketPeriod > 0)
        preparePacketDelay.Restart();
    }

    /**Handle incoming T.38 packet.

       If returns FALSE, then the reading loop should be terminated.
      */
    PBoolean HandlePacket(
      const T38_IFP & ifp
    );

    /**Handle lost T.38 packets.

       If returns FALSE, then the reading loop should be terminated.
      */
    PBoolean HandlePacketLost(
      unsigned nLost
    );

  protected:

    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnChangeModemClass();
    virtual void OnOpenIn();
    virtual void OnOpenOut();
    virtual void OnCloseIn();
    virtual void OnCloseOut();

    enum { msPerOut = 30 };

  private:
    void SignalOutDataReady() { outDataReadySyncPoint.Signal(); }
    void WaitOutDataReady() { outDataReadySyncPoint.Wait(); }
    PBoolean WaitOutDataReady(const PTimeInterval & timeout) {
      return outDataReadySyncPoint.Wait(timeout);
    }

    void _ResetModemState();

  private:
    DataStream bufOut;

    int preparePacketTimeout;
    int preparePacketPeriod;

    PAdaptiveDelay preparePacketDelay;

    int stateOut;
    DataType onIdleOut;
    int callbackParamOut;
    MODPARS ModParsOut;
    PBoolean delaySignalOut;
    PBoolean startedTimeOutBufEmpty;
    PTime timeOutBufEmpty;
    PTime timeDelayEndOut;
    PTime timeBeginOut;
    PINDEX countOut;
    PBoolean moreFramesOut;
    HDLC hdlcOut;

    int callbackParamIn;
    volatile int isCarrierIn;
#if PTRACING
    PTime timeBeginIn;
#endif
    PINDEX countIn;
    PBoolean firstIn;

    T30 t30;

    ModStream *modStreamIn;
    ModStream *modStreamInSaved;

    volatile int stateModem;

    PSyncPoint outDataReadySyncPoint;
};
///////////////////////////////////////////////////////////////

#endif  // _T38ENGINE_H

