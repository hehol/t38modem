/*
 * pmodemi.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2009 Vyacheslav Frolov
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
 * Revision 1.8  2009-07-08 18:43:44  vfrolov
 * Added PseudoModem::ttyName()
 *
 * Revision 1.8  2009/07/08 18:43:44  vfrolov
 * Added PseudoModem::ttyName()
 *
 * Revision 1.7  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.6  2007/03/23 10:14:36  vfrolov
 * Implemented voice mode functionality
 *
 * Revision 1.5  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 *
 * Revision 1.4  2002/05/15 16:18:00  vfrolov
 * Implemented per modem routing for I/C calls
 *
 * Revision 1.3  2002/03/05 12:32:02  vfrolov
 * Added Copyright header
 * Changed class hierarchy
 *   PseudoModem is abstract
 *   PseudoModemBody is child of PseudoModem
 *   Added PseudoModemQ::CreateModem() to create instances
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

#include "pmodem.h"

///////////////////////////////////////////////////////////////
class ModemEngine;

class PseudoModemBody : public PseudoModem
{
    PCLASSINFO(PseudoModemBody, PseudoModem);
  public:

  /**@name Construction */
  //@{
    PseudoModemBody(const PString &_tty, const PString &_route, const PNotifier &_callbackEndPoint);
    ~PseudoModemBody();
  //@}

  /**@name Operations */
  //@{
    PBYTEArray *FromInPtyQ() { return inPtyQ.Dequeue(); }
    void ToOutPtyQ(const void *buf, PINDEX count) { ToPtyQ(buf, count, TRUE); };
  //@}

    virtual PBoolean IsReady() const;
    PBoolean CheckRoute(const PString &number) const;
    PBoolean Request(PStringToString &request) const;
    PBoolean Attach(T38Engine *t38engine) const;
    void Detach(T38Engine *t38engine) const;
    PBoolean Attach(AudioEngine *audioEngine) const;
    void Detach(AudioEngine *audioEngine) const;

    const PNotifier &GetCallbackEndPoint() const { return callbackEndPoint; }

  protected:
    virtual const PString &ttyPath() const = 0;
    virtual ModemThreadChild *GetPtyNotifier() = 0;
    virtual PBoolean StartAll();
    virtual void StopAll();
    virtual void MainLoop() = 0;

    PBoolean AddModem() const;
    PBYTEArray *FromOutPtyQ() { return outPtyQ.Dequeue(); }
    void ToInPtyQ(PBYTEArray *buf) { inPtyQ.Enqueue(buf); }
    void ToInPtyQ(const void *buf, PINDEX count) { ToPtyQ(buf, count, FALSE); };

    PMutex Mutex;

  private:
    void Main();
    void ToPtyQ(const void *buf, PINDEX count, PBoolean OutQ);

    PString route;
    const PNotifier callbackEndPoint;
    ModemEngine *engine;

    PBYTEArrayQ outPtyQ;
    PBYTEArrayQ inPtyQ;
};
///////////////////////////////////////////////////////////////

#endif  // _PMODEMI_H

