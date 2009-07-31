/*
 * modemstrm.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2009 Vyacheslav Frolov
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
 * $Log: modemstrm.cxx,v $
 * Revision 1.4  2009-07-31 17:34:40  vfrolov
 * Removed --h323-old-asn and --sip-old-asn options
 *
 * Revision 1.4  2009/07/31 17:34:40  vfrolov
 * Removed --h323-old-asn and --sip-old-asn options
 *
 * Revision 1.3  2009/07/27 16:21:24  vfrolov
 * Moved h323lib specific code to h323lib directory
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#include <asn/t38.h>

#include "../t38engine.h"
#include "modemstrm.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
T38ModemMediaStream::T38ModemMediaStream(
    OpalConnection & conn,
    unsigned sessionID,
    PBoolean isSource,
    T38Engine *_t38engine)
  : OpalMediaStream(conn, GetT38MediaFormat(), sessionID, isSource),
    t38engine(_t38engine)
{
  PTRACE(4, "T38ModemMediaStream::T38ModemMediaStream " << *this);

  PTRACE(4, "T38ModemMediaStream::T38ModemMediaStream DataSize=" << GetDataSize());
}

const OpalMediaFormat & T38ModemMediaStream::GetT38MediaFormat()
{
  return OpalT38;
}

PBoolean T38ModemMediaStream::Open()
{
  if (isOpen)
    return TRUE;

  if (!t38engine) {
    PTRACE(1, "T38ModemMediaStream::Open No t38engine.");
    return FALSE;
  }

  PTRACE(3, "T38ModemMediaStream::Open " << *this);

  currentSequenceNumber = 0;
#if PTRACING
  totallost = 0;
#endif

  return OpalMediaStream::Open();
}

PBoolean T38ModemMediaStream::Close()
{
  if (isOpen) {
    PTRACE(3, "T38ModemMediaStream::Close " << *this);

    if (IsSink()) {
      PTRACE(2, "T38ModemMediaStream::Close Send statistics:"
                " sequence=" << currentSequenceNumber <<
                " lost=" << totallost);
    } else {
      PTRACE(2, "T38ModemMediaStream::Close Receive statistics:"
                " sequence=" << currentSequenceNumber);
    }

    t38engine->Close();
  }

  return OpalMediaStream::Close();
}

PBoolean T38ModemMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  T38_IFP ifp;
  int res;

  do {
    res = t38engine->PreparePacket(ifp);
  } while (currentSequenceNumber == 0 && res < 0);

  if (res > 0) {
    PTRACE(3, "T38ModemMediaStream::ReadPacket ifp = " << setprecision(2) << ifp);

    packet.SetSequenceNumber(WORD(currentSequenceNumber++ & 0xFFFF));

    PASN_OctetString ifp_packet;

    ifp_packet.EncodeSubType(ifp);

    packet.SetPayloadSize(ifp_packet.GetDataLength());
    packet[0] = 0x80;
    packet.SetPayloadType(mediaFormat.GetPayloadType());
    memcpy(packet.GetPayloadPtr(), ifp_packet.GetPointer(), ifp_packet.GetDataLength());
  }
  else
  if (res < 0) {
    // send a "repeated" packet with a "fake" payload of one byte of 0xFF

    packet.SetPayloadSize(1);
    packet[0] = 0x80;
    packet.SetPayloadType(mediaFormat.GetPayloadType());
    packet.SetSequenceNumber(WORD((currentSequenceNumber - 1) & 0xFFFF));
    packet.GetPayloadPtr()[0] = 0xFF;
  }
  else
    return FALSE;

  PTRACE(4, "T38ModemMediaStream::ReadPacket"
            " packet " << packet.GetSequenceNumber() <<
            " size=" << packet.GetPayloadSize() <<
            " " << packet.GetPayloadType());

  return TRUE;
}

PBoolean T38ModemMediaStream::WritePacket(RTP_DataFrame & packet)
{
  PTRACE(5, "T38ModemMediaStream::WritePacket "
            " packet " << packet.GetSequenceNumber() <<
            " size=" << packet.GetPayloadSize() <<
            " " << packet.GetPayloadType());

  long packedSequenceNumber = (packet.GetSequenceNumber() & 0xFFFF) + (currentSequenceNumber & ~0xFFFFL);
  long lost = packedSequenceNumber - currentSequenceNumber;

  if (lost < -0x10000L/2) {
    lost += 0x10000L;
    packedSequenceNumber += 0x10000L;
  }
  else
  if (lost > 0x10000L/2) {
    lost -= 0x10000L;
    packedSequenceNumber -= 0x10000L;
  }

  if (lost < 0) {
#if PTRACING
    if (PTrace::CanTrace(5)) {
      PBoolean fake = packet.GetPayloadSize() == 1 && packet.GetPayloadPtr()[0] == 0xFF;

      PTRACE(5, "T38ModemMediaStream::WritePacket " << (fake ? "Fake" : "Repeated")
          << " packet " << packedSequenceNumber << " (expected " << currentSequenceNumber << ")");
    }
#endif
    return TRUE;
  }
  else
  if (lost > 0) {
#if PTRACING
    totallost += lost;
#endif
    if (!t38engine->HandlePacketLost(lost))
      return FALSE;
  }

  PASN_OctetString ifp_packet((const char *)packet.GetPayloadPtr(), packet.GetPayloadSize());

  T38_IFP ifp;

  if (!ifp_packet.DecodeSubType(ifp)) {
    PTRACE(2, "T38ModemMediaStream::WritePacket " T38_IFP_NAME " decode failure: "
        << PRTHEX(PBYTEArray(ifp_packet)) << "\n  ifp = "
        << setprecision(2) << ifp);
    return TRUE;
  }

  currentSequenceNumber = packedSequenceNumber + 1;

  return t38engine->HandlePacket(ifp);
}

PBoolean T38ModemMediaStream::IsSynchronous() const
{
  PTRACE(3, "T38ModemMediaStream::IsSynchronous " << *this);

  return FALSE;
}
/////////////////////////////////////////////////////////////////////////////

