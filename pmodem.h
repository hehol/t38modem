/*
 * pmodem.h
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
 * $Log: pmodem.h,v $
 * Revision 1.4  2002-05-15 16:17:49  vfrolov
 * Implemented per modem routing for I/C calls
 *
 * Revision 1.4  2002/05/15 16:17:49  vfrolov
 * Implemented per modem routing for I/C calls
 *
 * Revision 1.3  2002/03/05 12:31:58  vfrolov
 * Added Copyright header
 * Changed class hierarchy
 *   PseudoModem is abstract
 *   PseudoModemBody is child of PseudoModem
 *   Added PseudoModemQ::CreateModem() to create instances
 *
 * Revision 1.2  2002/01/10 06:10:02  craigs
 * Added MPL header
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _PMODEM_H
#define _PMODEM_H

#include "pmutils.h"

///////////////////////////////////////////////////////////////
class T38Engine;

class PseudoModem : public ModemThread
{
    PCLASSINFO(PseudoModem, ModemThread);
  public:
  
  /**@name Construction */
  //@{
    PseudoModem(const PString &_tty);
  //@}

  /**@name Operations */
    virtual BOOL IsReady() const = 0;
    virtual BOOL CheckRoute(const PString &number) const = 0;
    virtual BOOL Request(PStringToString &request) const = 0;
    virtual BOOL Attach(T38Engine *t38engine) const = 0;
    virtual void Detach(T38Engine *t38engine) const = 0;

    static const char *ttyPattern();
    const PString &ptyName() const { return ptyname; }
    const PString &ptyPath() const { return ptypath; }
    const PString &ttyPath() const { return ttypath; }
    const PString &modemToken() const { return ttypath; }
    BOOL IsValid() const { return valid; }
  //@}

  protected:
    Comparison Compare(const PObject & obj) const;
    BOOL ttySet(const PString &_tty);
    
    PString ptyname;
    PString ptypath;
    PString ttypath;
    BOOL valid;
};
///////////////////////////////////////////////////////////////
PQUEUE(_PseudoModemQ, PseudoModem);

class PseudoModemQ : protected _PseudoModemQ
{
    PCLASSINFO(PseudoModemQ, _PseudoModemQ);
  public:
    BOOL CreateModem(const PString &tty, const PString &route, const PNotifier &callbackEndPoint);
    void Enqueue(PseudoModem *modem);
    PseudoModem *DequeueWithRoute(const PString &number);
    PseudoModem *Dequeue(const PString &modemToken);
    PseudoModem *Find(const PString &modemToken) const;
    void Clean();
  protected:
    PMutex Mutex;
};
///////////////////////////////////////////////////////////////

#endif  // _PMODEM_H

