/*
 * g7231_fake.h
 *
 * Fake G.723.1 codec for T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2002 Equivalence Pty Ltd
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
 * The Initial Developer of the Original Code is Equivalence Pty Ltd
 *
 * Contributor(s): 
 *
 * $Log: g7231_fake.h,v $
 * Revision 1.3  2003-12-18 13:16:52  vfrolov
 * Fixed all CPU usage
 *
 * Revision 1.3  2003/12/18 13:16:52  vfrolov
 * Fixed all CPU usage
 *
 * Revision 1.2  2002/07/23 01:15:24  craigs
 * Added Capability clone function
 *
 * Revision 1.1  2002/04/30 04:17:54  craigs
 * Initial version
 *
 */

// mostly "stolen" from OpenAM

#ifndef _PM_G7231_FAKE_H
#define _PM_G7231_FAKE_H

#include <h323.h>
#include <h245.h>
#include <ptclib/delaychan.h>

class G7231_Fake_Codec : public H323AudioCodec
{
  PCLASSINFO(G7231_Fake_Codec, H323AudioCodec);

  public:
    G7231_Fake_Codec(Direction dir);

    unsigned GetBandwidth() const;
    static int GetFrameLen(int val);
      
    BOOL Read(BYTE * buffer, unsigned & length, RTP_DataFrame &);
    BOOL Write(const BYTE * buffer, unsigned length, const RTP_DataFrame & rtp, unsigned & frames);

  protected:
    PAdaptiveDelay delayRead;
    PAdaptiveDelay delayWrite;
};  

class G7231_Fake_Capability : public H323AudioCapability
{
  PCLASSINFO(G7231_Fake_Capability, H323AudioCapability)

  public:
    G7231_Fake_Capability();
    PObject * Clone();

    unsigned GetSubType() const { return H245_AudioCapability::e_g7231; }
    PString GetFormatName() const { return "G.723.1"; }

    H323Codec * CreateCodec(H323Codec::Direction direction) const;

    BOOL OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const;
    BOOL OnReceivedPDU(const H245_AudioCapability & pdu, unsigned & packetSize);
    PObject * Clone() const;
};

#endif
