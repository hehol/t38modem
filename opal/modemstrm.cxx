/*
 * modemstrm.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2010 Vyacheslav Frolov
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
 * Revision 1.12  2010-10-06 16:54:19  vfrolov
 * Redesigned engine opening/closing
 *
 * Revision 1.12  2010/10/06 16:54:19  vfrolov
 * Redesigned engine opening/closing
 *
 * Revision 1.11  2010/03/15 13:46:46  vfrolov
 * Added resetting prepare packet timeout on start media patch for fax sink
 *
 * Revision 1.10  2010/01/14 18:32:51  vfrolov
 * Added ignoring packets with mismatched payload type and fake packets
 *
 * Revision 1.9  2009/12/08 15:06:22  vfrolov
 * Fixed incompatibility with OPAL trunk
 *
 * Revision 1.8  2009/11/26 07:24:22  vfrolov
 * Added adjusting sequence numbers
 *
 * Revision 1.7  2009/11/20 16:37:27  vfrolov
 * Fixed audio class application blocking by forced T.38 mode
 *
 * Revision 1.6  2009/11/10 08:13:38  vfrolov
 * Fixed race condition on re-opening T38Engine
 *
 * Revision 1.5  2009/10/27 19:03:50  vfrolov
 * Added ability to re-open T38Engine
 * Added ability to prepare IFP packets with adaptive delay/period
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

#include <opal/opal_config.h>

#include <asn/t38.h>
#include <opal/patch.h>

#include "../audio.h"
#include "../t38engine.h"
#include "modemstrm.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
AudioModemMediaStream::AudioModemMediaStream(
    OpalConnection & conn,
    unsigned sessionID,
    PBoolean isSource,
    AudioEngine *engine)
  : OpalMediaStream(conn, OpalPCM16, sessionID, isSource)
  , audioEngine(engine)
{
  PTRACE(4, "AudioModemMediaStream::AudioModemMediaStream " << *this);

  PAssert(audioEngine != NULL, "audioEngine is NULL");
}

AudioModemMediaStream::~AudioModemMediaStream()
{
  ReferenceObject::DelPointer(audioEngine);
}

PBoolean AudioModemMediaStream::Open()
{
  if (IsOpen())
    return TRUE;

  PTRACE(3, "AudioModemMediaStream::Open " << *this);

  if (IsSink())
    audioEngine->OpenIn(EngineBase::HOWNERIN(this));
  else
    audioEngine->OpenOut(EngineBase::HOWNEROUT(this));

  return OpalMediaStream::Open();
}

#if (OPAL_PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= OPAL_PACK_VERSION(3, 10, 5))
void AudioModemMediaStream::InternalClose()
#else
PBoolean AudioModemMediaStream::Close()
#endif
{
  if (IsOpen()) {
    PTRACE(3, "AudioModemMediaStream::Close " << *this);

    if (IsSink())
      audioEngine->CloseIn(EngineBase::HOWNERIN(this));
    else
      audioEngine->CloseOut(EngineBase::HOWNEROUT(this));
  }

#if (OPAL_PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) < OPAL_PACK_VERSION(3, 10, 5))
  return OpalMediaStream::Close();
#endif
}

PBoolean AudioModemMediaStream::ReadData(BYTE * data, PINDEX size, PINDEX & length)
{
  if (!IsOpen() || !audioEngine->Read(EngineBase::HOWNEROUT(this), data, size)) {
    length = 0;
    return false;
  }

  length = size;

  return true;
}

PBoolean AudioModemMediaStream::WriteData(const BYTE * data, PINDEX length, PINDEX & written)
{
  if (!IsOpen() || !audioEngine->Write(EngineBase::HOWNERIN(this), data, length)) {
    written = 0;
    return false;
  }

  written = length;

  return true;
}
/////////////////////////////////////////////////////////////////////////////
T38ModemMediaStream::T38ModemMediaStream(
    OpalConnection & conn,
    unsigned sessionID,
    PBoolean isSource,
    T38Engine *engine)
  : OpalMediaStream(conn, OpalT38, sessionID, isSource)
  , t38engine(engine)
{
  PTRACE(4, "T38ModemMediaStream::T38ModemMediaStream " << *this);

  PAssert(t38engine != NULL, "t38engine is NULL");

  PTRACE(4, "T38ModemMediaStream::T38ModemMediaStream DataSize=" << GetDataSize());
}

T38ModemMediaStream::~T38ModemMediaStream()
{
  ReferenceObject::DelPointer(t38engine);
}

PBoolean T38ModemMediaStream::Open()
{
  if (IsOpen())
    return TRUE;

  PTRACE(3, "T38ModemMediaStream::Open " << *this);

  currentSequenceNumber = 0;
#if PTRACING
  totallost = 0;
#endif

  if (IsSink())
    t38engine->OpenIn(EngineBase::HOWNERIN(this));
  else
    t38engine->OpenOut(EngineBase::HOWNEROUT(this));

  return OpalMediaStream::Open();
}

#if (OPAL_PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= OPAL_PACK_VERSION(3, 10, 5))
void T38ModemMediaStream::InternalClose()
#else
PBoolean T38ModemMediaStream::Close()
#endif
{
  if (IsOpen()) {
    PTRACE(3, "T38ModemMediaStream::Close " << *this);

    if (IsSink()) {
      PTRACE(2, "T38ModemMediaStream::Close Send statistics:"
                " sequence=" << currentSequenceNumber <<
                " lost=" << totallost);

      t38engine->CloseIn(EngineBase::HOWNERIN(this));
    } else {
      PTRACE(2, "T38ModemMediaStream::Close Receive statistics:"
                " sequence=" << currentSequenceNumber);

      t38engine->CloseOut(EngineBase::HOWNEROUT(this));
    }
  }

#if (OPAL_PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) < OPAL_PACK_VERSION(3, 10, 5))
  return OpalMediaStream::Close();
#endif
}

void T38ModemMediaStream::OnStartMediaPatch()
{
  if (IsSource()) {
#if (OPAL_PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= OPAL_PACK_VERSION(3, 10, 5))
    if (m_mediaPatch != NULL) {
      OpalMediaStreamPtr sink = m_mediaPatch->GetSink();
#else
    if (mediaPatch != NULL) {
      OpalMediaStreamPtr sink = mediaPatch->GetSink();
#endif
      if (sink != NULL) {
        OpalMediaFormat format = sink->GetMediaFormat();

        if (format.IsValid()) {
          if (format.GetMediaType() != OpalMediaType::Fax()) {
            myPTRACE(3, "T38ModemMediaStream::OnStartMediaPatch: use timeout=0, period=20 for sink " << *sink);

            t38engine->SetPreparePacketTimeout(EngineBase::HOWNEROUT(this), 0, 20);
          } else {
            myPTRACE(3, "T38ModemMediaStream::OnStartMediaPatch: use timeout=-1 for sink " << *sink);

            t38engine->SetPreparePacketTimeout(EngineBase::HOWNEROUT(this), -1);
          }
        } else {
          myPTRACE(1, "T38ModemMediaStream::OnStartMediaPatch: format is invalid !!!");
        }
      } else {
        myPTRACE(1, "T38ModemMediaStream::OnStartMediaPatch: sink is NULL !!!");
      }
    } else {
      myPTRACE(1, "T38ModemMediaStream::OnStartMediaPatch: mediaPatch is NULL !!!");
    }
  }
}

PBoolean T38ModemMediaStream::ReadPacket(RTP_DataFrame & packet)
{
  if (!IsOpen())
    return FALSE;

  T38_IFP ifp;
  int res;

  packet.SetTimestamp(timestamp);
  timestamp += 160;

  do {
    //PTRACE(4, "T38ModemMediaStream::ReadPacket ...");
    res = t38engine->PreparePacket(EngineBase::HOWNEROUT(this), ifp);
  } while (currentSequenceNumber == 0 && res < 0);

  packet[0] = 0x80;
  packet.SetPayloadType(mediaFormat.GetPayloadType());

  if (res > 0) {
    PTRACE(4, "T38ModemMediaStream::ReadPacket ifp = " << setprecision(2) << ifp);

    PASN_OctetString ifp_packet;
    ifp_packet.EncodeSubType(ifp);

    packet.SetPayloadSize(ifp_packet.GetDataLength());
    memcpy(packet.GetPayloadPtr(), ifp_packet.GetPointer(), ifp_packet.GetDataLength());
    packet.SetSequenceNumber(WORD(currentSequenceNumber++ & 0xFFFF));
  }
  else
  if (res < 0) {
    // send a "repeated" packet with a "fake" payload of one byte of 0xFF

    //packet.SetPayloadSize(1);
    //packet.GetPayloadPtr()[0] = 0xFF;

    packet.SetPayloadSize(0);
    packet.SetSequenceNumber(WORD((currentSequenceNumber - 1) & 0xFFFF));
  }
  else {
    return FALSE;
  }

  PTRACE(5, "T38ModemMediaStream::ReadPacket"
            " packet " << packet.GetSequenceNumber() <<
            " size=" << packet.GetPayloadSize() <<
            " type=" << packet.GetPayloadType() <<
            " ts=" << packet.GetTimestamp());

  return TRUE;
}

PBoolean T38ModemMediaStream::WritePacket(RTP_DataFrame & packet)
{
  if (!IsOpen())
    return FALSE;

  PTRACE(5, "T38ModemMediaStream::WritePacket "
            " packet " << packet.GetSequenceNumber() <<
            " size=" << packet.GetPayloadSize() <<
            " " << packet.GetPayloadType());

  if (mediaFormat.GetPayloadType() != packet.GetPayloadType()) {
    PTRACE(5, "T38ModemMediaStream::WritePacket: ignored packet with mismatched payload type");
    return TRUE;
  }

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
    PTRACE(lost == -1 ? 5 : 3,
        "T38ModemMediaStream::WritePacket: " << (packet.GetPayloadSize() == 0 ? "Fake" : "Repeated") <<
        " packet " << packedSequenceNumber << " (expected " << currentSequenceNumber << ")");

    if (lost > -10)
      return TRUE;
  }

  if (packet.GetPayloadSize() == 0) {
    PTRACE(5, "T38ModemMediaStream::WritePacket: ignored fake packet");
    return TRUE;
  }

  PASN_OctetString ifp_packet((const char *)packet.GetPayloadPtr(), packet.GetPayloadSize());

  T38_IFP ifp;

  if (!ifp_packet.DecodeSubType(ifp)) {
    PTRACE(2, "T38ModemMediaStream::WritePacket " T38_IFP_NAME " decode failure: "
        << PRTHEX(PBYTEArray(ifp_packet)) << "\n  ifp = "
        << setprecision(2) << ifp);
    return TRUE;
  }

  if (lost != 0) {
    if (lost < 0 || lost > 10)
      lost = 1;

#if PTRACING
    totallost += lost;
#endif

    if (!t38engine->HandlePacketLost(EngineBase::HOWNERIN(this), lost))
      return FALSE;

    PTRACE(3, "T38ModemMediaStream::WritePacket: adjusting sequence number to " << packedSequenceNumber);
  }

  currentSequenceNumber = packedSequenceNumber + 1;

  return t38engine->HandlePacket(EngineBase::HOWNERIN(this), ifp);
}
/////////////////////////////////////////////////////////////////////////////

