/*
 * sipep.cxx
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
 * $Log: sipep.cxx,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 *
 */

#include <ptlib.h>
#include <sip/sipcon.h>

#include "manager.h"
#include "ifpmediafmt.h"
#include "t38session.h"
#include "sipep.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
class MySIPConnection : public SIPConnection
{
  PCLASSINFO(MySIPConnection, SIPConnection);

  public:
  /**@name Construction */
  //@{
    MySIPConnection(
      OpalCall & call,                          ///<  Owner call for connection
      SIPEndPoint & endpoint,                   ///<  Owner endpoint for connection
      const PString & token,                    ///<  token to identify the connection
      const SIPURL & address,                   ///<  Destination address for outgoing call
      OpalTransport * transport,                ///<  Transport INVITE came in on
      unsigned int options = 0,                 ///<  Connection options
      OpalConnection::StringOptions * stringOptions = NULL  ///<  complex string options
    )
    : SIPConnection(call, endpoint, token, address, transport, options, stringOptions) {}
  //@}

    virtual PString GetDestinationAddress();

    virtual RTP_Session * CreateSession(
      const OpalTransport & transport,
      unsigned sessionID,
      RTP_QOS * rtpqos
    );

    virtual void AdjustMediaFormats(
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
    ) const;

    void AddMediaFormatList(const OpalMediaFormatList & list) { mediaFormatList += list; }

    BOOL RequestModeChangeT38();
    virtual void OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response);

  protected:
    OpalMediaFormatList mediaFormatList;
};
/////////////////////////////////////////////////////////////////////////////
//
// I try to implement an outgoing Re-INVITE here (temporary),
// because it has not been implemented in OPAL (yet).
//
class MySIPInvite : public SIPTransaction
{
    PCLASSINFO(MySIPInvite, SIPTransaction);
  public:
    MySIPInvite(
      SIPConnection & connection,
      OpalTransport & transport,
      RTP_SessionManager & sm
    );

    virtual BOOL OnReceivedResponse(SIP_PDU & response);
};
/////////////////////////////////////////////////////////////////////////////
//
//  Implementation
//
/////////////////////////////////////////////////////////////////////////////
PString MySIPEndPoint::ArgSpec()
{
  return
    "-no-sip."
    "-sip-old-asn."
    "-sip-redundancy:"
    "-sip-repeat:"
    "-sip-listen:"
    "-sip-no-listen."
  ;
}

PStringArray MySIPEndPoint::Descriptions()
{
  PStringArray descriptions = PString(
      "SIP options:\n"
      "  --no-sip                  : Disable SIP protocol.\n"
      "  --sip-old-asn             : Use original ASN.1 sequence in T.38 (06/98)\n"
      "                              Annex A (w/o CORRIGENDUM No. 1 fix).\n"
      "  --sip-redundancy I[L[H]]  : Set redundancy for error recovery for\n"
      "                              (I)ndication, (L)ow speed and (H)igh\n"
      "                              speed IFP packets.\n"
      "                              'I', 'L' and 'H' are digits.\n"
      "  --sip-repeat ms           : Continuously resend last UDPTL packet each ms\n"
      "                              milliseconds.\n"
      "  --sip-listen iface        : Interface/port(s) to listen for SIP requests\n"
      "                            : '*' is all interfaces (default tcp$*:5060 and\n"
      "                            : udp$*:5060).\n"
      "  --sip-no-listen           : Disable listen for incoming calls.\n"
  ).Lines();

  return descriptions;
}

BOOL MySIPEndPoint::Create(OpalManager & mgr, const PConfigArgs & args)
{
  if (args.HasOption("no-sip")) {
    cout << "Disabled SIP protocol" << endl;
    return TRUE;
  }

  if ((new MySIPEndPoint(mgr))->Initialise(args))
    return TRUE;

  return FALSE;
}

BOOL MySIPEndPoint::Initialise(const PConfigArgs & args)
{
  AddMediaFormatList(OpalG711_ULAW_64K);
  AddMediaFormatList(OpalG711_ALAW_64K);
  AddMediaFormatList(args.HasOption("sip-old-asn") ? OpalT38_IFP_PRE : OpalT38_IFP_COR);
  //AddMediaFormatList("UserInput/basicString");

  if (args.HasOption("sip-old-asn"))
    SetT38_IFP_PRE();

  if (args.HasOption("sip-redundancy")) {
    const char *r = args.GetOptionString("sip-redundancy");
    if (isdigit(r[0])) {
      in_redundancy = r[0] - '0';
      if (isdigit(r[1])) {
        ls_redundancy = r[1] - '0';
        if (isdigit(r[2])) {
          hs_redundancy = r[2] - '0';
        }
      }
    }
  }

  if (args.HasOption("sip-repeat"))
    re_interval = (int)args.GetOptionString("sip-repeat").AsInteger();

  if (!args.HasOption("sip-no-listen")) {
    PStringArray listeners;

    if (args.HasOption("sip-listen"))
      listeners = args.GetOptionString("sip-listen").Lines();
    else
      listeners = GetDefaultListeners();

    if (!StartListeners(listeners)) {
      cerr << "Could not open any SIP listener from "
           << setfill(',') << listeners << endl;
      return FALSE;
    }
    cout << "Waiting for incoming SIP calls from "
         << setfill(',') << listeners << endl;
  }

  return TRUE;
}

SIPConnection * MySIPEndPoint::CreateConnection(
    OpalCall & call,
    const PString & token,
    void * /*userData*/,
    const SIPURL & destination,
    OpalTransport * transport,
    SIP_PDU * /*invite*/,
    unsigned int options,
    OpalConnection::StringOptions * stringOptions)
{
  MySIPConnection * connection =
      new MySIPConnection(call, *this, token, destination, transport, options, stringOptions);

  connection->AddMediaFormatList(mediaFormatList);

  return connection;
}

void MySIPEndPoint::SetWriteInterval(
    OpalConnection &connection,
    const PTimeInterval &interval)
{
  ((MyManager &)GetManager()).SetWriteInterval(connection, interval);
}

BOOL MySIPEndPoint::RequestModeChangeT38(OpalConnection & connection)
{
  PAssert(PIsDescendant(&connection, MySIPConnection), PInvalidCast);

  return ((MySIPConnection &)connection).RequestModeChangeT38();
}
/////////////////////////////////////////////////////////////////////////////
PString MySIPConnection::GetDestinationAddress()
{
  PTRACE(2, "MySIPConnection::GetDestinationAddress " << localPartyAddress);

  PINDEX begNumber = localPartyAddress.Find(':');

  if (begNumber != P_MAX_INDEX)
    begNumber++;

  PINDEX endNumber = localPartyAddress.Find('@');

  if (endNumber != P_MAX_INDEX)
    endNumber--;

  PString number = localPartyAddress(begNumber, endNumber);

  return number;
}

RTP_Session * MySIPConnection::CreateSession(
    const OpalTransport & transport,
    unsigned sessionID,
    RTP_QOS * rtpqos)
{
  PTRACE(3, "MySIPConnection::CreateSession " << sessionID << " t=" << transport);

  if (sessionID == OpalMediaFormat::DefaultDataSessionID) {
    MySIPEndPoint &ep = (MySIPEndPoint &)GetEndPoint();

    PTimeInterval interval(PMaxTimeInterval);

    if (ep.InRedundancy() > 0 || ep.LsRedundancy() > 0 || ep.HsRedundancy() > 0)
      interval = 90;

    if (ep.ReInterval() > 0 && (interval > ep.ReInterval()))
      interval = ep.ReInterval();

    ep.SetWriteInterval(*this, interval);

    return
        ::CreateSessionT38(
            *this,
            transport,
            sessionID,
            rtpqos,
            RTP_DataFrame::DynamicBase,
            ep.InRedundancy(),
            ep.LsRedundancy(),
            ep.HsRedundancy(),
            ep.ReInterval());
  }

  return SIPConnection::CreateSession(transport, sessionID, rtpqos);
}

void MySIPConnection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  //PTRACE(3, "MySIPConnection::AdjustMediaFormats:\n" << setprecision(2) << mediaFormats);

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    BOOL found = FALSE;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++) {
      if (mediaFormats[i] == mediaFormatList[j]) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      PTRACE(3, "MySIPConnection::AdjustMediaFormats Remove " << mediaFormats[i]);
      mediaFormats.Remove(mediaFormats[i]);
      i--;
    }
  }

  SIPConnection::AdjustMediaFormats(mediaFormats);

  //PTRACE(3, "MySIPConnection::AdjustMediaFormats:\n" << setprecision(2) << mediaFormats);
}

BOOL MySIPConnection::RequestModeChangeT38()
{
  SIPTransaction * invite = new MySIPInvite(*this, *transport, rtpSessions);

  PTRACE(3, "MySIPConnection::RequestModeChangeT38 " << rtpSessions);

  if (!invite->Start()) {
    PTRACE(1, "MySIPConnection::RequestModeChangeT38 invite->Start() - fail");
    return FALSE;
  }

  {
    PWaitAndSignal m(invitationsMutex);
    invitations.Append(invite);
  }

  PTRACE(3, "MySIPConnection::RequestModeChangeT38 - OK");

  return TRUE;
}

void MySIPConnection::OnReceivedOK(SIPTransaction & transaction, SIP_PDU & response)
{
  PTRACE(3, "SIP\tReceived INVITE OK response");

  if (transaction.GetMethod() == SIP_PDU::Method_INVITE) {
    if (phase == EstablishedPhase && !IsConnectionOnHold()) {
      PWaitAndSignal m(streamsMutex);
      GetCall().RemoveMediaStreams();
      ReleaseSession(OpalMediaFormat::DefaultAudioSessionID, TRUE);
    }
  }

  SIPConnection::OnReceivedOK(transaction, response);
}

/////////////////////////////////////////////////////////////////////////////
MySIPInvite::MySIPInvite(SIPConnection & connection, OpalTransport & transport, RTP_SessionManager & sm)
  : SIPTransaction(connection, transport, Method_INVITE)
{
  mime.SetDate() ;                             // now
  mime.SetUserAgent(connection.GetEndPoint()); // normally 'OPAL/2.0'

  RTP_SessionManager rtpSessions = sm;

  connection.BuildSDP(sdp, rtpSessions, OpalMediaFormat::DefaultDataSessionID);
  connection.OnCreatingINVITE(*this);
}

BOOL MySIPInvite::OnReceivedResponse(SIP_PDU & response)
{
  PWaitAndSignal m(mutex);
	
  States originalState = state;

  if (!SIPTransaction::OnReceivedResponse(response))
    return FALSE;

  if (response.GetStatusCode()/100 == 1) {
    retryTimer.Stop();
    completionTimer = PTimeInterval(0, mime.GetExpires(180));
  } 
  else {
    completionTimer = endpoint.GetAckTimeout();

    // If the state was already 'Completed', ensure that still an
    // ACK is sent
    if (originalState >= Completed) {
      connection->SendACK(*this, response);
    }
  }

  /* Handle response to outgoing call cancellation */
  if (response.GetStatusCode() == Failure_RequestTerminated)
    SetTerminated(Terminated_Success);

  return TRUE;
}
/////////////////////////////////////////////////////////////////////////////

