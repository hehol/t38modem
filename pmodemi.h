/*
 * $Id: pmodemi.h,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: pmodemi.h,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _PMODEMI_H
#define _PMODEMI_H

#include "pmutils.h"

///////////////////////////////////////////////////////////////
class PseudoModem;
class InPty;
class OutPty;
class ModemEngine;
class T38Engine;

class PseudoModemBody : public ModemThread
{
    PCLASSINFO(PseudoModemBody, ModemThread);
  public:
  
 
  /**@name Construction */
  //@{
    PseudoModemBody(const PseudoModem &_parent, const PNotifier &callbackEndPoint);
    ~PseudoModemBody();
  //@}

  /**@name Operations */
  //@{
    PBYTEArray *FromOutPtyQ() { return outPtyQ.Dequeue(); }
    PBYTEArray *FromInPtyQ() { return inPtyQ.Dequeue(); }
    void ToPtyQ(const void *buf, PINDEX count, BOOL OutQ);
  //@}
  
    BOOL IsReady() const;
    BOOL Request(PStringToString &request) const;
    BOOL Attach(T38Engine *t38engine) const;
    void Detach(T38Engine *t38engine) const;
    
    const PNotifier &GetCallbackEndPoint() const { return callbackEndPoint; }

    const PString &ptyName() const;
    const PString &ptyPath() const;
    const PString &ttyPath() const;
    const PString &modemToken() const;
    int handlePty() const { return hPty; }

    PMutex ptyMutex;

  protected:
    virtual BOOL StartAll();
    virtual void StopAll();
    virtual BOOL OpenPty();
    BOOL IsOpenPty() const { return hPty >= 0; }
    virtual void ClosePty();
    virtual void Main();

    const PseudoModem &parent;
    const PNotifier callbackEndPoint;
    int hPty;
    InPty *inPty;
    OutPty *outPty;
    ModemEngine *engine;
    
    PBYTEArrayQ outPtyQ;
    PBYTEArrayQ inPtyQ;
    PMutex Mutex;
};
///////////////////////////////////////////////////////////////

#endif  // _PMODEMI_H

