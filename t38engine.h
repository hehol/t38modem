/*
 * t38engine.h
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
 * $Log: t38engine.h,v $
 * Revision 1.13  2002-11-28 09:17:36  vfrolov
 * Added missing const
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
#include <t38proto.h>

///////////////////////////////////////////////////////////////
class MODPARS
{
  public:
    MODPARS(
          int _dataType = -1,
          int _val = -1, 
          unsigned _ind = unsigned(-1), 
          int _lenInd = -1, 
          unsigned _msgType = unsigned(-1), 
          int _br = -1
    ) : dataType(_dataType), val(_val), ind(_ind), lenInd(_lenInd),
        msgType(_msgType), br(_br) {}
    
    BOOL IsModValid() { return val >= 0; }

    int dataType;
    int val;
    unsigned ind;
    int lenInd;
    unsigned msgType;
    int br;
};
///////////////////////////////////////////////////////////////

class PASN_OctetString;
class ModStream;

class T38Engine : public OpalT38Protocol
{
  PCLASSINFO(T38Engine, OpalT38Protocol);
  public:
    
    enum {
      diagOutOfOrder	= 0x01,
      diagLost		= 0x02,
      diagDiffSig	= 0x04,	// a different signal is detected
      diagBadFcs	= 0x08,
      diagNoCarrier	= 0x10,
      diagError		= 0x80,	// bad usage
    };

    enum {
      dtNone,
      dtCed,
      dtCng,
      dtSilence,
      dtHdlc,
      dtRaw,
    };

    enum {
      cbpUserDataMod	= 255,
      cbpReset		= -1,
      cbpOutBufEmpty	= -2,
    };

  /**@name Construction */
  //@{
    T38Engine(const PString &_name = "");
    ~T38Engine();
  //@}
  
  /**@name Operations */
  //@{
    void CleanUpOnTermination();
    void SetT38Mode(BOOL mode = TRUE);
    void SetRedundancy(
      int indication,
      int low_speed,
      int high_speed
    );
    /**The calling SetOldASN() is aquivalent to the following change of the t38.asn:

           -  t4-non-ecm-sig-end,
           -   ...
           +  t4-non-ecm-sig-end
     */
    void SetOldASN() { old_asn = TRUE; }
  //@}
  
  /**@name Modem API */
  //@{
    BOOL Attach(const PNotifier &callback);
    void Detach(const PNotifier &callback);
    void ResetModemState();

    void SendOnIdle(int _dataType);
    BOOL SendStart(int _dataType, int param);
    int Send(const void *pBuf, PINDEX count);
    BOOL SendStop(BOOL moreFrames, int _callbackParam);

    BOOL RecvWait(int _dataType, int param, int _callbackParam);
    BOOL RecvStart(int _callbackParam);
    int Recv(void *pBuf, PINDEX count);
    int RecvDiag();
    BOOL RecvStop();
  //@}
    
  protected:
  
    void EncodeIFPPacket(PASN_OctetString &ifp_packet, const T38_IFPPacket &T38_ifp) const;
    BOOL DecodeIFPPacket(const PASN_OctetString &ifp_packet, T38_IFPPacket &T38_ifp) const;

    BOOL Originate();
    BOOL Answer();

    /**Prepare outgoing T.38 packet.

       If returns  0, then the writing loop should be terminated.
       If returns >0, then the ifp packet is correct and should be sent.
       If returns <0, then the ifp packet is not correct.
      */
    int PreparePacket(
      T38_IFPPacket & ifp,
      BOOL enableTimeout
    );

    /**Handle incoming T.38 packet.

       If returns FALSE, then the reading loop should be terminated.
      */
    BOOL HandlePacket(
      const T38_IFPPacket & ifp
    );

    /**Handle lost T.38 packets.

       If returns FALSE, then the reading loop should be terminated.
      */
    BOOL HandlePacketLost(
      unsigned nLost
    );
    
    int in_redundancy;
    int ls_redundancy;
    int hs_redundancy;
    BOOL old_asn;
    
  private:
  
    void SignalOutDataReady() { outDataReadySyncPoint.Signal(); }
    void WaitOutDataReady() { outDataReadySyncPoint.Wait(); }
    BOOL WaitOutDataReady(const PTimeInterval & timeout) {
      return outDataReadySyncPoint.Wait(timeout);
    }
    
    BOOL IsT38Mode() const { return T38Mode; }
    void ModemCallbackWithUnlock(INT extra);

    int stateOut;
    int delayRestOut;
    int onIdleOut;
    int callbackParamOut;
    DataStream bufOut;
    MODPARS ModParsOut;
    PINDEX countOut;
    int lastDteCharOut;
    PTime timeBeginOut;
    BOOL moreFramesOut;
    PSyncPoint outDataReadySyncPoint;
    
    int stateIn;
    int callbackParamIn;
    int isCarrierIn;
    
    ModStream *modStreamIn;
    ModStream *modStreamInSaved;
    
    volatile int stateModem;
    PNotifier modemCallback;
    BOOL T38Mode;
    
    PMutex Mutex;
    PMutex MutexOut;
    PMutex MutexIn;
    PMutex MutexModem;
    
    const PString name;
};
///////////////////////////////////////////////////////////////

#endif  // _T38ENGINE_H

