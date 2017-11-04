/*
 * manager.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2011 Vyacheslav Frolov
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
 * $Log: manager.cxx,v $
 * Revision 1.16  2011-01-17 08:33:17  vfrolov
 * Added --displayname option
 *
 * Revision 1.16  2011/01/17 08:33:17  vfrolov
 * Added --displayname option
 *
 * Revision 1.15  2010/07/09 13:18:13  vfrolov
 * Fixed help message
 *
 * Revision 1.14  2010/07/08 11:40:18  vfrolov
 * Fixed route message for sip
 * Added call end reason to call cleared message
 * Added support for multiple <dn!N> per route
 *
 * Revision 1.13  2010/03/15 13:40:27  vfrolov
 * Removed unused code
 *
 * Revision 1.12  2010/02/24 14:20:10  vfrolov
 * Added variant of patch #2954967 "opal sip/h323 build-time detection"
 * Thanks Mariusz Mazur
 *
 * Revision 1.11  2010/02/12 08:55:07  vfrolov
 * Implemented fake codecs
 *
 * Revision 1.10  2009/12/23 17:53:00  vfrolov
 * Deprecated route comma delimiter
 *
 * Revision 1.9  2009/11/10 11:30:57  vfrolov
 * Implemented G.711 fallback to fax pass-through mode
 *
 * Revision 1.8  2009/07/31 17:34:40  vfrolov
 * Removed --h323-old-asn and --sip-old-asn options
 *
 * Revision 1.7  2009/07/22 14:42:49  vfrolov
 * Added Descriptions(args) to endpoints
 *
 * Revision 1.6  2009/07/15 13:23:20  vfrolov
 * Added Descriptions(args)
 *
 * Revision 1.5  2009/01/26 15:25:36  vfrolov
 * Added --stun option
 *
 * Revision 1.4  2009/01/15 08:46:34  vfrolov
 * Fixed OnRouteConnection() be able to compile with OPAL trunk since 21925
 *
 * Revision 1.3  2008/09/11 07:45:09  frolov
 * Fixed compiler warnings
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>

#include <opal_config.h>

#include "../pmutils.h"

#if OPAL_H323
#include "h323ep.h"
#endif

#if OPAL_SIP
#include "sipep.h"
#endif

#include "modemep.h"
#include "manager.h"
#include "fake_codecs.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
MyManager::MyManager()
{
  //autoStartTransmitFax = TRUE;
}

PString MyManager::ArgSpec()
{
  return
#if OPAL_H323
    MyH323EndPoint::ArgSpec() +
#endif
#if OPAL_SIP
    MySIPEndPoint::ArgSpec() +
#endif
    ModemEndPoint::ArgSpec() +
    "-ports:"
    "-route:"
    "u-username:"
    "-displayname:"
    "-stun:"
    "-fake-audio:"
  ;
}

PStringArray MyManager::Descriptions()
{
  PStringArray descriptions = PString(
      "Common options:\n"
      "  --ports T:B-M[,...]       : For (T)ype set (B)ase and (M)ax ports to use.\n"
      "                              T is 'udp', 'rtp' or 'tcp'. B and M are numbers.\n"
      "  --route pat=dst[;option[=value][;...]]\n"
      "                            : Route the calls with incoming destination address\n"
      "                              matching the regexp pat to the outgoing\n"
      "                              destination address dst.\n"
      "                              All '<dn>' meta-strings found in dst or in\n"
      "                              following route options will be replaced by all\n"
      "                              valid consecutive E.164 digits from the incoming\n"
      "                              destination address. To strip N first digits use\n"
      "                              '<dn!N>' meta-string.\n"
      "                              If the specification is of the form @filename,\n"
      "                              then the file is read with each line consisting\n"
      "                              of a pat=dst[;...] route specification.\n"
      "  -u --username str         : Set the default username to str.\n"
      "  --displayname str         : Set the default display name to str.\n"
      "                              Can be overridden by route option\n"
      "                                OPAL-" OPAL_OPT_CALLING_DISPLAY_NAME "=str\n"
      "  --stun server             : Set STUN server.\n"
      "  --fake-audio [!]wildcard[,[!]...]\n"
      "                            : Register the fake audio format(s) matching the\n"
      "                              wildcard(s). The '*' character match any\n"
      "                              substring. The leading '!' character indicates\n"
      "                              a negative test.\n"
      "                              May be used multiple times.\n"
  ).Lines();

  PStringArray arr[] = {
#if OPAL_H323
    MyH323EndPoint::Descriptions(),
#endif
#if OPAL_SIP
    MySIPEndPoint::Descriptions(),
#endif
    ModemEndPoint::Descriptions(),
  };

  for (PINDEX i = 0 ; i < PINDEX(sizeof(arr)/sizeof(arr[0])) ; i++) {
    if (arr[i].GetSize() > 0) {
      descriptions.Append(new PString(""));
      descriptions += arr[i];
    }
  }

  return descriptions;
}

PStringArray MyManager::Descriptions(const PConfigArgs & args)
{
  if (args.HasOption("fake-audio")) {
    PStringStream s;

    s << setfill(',') << args.GetOptionString("fake-audio").Lines();

    FakeCodecs::RegisterFakeAudioFormats(s.Tokenise(",", FALSE));
  }

  PStringArray descriptions;
  PBoolean first = TRUE;

  PStringArray arr[] = {
#if OPAL_H323
    MyH323EndPoint::Descriptions(args),
#endif
#if OPAL_SIP
    MySIPEndPoint::Descriptions(args),
#endif
    ModemEndPoint::Descriptions(args),
  };

  for (PINDEX i = 0 ; i < PINDEX(sizeof(arr)/sizeof(arr[0])) ; i++) {
    if (arr[i].GetSize() > 0) {
      if (!first)
        descriptions.Append(new PString(""));
      else
        first = FALSE;

      descriptions += arr[i];
    }
  }

  return descriptions;
}

PBoolean MyManager::Initialise(const PConfigArgs & args)
{
  cout << "\n" << args << "\n";;

  PTRACE_INITIALISE(args);

  DisableDetectInBandDTMF(TRUE);
  m_silenceDetectParams.m_mode = OpalSilenceDetector::NoSilenceDetection;

  if (args.HasOption("ports")) {
    PString p = args.GetOptionString("ports");
    PStringArray ports = p.Tokenise(",\r\n", FALSE);

    for (PINDEX i = 0 ; i < ports.GetSize() ; i++) {
      p = ports[i];
      PStringArray ps = p.Tokenise(":-", FALSE);
      if (ps.GetSize() == 3) {
        if (ps[0] == "udp")
          SetUDPPorts(ps[1].AsUnsigned(), ps[2].AsUnsigned());
        else
        if (ps[0] == "rtp")
          SetRtpIpPorts(ps[1].AsUnsigned(), ps[2].AsUnsigned());
        else
        if (ps[0] == "tcp")
          SetTCPPorts(ps[1].AsUnsigned(), ps[2].AsUnsigned());
      }
    }
    PTRACE(1, "UDP ports: " << GetUDPPortBase() << "-" << GetUDPPortMax());
    PTRACE(1, "RTP ports: " << GetRtpIpPortBase() << "-" << GetRtpIpPortMax());
    PTRACE(1, "TCP ports: " << GetTCPPortBase() << "-" << GetTCPPortMax());
  }

  SetDefaultUserName(
    args.HasOption("username") ?
      args.GetOptionString("username") :
      PProcess::Current().GetName() + " v" + PProcess::Current().GetVersion()
  );

  if (args.HasOption("displayname"))
    SetDefaultDisplayName(args.GetOptionString("displayname"));

  //if (args.HasOption("stun"))
  //  SetSTUNServer(args.GetOptionString("stun"));

  //if (stun != NULL) {
  //  cout << "STUN server \"" << stun->GetServer() << "\" replies " << stun->GetNatTypeName();

  //  PIPSocket::Address externalAddress;

  //  if (stun->GetExternalAddress(externalAddress))
  //    cout << ", external IP " << externalAddress;

  //  cout << endl;
  //}

  if (!ModemEndPoint::Create(*this, args))
    return FALSE;

#if OPAL_H323
  if (!MyH323EndPoint::Create(*this, args))
    return FALSE;
#endif

#if OPAL_SIP
  if (!MySIPEndPoint::Create(*this, args))
    return FALSE;
#endif

  /*
   We only support G.711 and T.38 internally, so make sure no other codecs get
   offered on outbound calls
   */
  // Removing these two lines so we can support the Fake Codecs.
  // static char const *FormatMask[] = { "!G.711*", "!@fax", "!UserInput/RFC2833", "!NamedSignalEvent" };
  // SetMediaFormatMask(PStringArray(PARRAYSIZE(FormatMask), FormatMask));

  if (args.HasOption("route")) {
    SetRouteTable(args.GetOptionString("route").Tokenise("\r\n", FALSE));

    cout << "Route table:" << endl;

    const RouteTable &routeTable = GetRouteTable();

    for (PINDEX i=0 ; i < routeTable.GetSize() ; i++) {
      cout << "  " << routeTable[i].GetPartyA() << "," << routeTable[i].GetPartyB() << "=" << routeTable[i].GetDestination() << endl;
    }
  }

  SetRtpIpPorts(10000,10999);  // for Verizon Certification

  return TRUE;
}

bool MyManager::OnRouteConnection(PStringSet & routesTried,
                                  const PString & a_party,
                                  const PString & b_party,
                                  OpalCall & call,
                                  unsigned options,
                                  OpalConnection::StringOptions * stringOptions)
{
  const PString &token = call.GetToken();

  if (!OpalManager::OnRouteConnection(routesTried, a_party, b_party, call, options, stringOptions)) {
    cout << "Call[" << token << "] from " << a_party << " to " << b_party << ", no route!" << endl;
    PTRACE(1, "Call[" << token << "] from " << a_party << " to " << b_party << ", no route!");
    return false;
  }

  PString dst;
  PSafePtr<OpalConnection> dst_conn = call.GetConnection(1);

  if (dst_conn != NULL) {
    dst = dst_conn->GetRemotePartyURL();

    if (dst.NumCompare(dst_conn->GetPrefixName() + ":") != EqualTo)
      dst = dst_conn->GetPrefixName() + ":" + dst;
  }

  cout << "Call[" << token << "] from " << a_party << " to " << b_party << ", route to " << dst << endl;
  PTRACE(1, "Call[" << token << "] from " << a_party << " to " << b_party << ", route to " << dst);

  return true;
}

void MyManager::OnClearedCall(OpalCall & call)
{
  cout << "Call[" << call.GetToken() << "] cleared (" << call.GetCallEndReasonText() << ")" << endl;
  PTRACE(1, "Call[" << call.GetToken() << "] cleared (" << call.GetCallEndReason() << ")");

  OpalManager::OnClearedCall(call);
}

// Here we check the routes as they get added to see if there are any
// options that we care about.
PBoolean MyManager::AddRouteEntry(const PString & spec)
{
  PStringArray RouteParts = spec.Tokenise(":");
  PString RouteType = RouteParts[0];
  RouteParts = spec.Tokenise(";");
  for (PINDEX i = 0; i < RouteParts.GetSize(); i++) {
    PStringArray RouteSubParts = RouteParts[i].Tokenise("=");
    if (RouteType == "sip") {
      if (RouteSubParts[0] == "OPAL-Disable-T38-Mode") {
        if ((RouteSubParts.GetSize() == 1) || (RouteSubParts[1]=="true")) {
          cout << "Disable-T38-Mode on sip route" << endl;
          MySIPEndPoint::defaultStringOptions.SetAt("Disable-T38-Mode", "true");
        }
      }
      else if (RouteSubParts[0] == "OPAL-Enable-Audio") {
        // For Enable-Audio we have to Register the Fake Codecs
        // and make sure we always include G.711
        if (RouteSubParts.GetSize() == 2) {
          cout << "Enable-Audio=" << RouteSubParts[1] << " on sip route" << endl;
          FakeCodecs::RegisterFakeAudioFormats(RouteSubParts[1].Tokenise(",", FALSE));
          MySIPEndPoint::defaultStringOptions.SetAt("Enable-Audio","G.711*,"+RouteSubParts[1]);
        }
      }
    }
    else if (RouteType == "h323") {
      if (RouteSubParts[0] == "OPAL-Disable-T38-Mode") {
        if ((RouteSubParts.GetSize() == 1) || (RouteSubParts[1]=="true")) {
          cout << "Disable-T38-Mode on h323 route" << endl;
          MyH323EndPoint::defaultStringOptions.SetAt("Disable-T38-Mode", "true");
        }
      }
      else if (RouteSubParts[0] == "OPAL-Enable-Audio") {
        // For Enable-Audio we have to Register the Fake Codecs
        // and make sure we always include G.711
        if (RouteSubParts.GetSize() == 2) {
          cout << "Enable-Audio=" << RouteSubParts[1] << " on h323 route" << endl;
          FakeCodecs::RegisterFakeAudioFormats(RouteSubParts[1].Tokenise(",", FALSE));
          MyH323EndPoint::defaultStringOptions.SetAt("Enable-Audio","G.711*,"+RouteSubParts[1]);
        }
      }
    }
    else if (RouteType == "modem") {
      if (RouteSubParts[0] == "OPAL-Force-Fax-Mode") {
        if ((RouteSubParts.GetSize() == 1) || (RouteSubParts[1]=="true")) {
          cout << "Force-Fax-Mode on modem route" << endl;
          ModemEndPoint::defaultStringOptions.SetAt("Force-Fax-Mode", "true");
        }
      }
      else if (RouteSubParts[0] == "OPAL-Force-Fax-Mode-Delay") {
        if (RouteSubParts.GetSize() == 2) {
          cout << "Force-Fax-Mode-Delay=" << RouteSubParts[1] << " on modem route" << endl;
          ModemEndPoint::defaultStringOptions.SetAt("Force-Fax-Mode-Delay",RouteSubParts[1]);
        }
      }
      else if (RouteSubParts[0] == "OPAL-No-Force-T38-Mode") {
        if ((RouteSubParts.GetSize() == 1) || (RouteSubParts[1]=="true")) {
          cout << "No-Force-T38-Mode on modem route" << endl;
          ModemEndPoint::defaultStringOptions.SetAt("No-Force-T38-Mode", "true");
        }
      }
    }
  }

  return OpalManager::AddRouteEntry(spec);
}

PBoolean MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  OpalCall &call = connection.GetCall();

  cout << "Open " << stream << " for Call[" << call.GetToken() << "]" << endl;
  return OpalManager::OnOpenMediaStream(connection, stream);
}

void MyManager::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalCall &call = stream.GetConnection().GetCall();

  cout << "Close " << stream << " for Call[" << call.GetToken() << "]" << endl;
  OpalManager::OnClosedMediaStream(stream);
}

PString MyManager::ApplyRouteTable(const PString & proto, const PString & addr, PINDEX & routeIndex)
{
  PString destination = OpalManager::ApplyRouteTable(proto, addr, routeIndex);

  PINDEX pos = 0;

  while ((pos = destination.Find("<dn!", pos)) != P_MAX_INDEX) {
    PINDEX strip_num_len = (PINDEX)::strspn((const char *)destination + pos + 4, "0123456789");

    if (destination[pos + 4 + strip_num_len] == '>') {
      PINDEX strip_num = (PINDEX)destination.Mid(pos + 4, strip_num_len).AsInteger();

      destination.Splice(addr.Left((PINDEX)::strspn(addr, "0123456789*#")).Mid(strip_num), pos, 4 + strip_num_len + 1);
    } else {
      pos++;
    }
  }

  return destination;
}
/////////////////////////////////////////////////////////////////////////////

