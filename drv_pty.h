/*
 * drv_pty.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2004 Vyacheslav Frolov
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
 * $Log: drv_pty.h,v $
 * Revision 1.2  2004-07-19 08:31:06  vfrolov
 * Fixed "friend declaration requires class-key"
 *
 * Revision 1.2  2004/07/19 08:31:06  vfrolov
 * Fixed "friend declaration requires class-key"
 *
 * Revision 1.1  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 *
 *
 * Log: pty.h,v
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

#ifndef _DRV_PTY_H
#define _DRV_PTY_H

#ifndef _WIN32
  #define MODEM_DRIVER_Pty
#endif

#ifdef MODEM_DRIVER_Pty

#include "pmodemi.h"

///////////////////////////////////////////////////////////////
class InPty;
class OutPty;

class PseudoModemPty : public PseudoModemBody
{
    PCLASSINFO(PseudoModemPty, PseudoModemBody);

  public:
  /**@name Construction */
  //@{
    PseudoModemPty(const PString &_tty, const PString &_route, const PNotifier &_callbackEndPoint);
    ~PseudoModemPty();
  //@}

  /**@name static functions */
  //@{
    static BOOL CheckTty(const PString &_tty);
    static PStringArray Description();
  //@}

  protected:
  /**@name Overrides from class PseudoModemBody */
  //@{
    const PString &ttyPath() const;
    ModemThreadChild *GetPtyNotifier();
    BOOL StartAll();
    void StopAll();
    void MainLoop();
  //@}

  private:
    BOOL OpenPty();
    void ClosePty();
    BOOL IsOpenPty() const { return hPty >= 0; }

    int hPty;
    InPty *inPty;
    OutPty *outPty;

    PString ptypath;
    PString ttypath;

    friend class InPty;
    friend class OutPty;
};
///////////////////////////////////////////////////////////////

#endif // MODEM_DRIVER_Pty

#endif // _DRV_PTY_H

