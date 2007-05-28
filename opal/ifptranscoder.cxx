/*
 * ifptranscoder.cxx
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
 * $Log: ifptranscoder.cxx,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 *
 */

#include <ptlib.h>
#include <opal/transcoders.h>
#include <asn/t38.h>

#include "../pmutils.h"
#include "ifpmediafmt.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
class T38_IFPTranscoder : public OpalTranscoder {
  public:
    T38_IFPTranscoder(
      const OpalMediaFormat & inputMediaFormat,  ///<  Input media format
      const OpalMediaFormat & outputMediaFormat, ///<  Output media format
      BOOL _cor2pre)
    : OpalTranscoder(inputMediaFormat, outputMediaFormat),
      cor2pre(_cor2pre) {}

    virtual PINDEX GetOptimalDataFrameSize(
      BOOL /*input*/      ///<  Flag for input or output data size
    ) const { return 0; }

    virtual BOOL Convert(
      const RTP_DataFrame & input,  ///<  Input data
      RTP_DataFrame & output        ///<  Output data
    );

  protected:
    BOOL cor2pre;
};

class T38_IFP_Cor_Pre : public T38_IFPTranscoder {
  public:
    T38_IFP_Cor_Pre() : T38_IFPTranscoder(OpalT38_IFP_COR, OpalT38_IFP_PRE, TRUE) {}
};

class T38_IFP_Pre_Cor : public T38_IFPTranscoder {
  public:
    T38_IFP_Pre_Cor() : T38_IFPTranscoder(OpalT38_IFP_PRE, OpalT38_IFP_COR, FALSE) {}
};
/////////////////////////////////////////////////////////////////////////////
static BOOL IfpTranscode(
    BOOL cor2pre,
    const PASN_OctetString &ifp_packet_src,
    PASN_OctetString &ifp_packet_dst)
{
  if (ifp_packet_src.GetDataLength() == 1) {
    // "fake" packet or packet w/o data_field
    ifp_packet_dst = ifp_packet_src;
  }
  else
  if (cor2pre) {
    T38_IFPPacket ifp;

    if (!ifp_packet_src.DecodeSubType(ifp)) {
      PTRACE(2, "IfpTranscode Corrigendum IFP decode failure: "
          << PRTHEX(PBYTEArray(ifp_packet_src)) << "\n  ifp = "
          << setprecision(2) << ifp);
      return FALSE;
    }

    if (ifp.HasOptionalField(T38_IFPPacket::e_data_field)) {
      PINDEX count = ifp.m_data_field.GetSize();

      for( PINDEX i = 0 ; i < count ; i++ ) {
        ifp.m_data_field[i].m_field_type.SetExtendable(FALSE);
      }
      ifp_packet_dst.EncodeSubType(ifp);
    } else {
      ifp_packet_dst = ifp_packet_src;
    }
  }
  else {
    T38_PreCorrigendum_IFPPacket ifp;

    if (!ifp_packet_src.DecodeSubType(ifp)) {
      PTRACE(2, "IfpTranscode Pre corrigendum IFP decode failure: "
          << PRTHEX(PBYTEArray(ifp_packet_src)) << "\n  ifp = "
          << setprecision(2) << ifp);
      return FALSE;
    }

    if (ifp.HasOptionalField(T38_PreCorrigendum_IFPPacket::e_data_field)) {
      PINDEX count = ifp.m_data_field.GetSize();

      for( PINDEX i = 0 ; i < count ; i++ ) {
        ifp.m_data_field[i].m_field_type.SetExtendable(TRUE);
      }
      ifp_packet_dst.EncodeSubType(ifp);
    } else {
      ifp_packet_dst = ifp_packet_src;
    }
  }

  PTRACE(5, (cor2pre ? "IfpTranscode COR->PRE" : "PRE->COR")
      << hex << setfill('0') << ": {\n"
      << PBYTEArray(ifp_packet_src)
      << " } -> {\n"
      << PBYTEArray(ifp_packet_dst)
      << " }" << dec << setfill(' '));

  return TRUE;
}

BOOL T38_IFPTranscoder::Convert(const RTP_DataFrame & input, RTP_DataFrame & output)
{
  PASN_OctetString ifp_packet_src((const char *)input.GetPayloadPtr(), input.GetPayloadSize());
  PASN_OctetString ifp_packet_dst;

  if (!IfpTranscode(cor2pre, ifp_packet_src, ifp_packet_dst))
    return FALSE;

  output.SetPayloadSize(ifp_packet_dst.GetDataLength());
  memcpy(output.GetPayloadPtr(), ifp_packet_dst.GetPointer(), ifp_packet_dst.GetDataLength());
  output.SetSequenceNumber(input.GetSequenceNumber());

  return TRUE;
}
/////////////////////////////////////////////////////////////////////////////
OPAL_REGISTER_TRANSCODER(T38_IFP_Cor_Pre, OpalT38_IFP_COR, OpalT38_IFP_PRE);
OPAL_REGISTER_TRANSCODER(T38_IFP_Pre_Cor, OpalT38_IFP_PRE, OpalT38_IFP_COR);
/////////////////////////////////////////////////////////////////////////////

