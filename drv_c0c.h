/*
 * drv_c0c.h
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
 * $Log: drv_c0c.h,v $
 * Revision 1.2  2004-07-19 08:31:06  vfrolov
 * Fixed "friend declaration requires class-key"
 *
 * Revision 1.2  2004/07/19 08:31:06  vfrolov
 * Fixed "friend declaration requires class-key"
 *
 * Revision 1.1  2004/07/07 13:36:46  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _DRV_C0C_H
#define _DRV_C0C_H

#ifdef _WIN32
  #define MODEM_DRIVER_C0C
#endif

#ifdef MODEM_DRIVER_C0C

#include "pmodemi.h"

///////////////////////////////////////////////////////////////
class InC0C;
class OutC0C;

class PseudoModemC0C : public PseudoModemBody
{
    PCLASSINFO(PseudoModemC0C, PseudoModemBody);

  public:
  /**@name Construction */
  //@{
    PseudoModemC0C(const PString &_tty, const PString &_route, const PNotifier &_callbackEndPoint);
    ~PseudoModemC0C();
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
    BOOL OpenC0C();
    void CloseC0C();
    BOOL OutPnpId();
    BOOL WaitReady();
    BOOL IsOpenC0C() const { return hC0C != INVALID_HANDLE_VALUE; }

    HANDLE hC0C;
    InC0C *inC0C;
    OutC0C *outC0C;
    BOOL reset;

    PString ptypath;

    friend class InC0C;
    friend class OutC0C;
};
///////////////////////////////////////////////////////////////

#endif // MODEM_DRIVER_C0C

#endif // _DRV_C0C_H

