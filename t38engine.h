/*
 * $Id: t38engine.h,v 1.4 2002-01-06 03:48:45 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: t38engine.h,v $
 * Revision 1.4  2002-01-06 03:48:45  craigs
 * Added changes to support efax 0.9
 * Thanks to Vyacheslav Frolov
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
      dtCed,
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
    void CleanUpOnTermination() { SetT38Mode(FALSE); }
    void SetT38Mode(BOOL mode = TRUE);
  //@}
  
  /**@name Modem API */
  //@{
    BOOL Attach(const PNotifier &callback);
    void Detach(const PNotifier &callback);
    void ResetModemState();

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
  
    virtual BOOL Originate(H323Transport & transport);
    virtual BOOL Answer(H323Transport & transport);

    /**Prepare outgoing T.38 packet.

       If returns FALSE, then the writing loop should be terminated.
      */
    virtual BOOL PreparePacket(
      T38_IFPPacket & ifp
    );

    /**Handle incoming T.38 packet.

       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL HandlePacket(
      const T38_IFPPacket & ifp
    );

    /**Handle lost T.38 packets.

       If returns FALSE, then the reading loop should be terminated.
      */
    virtual BOOL HandlePacketLost(
      unsigned nLost
    );
    
  private:
  
    virtual void SignalOutDataReady() { outDataReadySyncPoint.Signal(); }
    virtual void WaitOutDataReady() { outDataReadySyncPoint.Wait(); }
    
    BOOL IsT38Mode() const { return T38Mode; }

    int stateOut;
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

