/*
 * pmodemi.h
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
 * $Log: pmodemi.h,v $
 * Revision 1.2  2002-01-10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.2  2002/01/10 06:10:03  craigs
 * Added MPL header
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

