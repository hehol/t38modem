/*
 * drivers.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2004 Vyacheslav Frolov
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
 * Contributor(s): 
 *
 * $Log: drivers.h,v $
 * Revision 1.1  2004-07-07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 *
 * Revision 1.1  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 *
 *
 */

#ifndef _DRIVERS_H
#define _DRIVERS_H

///////////////////////////////////////////////////////////////
class PseudoModem;

class PseudoModemDrivers : protected PObject
{
    PCLASSINFO(PseudoModemDrivers, PObject);
  public:
    static PseudoModem *CreateModem(
      const PString &tty,
      const PString &route,
      const PNotifier &callbackEndPoint
    );

    static PStringArray Descriptions();
};
///////////////////////////////////////////////////////////////

#endif  // _DRIVERS_H

