/*
 * g7231_fake.cxx
 *
 * Fake G.723.1 codec for T38FAX Pseudo Modem
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
 * The Initial Developer of the Original Code is Equivalence Pty Ltd
 *
 * Contributor(s): 
 *
 * $Log: g7231_fake.cxx,v $
 * Revision 1.3  2003-12-18 13:16:46  vfrolov
 * Fixed all CPU usage
 *
 * Revision 1.3  2003/12/18 13:16:46  vfrolov
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

#include <ptlib.h>
#include <ptclib/delaychan.h>
#include <codecs.h>

#include "g7231_fake.h"

#define	G7231_BANDWIDTH		63
#define	G7231_SAMPLES_PER_BLOCK	240

//////////////////////////////////////////////


G7231_Fake_Capability::G7231_Fake_Capability()
  : H323AudioCapability(8, 4)
{
}

PObject * G7231_Fake_Capability::Clone()
{
  return new G7231_Fake_Capability(*this);
}

BOOL G7231_Fake_Capability::OnSendingPDU(H245_AudioCapability & cap, unsigned packetSize) const
{
  // set the choice to the correct type
  cap.SetTag(GetSubType());

  // get choice data
  H245_AudioCapability_g7231 & g7231 = cap;

  // max number of audio frames per PDU we want to send
  g7231.m_maxAl_sduAudioFrames = packetSize; 

  // no silence suppression
  g7231.m_silenceSuppression = FALSE;

  return TRUE;
}

BOOL G7231_Fake_Capability::OnReceivedPDU(const H245_AudioCapability & cap, unsigned & packetSize)
{
  const H245_AudioCapability_g7231 & g7231 = cap;
  packetSize = g7231.m_maxAl_sduAudioFrames;
  return TRUE;
}

PObject * G7231_Fake_Capability::Clone() const
{
  return new G7231_Fake_Capability();
}

H323Codec * G7231_Fake_Capability::CreateCodec(H323Codec::Direction direction) const
{
  return new G7231_Fake_Codec(direction);
}

///////////////////////////////////////////////////////////////

G7231_Fake_Codec::G7231_Fake_Codec(Direction dir)
  : H323AudioCodec("G.723.1", dir)
{
}


int G7231_Fake_Codec::GetFrameLen(int val)
{
  static const int frameLen[] = { 24, 20, 4, 1 };
  return frameLen[val & 3];
}

BOOL G7231_Fake_Codec::Read(BYTE * /*buffer*/, unsigned & length, RTP_DataFrame &)
{
  length = 0;

  delayRead.Delay((G7231_SAMPLES_PER_BLOCK*2)/16);

  return TRUE;
}

BOOL G7231_Fake_Codec::Write(const BYTE * buffer, unsigned length, const RTP_DataFrame & /* rtp */, unsigned & writtenLength)
{
  writtenLength = length;

  delayWrite.Delay((G7231_SAMPLES_PER_BLOCK*2)/16);

  return TRUE;
}

unsigned G7231_Fake_Codec::GetBandwidth() const      { return G7231_BANDWIDTH; }
