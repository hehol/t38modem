/*
 * pty.h
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
 * $Log: pty.h,v $
 * Revision 1.3  2002-04-19 14:29:37  vfrolov
 * Added Copyright header
 *
 * Revision 1.3  2002/04/19 14:29:37  vfrolov
 * Added Copyright header
 *
 * Revision 1.2  2002/01/10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#ifndef _PTY_H
#define _PTY_H

#include "pmutils.h"

///////////////////////////////////////////////////////////////
class PseudoModemBody;
///////////////////////////////////////////////////////////////
class UniPty : public ModemThreadChild
{
    PCLASSINFO(UniPty, ModemThreadChild);
  public:
  
 
  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    UniPty(PseudoModemBody &_parent);
  //@}

  /**@name Operations */
  //@{
  //@}

  protected:
    PseudoModemBody &Parent() { return (PseudoModemBody &)parent; }
    int hPty;
};
///////////////////////////////////////////////////////////////
class InPty : public UniPty
{
    PCLASSINFO(InPty, UniPty);
  public:
  
 
  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    InPty(PseudoModemBody &_parent);
  //@}

  /**@name Operations */
  //@{
  //@}

  protected:
    virtual void Main();
};
///////////////////////////////////////////////////////////////
class OutPty : public UniPty
{
    PCLASSINFO(OutPty, UniPty);
  public:
 
  /**@name Construction */
  //@{
    /**Create a new protocol handler.
     */
    OutPty(PseudoModemBody &_parent);
  //@}

  /**@name Operations */
  //@{
  //@}

  protected:
    virtual void Main();
};
///////////////////////////////////////////////////////////////

#endif  // _PTY_H

