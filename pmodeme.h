/*
 * pmodeme.cxx
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
 * $Log: pmodeme.h,v $
 * Revision 1.3  2002-04-19 14:29:33  vfrolov
 * Added Copyright header
 *
 * Revision 1.3  2002/04/19 14:29:33  vfrolov
 * Added Copyright header
 *
 * Revision 1.2  2002/01/10 06:10:02  craigs
 * Added MPL header
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _PMODEME_H
#define _PMODEME_H

#include "pmodemi.h"

///////////////////////////////////////////////////////////////
class ModemEngineBody;
class PseudoModemBody;
class T38Engine;

class ModemEngine : public ModemThreadChild
{
    PCLASSINFO(ModemEngine, ModemThreadChild);
  public:
 
  /**@name Construction */
  //@{
    ModemEngine(PseudoModemBody &_parent);
    ~ModemEngine();
  //@}

  /**@name Operations */
  //@{
    BOOL IsReady() const;
    BOOL Request(PStringToString &request) const;
    BOOL Attach(T38Engine *t38engine) const;
    void Detach(T38Engine *t38engine) const;
    const PString &modemToken() const { return Parent().modemToken(); }
  //@}

  protected:
    PseudoModemBody &Parent() const { return (PseudoModemBody &)parent; }
    virtual void Main();
    const PString &ptyName() const { return Parent().ptyName(); }
    void ToPtyQ(const void *buf, PINDEX count) const { Parent().ToPtyQ(buf, count, TRUE); }
    
    ModemEngineBody *body;
};
///////////////////////////////////////////////////////////////

#endif  // _PMODEME_H

