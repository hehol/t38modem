/*
 * pmodeme.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2010 Vyacheslav Frolov
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
 * Revision 1.8  2010-10-06 16:54:19  vfrolov
 * Redesigned engine opening/closing
 *
 * Revision 1.8  2010/10/06 16:54:19  vfrolov
 * Redesigned engine opening/closing
 *
 * Revision 1.7  2010/09/29 11:52:59  vfrolov
 * Redesigned engine attaching/detaching
 *
 * Revision 1.6  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.5  2007/03/23 10:14:35  vfrolov
 * Implemented voice mode functionality
 *
 * Revision 1.4  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
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
    PBoolean IsReady() const;
    PBoolean Request(PStringToString &request) const;
    virtual T38Engine *NewPtrT38Engine(PBoolean useFastT38) const;
    virtual AudioEngine *NewPtrAudioEngine() const;
    virtual EngineBase *NewPtrUserInputEngine() const;
    const PString &ptyName() const { return Parent().ptyName(); }
    const PString &modemToken() const { return Parent().modemToken(); }
  //@}

  protected:
    PseudoModemBody &Parent() const { return (PseudoModemBody &)parent; }
    virtual void Main();
    void ToPtyQ(const void *buf, PINDEX count) const { Parent().ToOutPtyQ(buf, count); }

    ModemEngineBody *body;
};
///////////////////////////////////////////////////////////////

#endif  // _PMODEME_H

