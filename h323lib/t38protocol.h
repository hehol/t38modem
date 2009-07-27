/*
 * t38protocol.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2009 Vyacheslav Frolov
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
 * Revision 1.1  2009-07-27 16:09:24  vfrolov
 * Initial revision
 *
 * Revision 1.1  2009/07/27 16:09:24  vfrolov
 * Initial revision
 *
 */

#ifndef _T38PROTOCOL_H
#define _T38PROTOCOL_H

#include <t38proto.h>
#include "../t38engine.h"

///////////////////////////////////////////////////////////////
class PASN_OctetString;

class T38Protocol : public OpalT38Protocol, public T38Engine
{
  PCLASSINFO(T38Protocol, OpalT38Protocol);

  public:

  /**@name Construction */
  //@{
    T38Protocol(const PString &_name = "")
      : T38Engine(_name),
        in_redundancy(0),
        ls_redundancy(0),
        hs_redundancy(0),
        re_interval(-1) {}
  //@}

  /**@name Operations */
  //@{
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

    void EncodeIFPPacket(PASN_OctetString &ifp_packet, const T38_IFP &T38_ifp) const;
    PBoolean HandleRawIFP(const PASN_OctetString & pdu);
    PBoolean Originate();
    PBoolean Answer();

    void CleanUpOnTermination();

  private:
    int in_redundancy;
    int ls_redundancy;
    int hs_redundancy;
    int re_interval;
};
///////////////////////////////////////////////////////////////

#endif  // _T38PROTOCOL_H

