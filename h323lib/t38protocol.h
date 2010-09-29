/*
 * t38protocol.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2009-2010 Vyacheslav Frolov
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
 * $Log: t38protocol.h,v $
 * Revision 1.2  2010-09-29 11:52:59  vfrolov
 * Redesigned engine attaching/detaching
 *
 * Revision 1.2  2010/09/29 11:52:59  vfrolov
 * Redesigned engine attaching/detaching
 *
 * Revision 1.1  2009/07/27 16:09:24  vfrolov
 * Initial revision
 *
 */

#ifndef _T38PROTOCOL_H
#define _T38PROTOCOL_H

#include <t38proto.h>

///////////////////////////////////////////////////////////////
class PASN_OctetString;
class T38Engine;
class PseudoModem;
///////////////////////////////////////////////////////////////
class T38Protocol : public OpalT38Protocol
{
  PCLASSINFO(T38Protocol, OpalT38Protocol);

  public:

  /**@name Construction */
  //@{
    T38Protocol(PseudoModem * pmodem);
    ~T38Protocol();
  //@}

  /**@name Operations */
  //@{
    PBoolean IsOK() const { return t38engine != NULL; }

    void SetRedundancy(
      int indication,
      int low_speed,
      int high_speed,
      int repeat_interval
    );

    /**The calling SetOldASN() is aquivalent to the following change of the t38.asn:

           -  t4-non-ecm-sig-end,
           -   ...
           +  t4-non-ecm-sig-end
     */
    void SetOldASN() { corrigendumASN = FALSE; }
  //@}

    PBoolean HandleRawIFP(const PASN_OctetString & pdu);
    PBoolean Originate();
    PBoolean Answer();

    void CleanUpOnTermination();

  private:
    T38Engine *t38engine;

    int in_redundancy;
    int ls_redundancy;
    int hs_redundancy;
    int re_interval;
};
///////////////////////////////////////////////////////////////

#endif  // _T38PROTOCOL_H

