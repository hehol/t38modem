/*
 * sipep.cxx
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
 * $Log: sipep.cxx,v $
 * Revision 1.6  2009-07-06 08:30:59  vfrolov
 * Fixed typo. Thanks Dmitry (gorod225)
 *
 * Revision 1.6  2009/07/06 08:30:59  vfrolov
 * Fixed typo. Thanks Dmitry (gorod225)
 *
 * Revision 1.5  2009/05/29 13:01:40  vfrolov
 * Ported to OPAL trunk
 *
 * Revision 1.4  2009/04/07 12:49:18  vfrolov
 * Implemented --sip-proxy and --sip-register options
 *
 * Revision 1.3  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.2  2007/07/20 14:34:45  vfrolov
 * Added setting of calling number of an outgoing connection
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#include <sip/sipcon.h>

#include "manager.h"
#include "ifpmediafmt.h"
#include "sipep.h"
#include "opalutils.h"

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

    virtual PBoolean SetUpConnection();

    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual void AdjustMediaFormats(
      OpalMediaFormatList & mediaFormats        ///<  Media formats to use
    ) const;

    void AddMediaFormatList(const OpalMediaFormatList & list) { mediaFormatList += list; }

  protected:
    OpalMediaFormatList mediaFormatList;
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
    /*
    "-sip-redundancy:"
    "-sip-repeat:"
    */
    "-sip-proxy:"
    "-sip-register:"
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
      /*
      "  --sip-redundancy I[L[H]]  : Set redundancy for error recovery for\n"
      "                              (I)ndication, (L)ow speed and (H)igh\n"
      "                              speed IFP packets.\n"
      "                              'I', 'L' and 'H' are digits.\n"
      "  --sip-repeat ms           : Continuously resend last UDPTL packet each ms\n"
      "                              milliseconds.\n"
      */
      "  --sip-proxy [user:[pwd]@]host\n"
      "                            : Proxy information.\n"
      "  --sip-register [user@]registrar[,pwd[,contact[,realm[,authID]]]]\n"
      "                            : Registration information. Can be used multiple\n"
      "                              times.\n"
      "  --sip-listen iface        : Interface/port(s) to listen for SIP requests\n"
      "                            : '*' is all interfaces (default tcp$*:5060 and\n"
      "                            : udp$*:5060).\n"
      "  --sip-no-listen           : Disable listen for incoming calls.\n"
  ).Lines();

  return descriptions;
}

PBoolean MySIPEndPoint::Create(OpalManager & mgr, const PConfigArgs & args)
{
  if (args.HasOption("no-sip")) {
    cout << "Disabled SIP protocol" << endl;
    return TRUE;
  }

  if ((new MySIPEndPoint(mgr))->Initialise(args))
    return TRUE;

  return FALSE;
}

PBoolean MySIPEndPoint::Initialise(const PConfigArgs & args)
{
  AddMediaFormatList(OpalG711_ULAW_64K);
  AddMediaFormatList(OpalG711_ALAW_64K);
  AddMediaFormatList(args.HasOption("sip-old-asn") ? OpalT38_IFP_PRE : OpalT38_IFP_COR);
  //AddMediaFormatList("UserInput/basicString");

  if (args.HasOption("sip-old-asn"))
    SetT38_IFP_PRE();

  /*
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
  */

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

  if (args.HasOption("sip-proxy"))
    SetProxy(args.GetOptionString("sip-proxy"));

  if (args.HasOption("sip-register")) {
    PString r = args.GetOptionString("sip-register");
    PStringArray regs = r.Tokenise("\r\n", FALSE);

    for (PINDEX i = 0 ; i < regs.GetSize() ; i++) {
      PStringArray prms = regs[i].Tokenise(",", TRUE);

      PAssert(prms.GetSize() >= 1, "empty registration information");

      if (prms.GetSize() >= 1) {
        SIPRegister::Params params;

        params.m_addressOfRecord = prms[0];

        if (prms.GetSize() >= 2) {
          params.m_password = prms[1];
          
          if (prms.GetSize() >= 3) {
            params.m_contactAddress = prms[2];

            if (prms.GetSize() >= 4) {
              params.m_realm = prms[3];

              if (prms.GetSize() >= 5) {
                params.m_authID = prms[4];
              }
            }
          }
        }

        params.m_expire = 300;

        if (!Register(params, regs[i])) {
          cerr << "Could not start SIP registration to " << params.m_addressOfRecord << endl;
          return FALSE;
        }
      }
    }
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
  PTRACE(2, "MySIPEndPoint::CreateConnection for " << call);

  MySIPConnection * connection =
      new MySIPConnection(call, *this, token, destination, transport, options, stringOptions);

  connection->AddMediaFormatList(mediaFormatList);

  return connection;
}

/*
void MySIPEndPoint::SetWriteInterval(
    OpalConnection &connection,
    const PTimeInterval &interval)
{
  ((MyManager &)GetManager()).SetWriteInterval(connection, interval);
}
*/

struct RequestModeChangeArgs {
  RequestModeChangeArgs(
    OpalEndPoint &_endpoint,
    const PString &_connectionToken,
    const OpalMediaFormat &_mediaFormat)
  : endpoint(_endpoint),
    connectionToken(_connectionToken),
    mediaFormat(_mediaFormat)
  {}

  OpalEndPoint &endpoint;
  PString connectionToken;
  OpalMediaFormat mediaFormat;
};

static void RequestModeChange(RequestModeChangeArgs args)
{
  PSafePtr<OpalConnection> connection = args.endpoint.GetConnectionWithLock(args.connectionToken);

  if (connection == NULL) {
    PTRACE(1, "::RequestModeChange Cannot get connection " << args.connectionToken);
    return;
  }

  if (!connection->GetCall().OpenSourceMediaStreams(*connection,
                                                    args.mediaFormat.GetMediaType(),
                                                    1,
                                                    args.mediaFormat))
  {
    PTRACE(1, "::RequestModeChange(" << *connection << ", " << args.mediaFormat << ")"
              " OpenSourceMediaStreams() - failed");
    return;
  }

  PTRACE(4, "::RequestModeChange(" << *connection << ", " << args.mediaFormat << ") - OK");
}

PBoolean MySIPEndPoint::RequestModeChange(
    OpalConnection & connection,
    const OpalMediaType & mediaType)
{
  if (mediaType != OpalMediaType::Fax())
    return PFalse;

  for (PINDEX i = 0 ; i < mediaFormatList.GetSize() ; i++) {
    if (mediaFormatList[i].GetMediaType() == mediaType) {
      new PThread1Arg<RequestModeChangeArgs>(
              RequestModeChangeArgs(*this, connection.GetToken(), mediaFormatList[i]),
              ::RequestModeChange,
              true);

      return PTrue;
    }
  }

  return PFalse;
}
/////////////////////////////////////////////////////////////////////////////
PBoolean MySIPConnection::SetUpConnection()
{
  PTRACE(2, "MySIPConnection::SetUpConnection " << *this << " name=" << GetLocalPartyName());

  PSafePtr<OpalConnection> conn = GetCall().GetConnection(0);

  if (conn != NULL && conn != this) {
    // Set the calling number of an outgoing connection

    PString name = conn->GetRemotePartyNumber();

    if (!name.IsEmpty() && name != "*") {
      SetLocalPartyName(name);

      PTRACE(1, "MySIPConnection::SetUpConnection new name=" << GetLocalPartyName());
    }
  }

  return SIPConnection::SetUpConnection();
}

OpalMediaFormatList MySIPConnection::GetMediaFormats() const
{
  PTRACE(4, "MySIPConnection::GetMediaFormats remoteFormatList=\n"
         << setfill('\n') << remoteFormatList << setfill(' '));

  OpalMediaFormatList list = SIPConnection::GetMediaFormats();

  PTRACE(4, "MySIPConnection::GetMediaFormats list=\n"
         << setfill('\n') << list << setfill(' '));

  if (list.HasFormat(OpalT38)) {
    list -= OpalT38;

    for (PINDEX i = 0 ; i < mediaFormatList.GetSize() ; i++) {
      if (mediaFormatList[i].GetMediaType() == OpalMediaType::Fax()) {
        list += mediaFormatList[i];
        break;
      }
    }
  }

  PTRACE(4, "MySIPConnection::GetMediaFormats list=\n"
         << setfill('\n') << list << setfill(' '));

  return list;
}

void MySIPConnection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  //PTRACE(3, "MySIPConnection::AdjustMediaFormats:\n" << setprecision(2) << mediaFormats);

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    PBoolean found = FALSE;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++) {
      if (mediaFormats[i] == mediaFormatList[j]) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      PTRACE(3, "MySIPConnection::AdjustMediaFormats Remove " << mediaFormats[i]);
      mediaFormats -= mediaFormats[i];
      i--;
    }
  }

  SIPConnection::AdjustMediaFormats(mediaFormats);

  //PTRACE(3, "MySIPConnection::AdjustMediaFormats:\n" << setprecision(2) << mediaFormats);
}
/////////////////////////////////////////////////////////////////////////////

