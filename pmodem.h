/*
 * pmodem.h
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
 * $Log: pmodem.h,v $
 * Revision 1.2  2002-01-10 06:10:02  craigs
 * Added MPL header
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

#include <ptlib.h>

///////////////////////////////////////////////////////////////
class H323EndPoint;
class PseudoModemBody;
class T38Engine;

class PseudoModem : public PObject
{
    PCLASSINFO(PseudoModem, PObject);
  public:
  
  /**@name Construction */
  //@{
    PseudoModem(const PString &_tty, const PNotifier &callbackEndPoint);
    ~PseudoModem();
  //@}

  /**@name Operations */
    BOOL IsReady() const;
    BOOL Request(PStringToString &request) const;
    BOOL Attach(T38Engine *t38engine) const;
    void Detach(T38Engine *t38engine) const;

    static const char *ttyPattern();
    const PString &ptyName() const { return ptyname; }
    const PString &ptyPath() const { return ptypath; }
    const PString &ttyPath() const { return ttypath; }
    const PString &modemToken() const { return ttypath; }
    BOOL IsValid() const { return body != NULL; }
  //@}

  protected:
    Comparison Compare(const PObject & obj) const;
    
    PString ptyname;
    PString ptypath;
    PString ttypath;
    PseudoModemBody *body;
};
///////////////////////////////////////////////////////////////
PQUEUE(_PseudoModemQ, PseudoModem);

class PseudoModemQ : public _PseudoModemQ
{
    PCLASSINFO(PseudoModemQ, _PseudoModemQ);
  public:
    void Enqueue(PseudoModem *modem);
    PseudoModem *Dequeue();
    PseudoModem *Dequeue(const PString &modemToken);
    PseudoModem *Find(const PString &modemToken) const;
    void Clean();
  protected:
    PMutex Mutex;
};
///////////////////////////////////////////////////////////////

#endif  // _PMODEM_H

