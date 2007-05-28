/*
 * manager.cxx
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
 * $Log: manager.cxx,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 *
 */

#include <ptlib.h>

#include "../pmutils.h"
#include "h323ep.h"
#include "sipep.h"
#include "modemep.h"
#include "ifpmediafmt.h"
#include "manager.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
MyManager::MyManager()
{
  //autoStartTransmitFax = TRUE;
}

PString MyManager::ArgSpec()
{
  return
    MyH323EndPoint::ArgSpec() +
    MySIPEndPoint::ArgSpec() +
    ModemEndPoint::ArgSpec() +
    "-ports:"
    "-route:"
    "u-username:"
/*
    "D-disable:"
    "P-prefer:"
*/
  ;
}

PStringArray MyManager::Descriptions()
{
  PStringArray descriptions = PString(
      "Common options:\n"
      "  --ports T:B-M[,...]       : For (T)ype set (B)ase and (M)ax ports to use.\n"
      "                              T is 'udp', 'rtp' or 'tcp'. B and M are numbers.\n"
      "  --route pat=dst[,...]     : Route the incoming calls with destination address\n"
      "                              matching the regexp pat to the outgoing\n"
      "                              destination address dst.\n"
      "                              If dst contains '<dn>', it will be replaced by a\n"
      "                              destination number. To strip N first digits from\n"
      "                              number use '<dn!N>' form.\n"
      "                              If the specification is of the form @filename,\n"
      "                              then the file is read with each line consisting\n"
      "                              of a pat=dst route specification.\n"
      "  -u --username str         : Set the default username to str.\n"
/*
      "  -D --disable codec        : Disable the specified codec.\n"
      "                              Can be used multiple times.\n"
      "  -P --prefer codec         : Prefer the specified codec.\n"
      "                              Can be used multiple times.\n"
*/
      "\n"
  ).Lines();

  descriptions += MyH323EndPoint::Descriptions();
  descriptions.Append(new PString(""));
  descriptions += MySIPEndPoint::Descriptions();
  descriptions.Append(new PString(""));
  descriptions += ModemEndPoint::Descriptions();

  return descriptions;
}

BOOL MyManager::Initialise(const PConfigArgs & args)
{
  DisableDetectInBandDTMF(TRUE);
  silenceDetectParams.m_mode = OpalSilenceDetector::NoSilenceDetection;

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

  if (!ModemEndPoint::Create(*this, args))
    return FALSE;

  if (!MyH323EndPoint::Create(*this, args))
    return FALSE;

  if (!MySIPEndPoint::Create(*this, args))
    return FALSE;

  if (args.HasOption("route")) {
    SetRouteTable(args.GetOptionString("route").Tokenise(",\r\n", FALSE));

    cout << "Route table:" << endl;

    const RouteTable &routeTable = GetRouteTable();

    for (PINDEX i=0 ; i < routeTable.GetSize() ; i++) {
      cout << "  " << routeTable[i].pattern << "=" << routeTable[i].destination << endl;
    }
  }

  return TRUE;
}

void MyManager::SetWriteInterval(
    OpalConnection &connection,
    const PTimeInterval &interval)
{
  PSafePtr<OpalConnection> pOtherConn = connection.GetCall().GetOtherPartyConnection(connection);

  if (pOtherConn != NULL) {
    if (PIsDescendant(&pOtherConn->GetEndPoint(), ModemEndPoint))
      ((ModemEndPoint &)pOtherConn->GetEndPoint()).SetReadTimeout(*pOtherConn, interval);
  }
}

PString MyManager::OnRouteConnection(OpalConnection & connection)
{
  PString src = connection.GetRemotePartyCallbackURL();
  PString dst = OpalManager::OnRouteConnection(connection);
  OpalCall &call = connection.GetCall();
  const PString &token = call.GetToken();
  PString addr = call.GetPartyB();

  if (addr.IsEmpty())
    addr = connection.GetDestinationAddress();

  if (!dst.IsEmpty()) {
    cout << "Call[" << token << "] from " << src << " to " << addr << ", route to " << dst << endl;
    PTRACE(1, "Call[" << token << "] from " << src << " to " << addr << ", route to " << dst);
  } else {
    cout << "Call[" << token << "] from " << src << " to " << addr << ", no route!" << endl;
    PTRACE(1, "Call[" << token << "] from " << src << " to " << addr << ", no route!");
  }

  return dst;
}

void MyManager::OnClearedCall(OpalCall & call)
{
  cout << "Call[" << call.GetToken() << "] cleared" << endl;
  PTRACE(1, "Call[" << call.GetToken() << "] cleared");

  OpalManager::OnClearedCall(call);
}

BOOL MyManager::OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream)
{
  OpalCall &call = connection.GetCall();

  cout << "Open " << stream << " for Call[" << call.GetToken() << "]" << endl;
  return OpalManager::OnOpenMediaStream(connection, stream);
}

void MyManager::OnClosedMediaStream(const OpalMediaStream & stream)
{
  cout << "Close " << stream << endl;
  OpalManager::OnClosedMediaStream(stream);
}

PString MyManager::ApplyRouteTable(const PString & proto, const PString & addr)
{
  PString destination = OpalManager::ApplyRouteTable(proto, addr);

  PINDEX pos;

  if ((pos = destination.Find("<dn!")) != P_MAX_INDEX) {
    PINDEX strip_num_len = ::strspn((const char *)destination + pos + 4, "0123456789");

    if (destination[pos + 4 + strip_num_len] == '>') {
      PINDEX strip_num = (PINDEX)destination.Mid(pos + 4, strip_num_len).AsInteger();

      destination.Splice(addr.Left(::strspn(addr, "0123456789*#")).Mid(strip_num), pos, 4 + strip_num_len + 1);
    }
  }

  return destination;
}

BOOL MyManager::OnRequestModeChange(
    OpalConnection & connection,
    const OpalMediaFormatList & /*mediaFormatList*/)
{
  PSafePtr<OpalConnection> pOtherConn = connection.GetCall().GetOtherPartyConnection(connection);

  if (pOtherConn != NULL) {
    if (PIsDescendant(&pOtherConn->GetEndPoint(), MyH323EndPoint)) {
      if (((MyH323EndPoint &)pOtherConn->GetEndPoint()).RequestModeChangeT38(*pOtherConn)) {
        myPTRACE(2, "MyManager::RequestModeChange RequestMode T38 - OK for " << connection);
        return TRUE;
      }
    }
    else
    if (PIsDescendant(&pOtherConn->GetEndPoint(), MySIPEndPoint)) {
      if (((MySIPEndPoint &)pOtherConn->GetEndPoint()).RequestModeChangeT38(*pOtherConn)) {
        myPTRACE(2, "MyManager::RequestModeChange RequestMode T38 - OK for " << connection);
        return TRUE;
      }
    }
  }

  myPTRACE(1, "MyManager::RequestModeChange RequestMode T38 - fail for " << connection);
  return FALSE;
}
/////////////////////////////////////////////////////////////////////////////

