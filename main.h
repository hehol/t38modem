// DRAFT DRAFT DRAFT
// mostly "stolen" from OpenAM

#ifndef _PM_MAIN_H
#define _PM_MAIN_H

#include <h323.h>
#include <h323pdu.h>
#include <lid.h>
#include "pmutils.h"

class T38Modem : public PProcess
{
  PCLASSINFO(T38Modem, PProcess)

  public:
    T38Modem();
    ~T38Modem();

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

    BOOL ForceT38Mode() const { return forceT38Mode; }
    PseudoModem * PMAlloc() const;
    void PMFree(PseudoModem *pmodem) const;

  protected:
    BOOL forceT38Mode;
    PseudoModemQ *pmodemQ;
    PStringArray routes;

    PDECLARE_NOTIFIER(PObject, MyH323EndPoint, OnMyCallback);
};

class AudioRead;
class AudioWrite;
class OpalT38Protocol;

class MyH323Connection : public H323Connection
{
  PCLASSINFO(MyH323Connection, H323Connection);

  public:
    MyH323Connection(MyH323EndPoint &, unsigned);
    ~MyH323Connection();

    //OpalT38Protocol * GetT38ProtocolHandler();
    //H323TransportUDP * GetT38TransportUDP();
    BOOL Attach(PseudoModem *_pmodem);

    // overrides from H323Connection
    AnswerCallResponse OnAnswerCall(const PString &, const H323SignalPDU &, H323SignalPDU &);
    BOOL OnStartLogicalChannel(H323Channel & channel);
    void OnClosedLogicalChannel(const H323Channel & channel);

    OpalT38Protocol * CreateT38ProtocolHandler() const;
    BOOL OpenAudioChannel(BOOL, unsigned, H323AudioCodec & codec);
    BOOL ForceT38Mode() const { return ep.ForceT38Mode(); }

  protected:
    const MyH323EndPoint & ep;
    PString e164Number;
    
    OpalT38Protocol *t38handler;
    H323TransportUDP *T38TransportUDP;
    PMutex T38Mutex;
    PseudoModem *pmodem;
    
    PMutex  connMutex;

    AudioWrite * audioWrite;
    AudioRead * audioRead;
};

///////////////////////////////////////////////////////////////

class AudioRead : public PChannel
{
  PCLASSINFO(AudioRead, PChannel);

  public:
    enum {
      Min_Headroom = 90,
      Max_Headroom = 150
    };

    AudioRead(MyH323Connection & conn);
    ~AudioRead();
    BOOL Read(void * buffer, PINDEX amount);
    BOOL Close();

  protected:
    void CreateSilenceFrame(PINDEX amount);
    void Synchronise(PINDEX amount);

    void FrameDelay(int delay);
    BOOL AdjustFrame(void * buffer, PINDEX amount);

    MyH323Connection & conn;
    BOOL closed;

    int headRoom;
    int WasRead;
    PTime lastReadTime;

    BYTE *frameBuffer;
    PINDEX frameLen, frameOffs;
    
    PMutex Mutex;
};

class AudioDelay : public PObject
{ 
  PCLASSINFO(AudioDelay, PObject);
  
  public:
    AudioDelay();
    BOOL Delay(int time);
    void Restart();
    int  GetError();
 
  protected:
    PTime  previousTime;
    BOOL   firstTime;
    int    error;
};

class AudioWrite : public PChannel
{
  PCLASSINFO(AudioWrite, PChannel)

  public:
    AudioWrite(MyH323Connection & conn);
    ~AudioWrite();

    BOOL Write(const void * buf, PINDEX len);
    BOOL Close();

  protected:
    MyH323Connection & conn;
    BOOL closed;
    AudioDelay delay;
    PMutex Mutex;
};

#endif  // _PM_MAIN_H


// End of File ///////////////////////////////////////////////////////////////

