/*
 * t38session.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007 Vyacheslav Frolov
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
 * $Log: t38session.cxx,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 *
 */

#include <ptlib.h>
#include <t38/t38proto.h>
#include <asn/t38.h>

#include "t38session.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
class MyT38PseudoRTP : public T38PseudoRTP
{
  PCLASSINFO(MyT38PseudoRTP, T38PseudoRTP);

  public:
  /**@name Construction */
  //@{
    /**Create a new RTP channel.
     */
    MyT38PseudoRTP(
      PHandleAggregator * aggregator, ///<  RTP aggregator
      unsigned id,                    ///<  Session ID for RTP channel
      BOOL remoteIsNAT,               ///<  TRUE is remote is behind NAT
      RTP_DataFrame::PayloadTypes pt
    );

#if PTRACING
    ~MyT38PseudoRTP();
#endif
  //@}

    void SetRedundancy(
      int indication,
      int low_speed,
      int high_speed,
      int repeat_interval
    );

  protected:
    virtual SendReceiveStatus OnSendData(RTP_DataFrame & frame);
    virtual SendReceiveStatus OnReceiveData(RTP_DataFrame & frame);
    virtual BOOL ReadData(RTP_DataFrame & frame, BOOL loop);

    SendReceiveStatus GetFrame(RTP_DataFrame & frame);

    RTP_DataFrame::PayloadTypes rtpPayloadType;

#if PTRACING
    int repeatedSend;
    int lostSend;
    int recoveredRecv;
    int lostRecv;
    int repeatedRecv;
#endif

    BOOL redundancy;
    int in_redundancy;
    int ls_redundancy;
    int hs_redundancy;
    int re_interval;

    long currentSequenceNumberSend;
    int maxRedundancySend;
    T38_UDPTLPacket udptlSend;
    PTime lastTimeSend;

    long currentSequenceNumberRecv;
    long udptlSequenceNumberRecv;
    T38_UDPTLPacket udptlRecv;
};
/////////////////////////////////////////////////////////////////////////////
//
//  Implementation
//
/////////////////////////////////////////////////////////////////////////////
RTP_Session * CreateSessionT38(
    const OpalConnection & connection,
    const OpalTransport & transport,
    unsigned sessionID,
    RTP_QOS * rtpqos,
    RTP_DataFrame::PayloadTypes pt,
    int in_redundancy,
    int ls_redundancy,
    int hs_redundancy,
    int re_interval)
{
  if (!transport.IsCompatibleTransport("ip$127.0.0.1"))
    return NULL;

  PIPSocket::Address localAddress;
  transport.GetLocalAddress().GetIpAddress(localAddress);

  OpalManager & manager = connection.GetEndPoint().GetManager();

  PIPSocket::Address remoteAddress;
  transport.GetRemoteAddress().GetIpAddress(remoteAddress);

  PSTUNClient * stun = manager.GetSTUN(remoteAddress);

  MyT38PseudoRTP *rtpSession = new MyT38PseudoRTP(NULL, sessionID, connection.RemoteIsNAT(), pt);
  rtpSession->SetRedundancy(in_redundancy, ls_redundancy, hs_redundancy, re_interval);

  WORD firstPort = manager.GetRtpIpPortPair();
  WORD nextPort = firstPort;

  while (!rtpSession->Open(localAddress,
                           nextPort, nextPort,
                           manager.GetRtpIpTypeofService(),
                           stun,
                           rtpqos))
  {
    nextPort = manager.GetRtpIpPortPair();
    if (nextPort == firstPort) {
      delete rtpSession;
      return NULL;
    }
  }

  localAddress = rtpSession->GetLocalAddress();
  if (manager.TranslateIPAddress(localAddress, remoteAddress))
    rtpSession->SetLocalAddress(localAddress);

  PTRACE(3, "CreateSessionT38 created " << *rtpSession << " " << pt);

  return rtpSession;
}
/////////////////////////////////////////////////////////////////////////////
MyT38PseudoRTP::MyT38PseudoRTP(
    PHandleAggregator * aggregator,
    unsigned id,
    BOOL remoteIsNAT,
    RTP_DataFrame::PayloadTypes pt)
  : T38PseudoRTP(aggregator, id, remoteIsNAT),
    rtpPayloadType(pt),
#if PTRACING
    repeatedSend(0),
    lostSend(0),
    recoveredRecv(0),
    lostRecv(0),
    repeatedRecv(0),
#endif
    redundancy(FALSE),
    in_redundancy(0),
    ls_redundancy(0),
    hs_redundancy(0),
    re_interval(-1),
    currentSequenceNumberSend(0),
    maxRedundancySend(0),
    currentSequenceNumberRecv(0),
    udptlSequenceNumberRecv(-1)
{
  udptlSend.m_error_recovery.SetTag(T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets);
}

#if PTRACING
MyT38PseudoRTP::~MyT38PseudoRTP()
{
  PTRACE(2, "MyT38PseudoRTP::Close Send statistics:"
            " sequence=" << currentSequenceNumberSend <<
            " repeated=" << repeatedSend <<
            " lost=" << lostSend);

  PTRACE(2, "MyT38PseudoRTP::Close Receive statistics:"
            " sequence=" << currentSequenceNumberRecv <<
            " repeated=" << repeatedRecv <<
            " recovered=" << recoveredRecv <<
            " lost=" << lostRecv);
}
#endif

void MyT38PseudoRTP::SetRedundancy(int indication, int low_speed, int high_speed, int repeat_interval)
{
  if (indication >= 0)
    in_redundancy = indication;
  if (low_speed >= 0)
    ls_redundancy = low_speed;
  if (high_speed >= 0)
    hs_redundancy = high_speed;

  redundancy = in_redundancy || ls_redundancy || hs_redundancy;

  re_interval = repeat_interval;

  PTRACE(3, "MyT38PseudoRTP::SetRedundancy" <<
            " indication=" << in_redundancy <<
            " low_speed=" << ls_redundancy <<
            " high_speed=" << hs_redundancy <<
            " repeat_interval=" << re_interval);
}

RTP_Session::SendReceiveStatus MyT38PseudoRTP::OnSendData(RTP_DataFrame & frame)
{
  if (frame.GetPayloadSize() == 0) {
    PTRACE(3, "MyT38PseudoRTP::OnSendData Empty packet");
    return e_IgnorePacket;
  }

  PTRACE(4, "MyT38PseudoRTP::OnSendData packet "
      << frame.GetSequenceNumber() << " size=" << frame.GetPayloadSize() << " " << frame.GetPayloadType());

  long packedSequenceNumber = (frame.GetSequenceNumber() & 0xFFFF) + (currentSequenceNumberSend & ~0xFFFFL);
  long lost = packedSequenceNumber - currentSequenceNumberSend;

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
    if (maxRedundancySend > 0) {
      maxRedundancySend--;
    }
    else
    if (currentSequenceNumberSend == 0 || re_interval <= 0 || (PTime() - lastTimeSend) < re_interval) {
      PTRACE(4, "MyT38PseudoRTP::OnSendData Repeated packet "
          << packedSequenceNumber << " (expected " << currentSequenceNumberSend << ")");
      return e_IgnorePacket;
    }
#if 1
    /*
     * Optimise repeated packet each time
     */
    if (redundancy) {
      T38_UDPTLPacket_error_recovery &recovery = udptlSend.m_error_recovery;
      if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
        int nRedundancy = secondary.GetSize();
        if (nRedundancy > maxRedundancySend)
          nRedundancy = maxRedundancySend;
        if (nRedundancy < 0)
          nRedundancy = 0;
        secondary.SetSize(nRedundancy);
      }
      else {
        PTRACE(3, "MyT38PseudoRTP::OnSendData Not implemented yet " << recovery.GetTagName());
      }
    }
#endif
#if PTRACING
    repeatedSend++;
#endif
  } else {
#if PTRACING
    if (lost > 0) {
      lostSend += lost;
      PTRACE(1, "MyT38PseudoRTP::OnSendData Losted packet(s) " << lost);
    }
#endif

    if (redundancy) {
      T38_UDPTLPacket_error_recovery &recovery = udptlSend.m_error_recovery;

      if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
        T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
        int nRedundancy = lost > 0 ? 0 : secondary.GetSize() + 1;

        if (nRedundancy > maxRedundancySend)
          nRedundancy = maxRedundancySend;
        if (nRedundancy < 0)
          nRedundancy = 0;
        secondary.SetSize(nRedundancy);

        if (nRedundancy > 0) {
          for (int i = nRedundancy - 1 ; i > 0 ; i--)
            secondary[i] = secondary[i - 1];
          secondary[0].SetValue(udptlSend.m_primary_ifp_packet.GetValue());
        }
      } else {
        PTRACE_IF(3, maxRedundancySend > 0,
            "MyT38PseudoRTP::OnSendData Not implemented yet " << recovery.GetTagName());
      }

      /*
       * Calculate maxRedundancySend for current ifp packet
       */

      T38_IFPPacket ifp;
      PASN_OctetString ifp_packet((const char *)frame.GetPayloadPtr(), frame.GetPayloadSize());

      ifp_packet.DecodeSubType(ifp);

      maxRedundancySend = hs_redundancy;

      switch( ifp.m_type_of_msg.GetTag() ) {
        case T38_Type_of_msg::e_t30_indicator:
          maxRedundancySend = in_redundancy;
          break;
        case T38_Type_of_msg::e_data:
          switch ((T38_Type_of_msg_data)ifp.m_type_of_msg) {
            case T38_Type_of_msg_data::e_v21:
              maxRedundancySend = ls_redundancy;
              break;
          }
          break;
      }
    }

    udptlSend.m_seq_number = frame.GetSequenceNumber();
    udptlSend.m_primary_ifp_packet.SetValue(PBYTEArray(frame.GetPayloadPtr(), frame.GetPayloadSize()));

    currentSequenceNumberSend = packedSequenceNumber + 1;
  }

#if 0
  // recovery test
  if (lost >= 0 && (currentSequenceNumberSend % 2)) {
    PTRACE(4, "MyT38PseudoRTP::OnSendData Skipped for recovery test PDU:\n"
              "  UDPTL = " << setprecision(2) << udptlSend);
    return e_IgnorePacket;
  }
#endif

  PPER_Stream rawData;
  udptlSend.Encode(rawData);
  rawData.CompleteEncoding();

#if PTRACING
      if (PTrace::CanTrace(4)) {
        PTRACE(4, "MyT38PseudoRTP::OnSendData Sending PDU:\n"
                  "  UDPTL = " << setprecision(2) << udptlSend << "\n"
                  "  " << setprecision(2) << rawData);
      }
      else {
        PTRACE(2, "MyT38PseudoRTP::OnSendData Sending PDU: seq=" << packedSequenceNumber);
      }
#endif

  // copy the UDPTL into the RTP packet
  frame.SetSize(rawData.GetSize());
  memcpy(frame.GetPointer(), rawData.GetPointer(), rawData.GetSize());

  lastTimeSend = PTime();

  return e_ProcessPacket;
}

RTP_Session::SendReceiveStatus MyT38PseudoRTP::GetFrame(RTP_DataFrame & frame)
{
  long lost = udptlSequenceNumberRecv - currentSequenceNumberRecv;

  if (lost < -0x10000L/2) {
    lost += 0x10000L;
    udptlSequenceNumberRecv += 0x10000L;
  }
  else
  if (lost > 0x10000L/2) {
    lost -= 0x10000L;
    udptlSequenceNumberRecv -= 0x10000L;
  }

  if (lost < 0) {
    PTRACE(4, "MyT38PseudoRTP::GetFrame Repeated packet "
        << udptlSequenceNumberRecv << " (expected " << currentSequenceNumberRecv << ")");
#if PTRACING
    repeatedRecv++;
#endif
    return e_IgnorePacket;
  }

  PASN_OctetString *ifp_packet;
#if PTRACING
  const char *recovered = "";
#endif

  if (lost > 0) {
    const T38_UDPTLPacket_error_recovery &recovery = udptlRecv.m_error_recovery;

    if (recovery.GetTag() == T38_UDPTLPacket_error_recovery::e_secondary_ifp_packets) {
      const T38_UDPTLPacket_error_recovery_secondary_ifp_packets &secondary = recovery;
      int nRedundancy = secondary.GetSize();

      if (lost > nRedundancy) {
        lost -= nRedundancy;
      } else {
        nRedundancy = lost;
        lost = 0;
      }

      if (nRedundancy > 0) {
        ifp_packet = &(PASN_OctetString &)secondary[nRedundancy - 1];
#if PTRACING
        recovered = "(recovered) ";
        recoveredRecv++;
#endif
      } else {
        ifp_packet = &(PASN_OctetString &)udptlRecv.m_primary_ifp_packet;
      }
    }
    else {
      PTRACE(3, "MyT38PseudoRTP::GetFrame Not implemented yet " << recovery.GetTagName());
      ifp_packet = &(PASN_OctetString &)udptlRecv.m_primary_ifp_packet;
    }

    if (lost) {
#if PTRACING
      lostRecv += lost;
#endif
      PTRACE(1, "MyT38PseudoRTP::GetFrame Losted packet(s) " << lost);
      currentSequenceNumberRecv += lost;
    }
  } else {
    ifp_packet = &(PASN_OctetString &)udptlRecv.m_primary_ifp_packet;
  }

  frame.SetPayloadSize(ifp_packet->GetDataLength());
  frame[0] = 0x80;
  frame.SetPayloadType(rtpPayloadType);
  frame.SetSequenceNumber(WORD(currentSequenceNumberRecv++ & 0xFFFF));
  memcpy(frame.GetPayloadPtr(), ifp_packet->GetPointer(), ifp_packet->GetDataLength());

  PTRACE(4, "MyT38PseudoRTP::GetFrame packet " << recovered
      << frame.GetSequenceNumber() << " size=" << frame.GetPayloadSize() << " " << frame.GetPayloadType());

  return RTP_UDP::OnReceiveData(frame);
}

RTP_Session::SendReceiveStatus MyT38PseudoRTP::OnReceiveData(RTP_DataFrame & frame)
{
  PPER_Stream rawData(frame.GetPointer(), frame.GetSize());

  if (udptlRecv.Decode(rawData)) {
    consecutiveBadPackets = 0;
    udptlSequenceNumberRecv = (udptlRecv.m_seq_number & 0xFFFF) + (currentSequenceNumberRecv & ~0xFFFFL);
  } else {
    consecutiveBadPackets++;

    PTRACE(2, "MyT38PseudoRTP::OnReceiveData Raw data decode failure:\n"
              "  " << setprecision(2) << rawData << "\n"
              "  UDPTL = " << setprecision(2) << udptlRecv);

    if (consecutiveBadPackets > 10) {
      PTRACE(1, "MyT38PseudoRTP::OnReceiveData Raw data decode failed "
          << consecutiveBadPackets << " times, aborting!");
      return e_AbortTransport;
    }

    return e_IgnorePacket;
  }

  PTRACE(4, "MyT38PseudoRTP::OnReceiveData Received PDU:\n"
            "  " << setprecision(2) << rawData << "\n"
            "  UDPTL = " << setprecision(2) << udptlRecv);

  return GetFrame(frame);
}

BOOL MyT38PseudoRTP::ReadData(RTP_DataFrame & frame, BOOL loop)
{
  do {
    BOOL do_read = (udptlSequenceNumberRecv < currentSequenceNumberRecv);
    int selectStatus = (do_read ? WaitForPDU(*dataSocket, *controlSocket, reportTimer) : -1);

    if (shutdownRead) {
      PTRACE(3, "T38_RTP\tSession " << sessionID << ", Read shutdown.");
      shutdownRead = FALSE;
      return FALSE;
    }

    switch (selectStatus) {
      case -2 :
        if (ReadControlPDU() == e_AbortTransport)
          return FALSE;
        break;

      case -3 :
        if (ReadControlPDU() == e_AbortTransport)
          return FALSE;
        // Then do -1 case

      case -1 :
        frame.SetSize(2048);  // set enough room for receiving UDPTL

        switch (do_read ? ReadDataPDU(frame) : GetFrame(frame)) {
          case e_ProcessPacket :
            if (shutdownRead)
              continue;
            return TRUE;
          case e_IgnorePacket :
            if (shutdownRead)
              continue;
            loop = FALSE;
            break;
          case e_AbortTransport :
            return FALSE;
        }
        break;

      case 0 :
        if (shutdownRead)
          continue;
        loop = FALSE;
        break;

      case PSocket::Interrupted:
        PTRACE(3, "T38_RTP\tSession " << sessionID << ", Interrupted.");
        return FALSE;

      default :
        PTRACE(1, "T38_RTP\tSession " << sessionID << ", Select error: "
                << PChannel::GetErrorText((PChannel::Errors)selectStatus));
        return FALSE;
    }
  } while (loop);

  // send a "repeated" packet with a "fake" payload of one byte of 0xFF

  frame.SetPayloadSize(1);
  frame[0] = 0x80;
  frame.SetPayloadType(rtpPayloadType);
  frame.SetSequenceNumber(WORD((currentSequenceNumberRecv - 1) & 0xFFFF));
  frame.GetPayloadPtr()[0] = 0xFF;

  return TRUE;
}
/////////////////////////////////////////////////////////////////////////////

