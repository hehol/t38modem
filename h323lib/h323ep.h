// DRAFT DRAFT DRAFT
// mostly "stolen" from OpenAM

#ifndef _Voxilla_MAIN_H
#define _Voxilla_MAIN_H

#include <h323.h>
#include <h323pdu.h>
#include <lid.h>
#include "pmutils.h"

class OpenAm : public PProcess
{
  PCLASSINFO(OpenAm, PProcess)

  public:
    OpenAm();
    ~OpenAm();

    void Main();
    void RecordFile(PArgList & args);

  protected:
    long GetCodec(const PString & codecname);
    OpalLineInterfaceDevice * GetDevice(const PString & device);
};

class PseudoModem;
class PseudoModemQ;

class MyH323EndPoint : public H323EndPoint
{
  PCLASSINFO(MyH323EndPoint, H323EndPoint);

  public:
    MyH323EndPoint();

    // overrides from H323EndPoint
    virtual H323Connection * CreateConnection(unsigned callReference);
    BOOL OnIncomingCall(H323Connection &, const H323SignalPDU &, H323SignalPDU &);

    // new functions
    BOOL Initialise(PConfigArgs & args);
    void OnConnectionEstablished(H323Connection &, const PString &);

    BOOL       requestT38Mode() const	      { return forceT38Mode; }
    PseudoModem * PMAlloc() const;
    void PMFree(PseudoModem *pmodem) const;

  protected:
    BOOL forceT38Mode;
    PseudoModemQ *pmodemQ;
    PString remote;	// remote host

    PDECLARE_NOTIFIER(PObject, MyH323EndPoint, OnMyCallback);
};

class OpalT38Protocol;

class MyH323Connection : public H323Connection
{
  PCLASSINFO(MyH323Connection, H323Connection);

  public:
    MyH323Connection(MyH323EndPoint &, unsigned);
    ~MyH323Connection();

    OpalT38Protocol * GetT38ProtocolHandler();
    H323TransportUDP * GetT38TransportUDP();
    BOOL Attach(PseudoModem *_pmodem);

    // overrides from H323Connection
    AnswerCallResponse OnAnswerCall(const PString &, const H323SignalPDU &, H323SignalPDU &);
    BOOL OnStartLogicalChannel(H323Channel & channel);
    OpalT38Protocol * CreateT38ProtocolHandler() const;

    void SetE164Number(const PString & _num)
      { e164Number = _num; }

    PString GetE164Number() const
      { return e164Number; }

  protected:
    const MyH323EndPoint & ep;
    PString securityToken, e164Number;
    
    OpalT38Protocol *t38handler;
    H323TransportUDP *T38TransportUDP;
    PMutex T38Mutex;
    PseudoModem *pmodem;
};

#endif  // _Voxilla_MAIN_H


// End of File ///////////////////////////////////////////////////////////////

