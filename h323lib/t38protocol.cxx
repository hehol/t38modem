/*
 * t38protocol.cxx
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
 * $Log: t38protocol.cxx,v $
 * Revision 1.1  2009-07-27 16:09:24  vfrolov
 * Initial revision
 *
 * Revision 1.1  2009/07/27 16:09:24  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>
#include <t38.h>
#include <transports.h>

#include "t38protocol.h"

#define new PNEW

///////////////////////////////////////////////////////////////
void T38Protocol::SetRedundancy(int indication, int low_speed, int high_speed, int repeat_interval)
{
  if (indication >= 0)
    in_redundancy = indication;
  if (low_speed >= 0)
    ls_redundancy = low_speed;
  if (high_speed >= 0)
    hs_redundancy = high_speed;

  re_interval = repeat_interval;

  myPTRACE(3, name << " T38Protocol::SetRedundancy indication=" << in_redundancy
                                            << " low_speed=" << ls_redundancy
                                            << " high_speed=" << hs_redundancy
                                            << " repeat_interval=" << re_interval
  );
}

#ifdef OPTIMIZE_CORRIGENDUM_IFP
  #define T38_IFP_NOT_NATIVE       T38_PreCorrigendum_IFPPacket
  #define T38_IFP_NOT_NATIVE_NAME  "Pre-corrigendum IFP"
  #define IS_NATIVE_ASN            corrigendumASN
  #define IS_EXTENDABLE            FALSE
#else
  #define T38_IFP_NOT_NATIVE       T38_IFPPacket
  #define T38_IFP_NOT_NATIVE_NAME  "IFP"
  #define IS_NATIVE_ASN            (!corrigendumASN)
  #define IS_EXTENDABLE            TRUE
#endif

void T38Protocol::EncodeIFPPacket(PASN_OctetString &ifp_packet, const T38_IFP &T38_ifp) const
{
  if (!IS_NATIVE_ASN && T38_ifp.HasOptionalField(T38_IFPPacket::e_data_field)) {
    T38_IFP ifp = T38_ifp;
    PINDEX count = ifp.m_data_field.GetSize();

    for( PINDEX i = 0 ; i < count ; i++ ) {
      ifp.m_data_field[i].m_field_type.SetExtendable(IS_EXTENDABLE);
    }
    ifp_packet.EncodeSubType(ifp);
  } else {
    ifp_packet.EncodeSubType(T38_ifp);
  }
}

PBoolean T38Protocol::HandleRawIFP(const PASN_OctetString & pdu)
{
  T38_IFP ifp;

  if (IS_NATIVE_ASN) {
    if (pdu.DecodeSubType(ifp))
      return T38Engine::HandlePacket(ifp);

    PTRACE(2, "T38\t" T38_IFP_NAME " decode failure:\n  " << setprecision(2) << ifp);
    return TRUE;
  }

  T38_IFP_NOT_NATIVE old_ifp;
  if (!pdu.DecodeSubType(old_ifp)) {
    PTRACE(2, "T38\t" T38_IFP_NOT_NATIVE_NAME " decode failure:\n  " << setprecision(2) << old_ifp);
    return TRUE;
  }

  ifp.m_type_of_msg = old_ifp.m_type_of_msg;

  if (old_ifp.HasOptionalField(T38_IFPPacket::e_data_field)) {
    ifp.IncludeOptionalField(T38_IFPPacket::e_data_field);
    PINDEX count = old_ifp.m_data_field.GetSize();
    ifp.m_data_field.SetSize(count);
    for (PINDEX i = 0 ; i < count; i++) {
      ifp.m_data_field[i].m_field_type = old_ifp.m_data_field[i].m_field_type;
      if (old_ifp.m_data_field[i].HasOptionalField(T38_Data_Field_subtype::e_field_data)) {
        ifp.m_data_field[i].IncludeOptionalField(T38_Data_Field_subtype::e_field_data);
        ifp.m_data_field[i].m_field_data = old_ifp.m_data_field[i].m_field_data;
      }
    }
  }

  return T38Engine::HandlePacket(ifp);
}

PBoolean T38Protocol::Originate()
{
  RenameCurrentThread(name + "(tx)");
  PTRACE(2, "T38\tOriginate, transport=" << *transport);

  long seq = -1;
  int maxRedundancy = 0;
#if PTRACING
  int repeated = 0;
#endif

  T38_UDPTLPacket udptl;
  udptl.m_error_recovery.SetTag(T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets);

#if REPEAT_INDICATOR_SENDING
  T38_IFP lastifp;
#endif

  for (;;) {
    T38_IFP ifp;
    int res;

    if (seq < 0) {
      SetPreparePacketTimeout(-1);
    } else {
      int timeout = (
#ifdef REPEAT_INDICATOR_SENDING
        lastifp.m_type_of_msg.GetTag() == T38_Type_of_msg::e_t30_indicator ||
#endif
        maxRedundancy > 0) ? msPerOut * 3 : -1;

      if (re_interval > 0 && (timeout <= 0 || timeout > re_interval))
        timeout = re_interval;

      SetPreparePacketTimeout(timeout);
    }

    res = PreparePacket(ifp);

#ifdef REPEAT_INDICATOR_SENDING
    if (res > 0)
      lastifp = ifp;
    else
    if (res < 0 && lastifp.m_type_of_msg.GetTag() == T38_Type_of_msg::e_t30_indicator) {
      // send indicator again
      ifp = lastifp;
      res = 1;
    }
#endif

    if (res > 0) {
      T38_UDPTLPacket_error_recovery &recovery = udptl.m_error_recovery;
      if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
        int nRedundancy = secondary.GetSize() + 1;
        if (nRedundancy > maxRedundancy)
          nRedundancy = maxRedundancy;
        if (nRedundancy < 0)
          nRedundancy = 0;
        secondary.SetSize(nRedundancy);

        if (nRedundancy > 0) {
          for (int i = nRedundancy - 1 ; i > 0 ; i--) {
            secondary[i] = secondary[i - 1];
          }
          secondary[0].SetValue(udptl.m_primary_ifp_packet.GetValue());
        }
      } else {
        PTRACE_IF(3, maxRedundancy > 0, "T38\tNot implemented yet " << recovery.GetTagName());
      }

      udptl.m_seq_number = ++seq & 0xFFFF;

      EncodeIFPPacket(udptl.m_primary_ifp_packet, ifp);

      /*
       * Calculate maxRedundancy for current ifp packet
       */
      maxRedundancy = hs_redundancy;

      switch( ifp.m_type_of_msg.GetTag() ) {
        case T38_Type_of_msg::e_t30_indicator:
          maxRedundancy = in_redundancy;
          break;
        case T38_Type_of_msg::e_data:
          switch( (T38_Type_of_msg_data)ifp.m_type_of_msg ) {
            case T38_Type_of_msg_data::e_v21:
              maxRedundancy = ls_redundancy;
              break;
          }
          break;
      }

#if 0
      // recovery test
      if (seq % 2)
        continue;
#endif
    }
    else if (res < 0) {
      if (maxRedundancy > 0)
        maxRedundancy--;
#if 1
      /*
       * Optimise repeated packet each time
       */
      T38_UDPTLPacket_error_recovery &recovery = udptl.m_error_recovery;
      if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
        int nRedundancy = secondary.GetSize();
        if (nRedundancy > maxRedundancy)
          nRedundancy = maxRedundancy;
        if (nRedundancy < 0)
          nRedundancy = 0;
        secondary.SetSize(nRedundancy);
      }
      else {
        PTRACE(3, "T38\tNot implemented yet " << recovery.GetTagName());
      }
#endif
#if PTRACING
      repeated++;
#endif
    }
    else
      break;

    PPER_Stream rawData;
    udptl.Encode(rawData);
    rawData.CompleteEncoding();

#if PTRACING
    if (res > 0) {
      if (PTrace::CanTrace(4)) {
        PTRACE(4, "T38\tSending PDU:\n  ifp = "
             << setprecision(2) << ifp << "\n  UDPTL = "
             << setprecision(2) << udptl << "\n  "
             << setprecision(2) << rawData);
      }
      else
      if (PTrace::CanTrace(3)) {
        PTRACE(3, "T38\tSending PDU: seq=" << seq
             << "\n  ifp = " << setprecision(2) << ifp);
      }
      else {
        PTRACE(2, "T38\tSending PDU:"
                " seq=" << seq <<
                " type=" << ifp.m_type_of_msg.GetTagName());
      }
    }
    else {
      PTRACE(4, "T38\tSending PDU again:\n  UDPTL = "
             << setprecision(2) << udptl << "\n  "
             << setprecision(2) << rawData);
    }
#endif

    if (!transport->WritePDU(rawData)) {
      PTRACE(1, "T38\tOriginate - WritePDU ERROR: " << transport->GetErrorText());
      break;
    }
  }

  myPTRACE(2, "T38\tSend statistics: sequence=" << seq
      << " repeated=" << repeated
      << GetThreadTimes(", CPU usage: "));

  return FALSE;
}

PBoolean T38Protocol::Answer()
{
  RenameCurrentThread(name + "(rx)");
  PTRACE(2, "T38\tAnswer, transport=" << *transport);

  // We can't get negotiated sender's address and port,
  // so accept first packet from any address and port
  transport->SetPromiscuous(transport->AcceptFromAny);

  int consecutiveBadPackets = 0;
  long expectedSequenceNumber = 0;
#if PTRACING
  int totalrecovered = 0;
  int totallost = 0;
  int repeated = 0;
#endif

  for (;;) {
    PPER_Stream rawData;
    if (!transport->ReadPDU(rawData)) {
      PTRACE(1, "T38\tError reading PDU: " << transport->GetErrorText(PChannel::LastReadError));
      break;
    }

    T38_UDPTLPacket udptl;

    if (udptl.Decode(rawData)) {
      consecutiveBadPackets = 0;

      // When we get the first packet, we know sender's address and port,
      // so accept next packets from sender's address and port only
      if (expectedSequenceNumber == 0) {
        PTRACE(3, "T38\tReceived first packet, remote=" << transport->GetLastReceivedAddress());
        transport->SetPromiscuous(transport->AcceptFromLastReceivedOnly);
      }
    } else {
      consecutiveBadPackets++;
      PTRACE(2, "T38\tRaw data decode failure:\n  "
             << setprecision(2) << rawData << "\n  UDPTL = "
             << setprecision(2) << udptl);
      if (consecutiveBadPackets > 3) {
        PTRACE(1, "T38\tRaw data decode failed multiple times, aborting!");
        break;
      }
      continue;
    }

    long receivedSequenceNumber = (udptl.m_seq_number & 0xFFFF) + (expectedSequenceNumber & ~0xFFFFL);
    long lost = receivedSequenceNumber - expectedSequenceNumber;

    if (lost < -0x10000L/2) {
      lost += 0x10000L;
      receivedSequenceNumber += 0x10000L;
    }
    else if (lost > 0x10000L/2) {
      lost -= 0x10000L;
      receivedSequenceNumber -= 0x10000L;
    }

    PTRACE(4, "T38\tReceived PDU:\n  "
           << setprecision(2) << rawData << "\n  UDPTL = "
           << setprecision(2) << udptl);

    if (lost < 0) {
      PTRACE(4, "T38\tRepeated packet " << receivedSequenceNumber);
#if PTRACING
      repeated++;
#endif
      continue;
    }
    else if(lost > 0) {
      const T38_UDPTLPacket_error_recovery &recovery = udptl.m_error_recovery;
      if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        const T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
        int nRedundancy = secondary.GetSize();
        if (lost > nRedundancy) {
          if (!T38Engine::HandlePacketLost(lost - nRedundancy))
            break;
#if PTRACING
          totallost += lost - nRedundancy;
#endif
          lost = nRedundancy;
        }
        else
          nRedundancy = lost;

        receivedSequenceNumber -= lost;

        for (int i = nRedundancy - 1 ; i >= 0 ; i--) {
          PTRACE(3, "T38\tReceived ifp seq=" << receivedSequenceNumber << " (secondary)");

          if (!HandleRawIFP(secondary[i]))
            goto done;

#if PTRACING
          totalrecovered++;
#endif
          lost--;
          receivedSequenceNumber++;
        }
        receivedSequenceNumber += lost;
      }
      else {
        PTRACE(3, "T38\tNot implemented yet " << recovery.GetTagName());
      }

      if (lost) {
        if (!T38Engine::HandlePacketLost(lost))
          break;
#if PTRACING
        totallost += lost;
#endif
      }
    }

#if 0
    // recovery test
    expectedSequenceNumber = receivedSequenceNumber;
    continue;
#endif

    PTRACE(3, "T38\tReceived ifp seq=" << receivedSequenceNumber);

    if (!HandleRawIFP(udptl.m_primary_ifp_packet))
      break;

    expectedSequenceNumber = receivedSequenceNumber + 1;
  }

done:

  myPTRACE(2, "T38\tReceive statistics: sequence=" << expectedSequenceNumber
      << " repeated=" << repeated
      << " recovered=" << totalrecovered
      << " lost=" << totallost
      << GetThreadTimes(", CPU usage: "));
  return FALSE;
}
///////////////////////////////////////////////////////////////
void T38Protocol::CleanUpOnTermination()
{
  myPTRACE(1, name << " T38Protocol::CleanUpOnTermination");

  OpalT38Protocol::CleanUpOnTermination();
  T38Engine::Close();
}
///////////////////////////////////////////////////////////////

