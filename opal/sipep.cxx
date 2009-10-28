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
 * Revision 1.12  2009-10-28 17:30:41  vfrolov
 * Fixed uncompatibility with OPAL trunk
 *
 * Revision 1.12  2009/10/28 17:30:41  vfrolov
 * Fixed uncompatibility with OPAL trunk
 *
 * Revision 1.11  2009/10/06 17:13:10  vfrolov
 * Fixed uncompatibility with OPAL trunk
 *
 * Revision 1.10  2009/07/31 17:34:40  vfrolov
 * Removed --h323-old-asn and --sip-old-asn options
 *
 * Revision 1.9  2009/07/22 17:26:54  vfrolov
 * Added ability to enable other audio formats
 *
 * Revision 1.8  2009/07/22 14:42:49  vfrolov
 * Added Descriptions(args) to endpoints
 *
 * Revision 1.7  2009/07/15 18:25:53  vfrolov
 * Added reordering of formats
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

/////////////////////////////////////////////////////////////////////////////
#define PACK_VERSION(major, minor, build) (((((major) << 8) + (minor)) << 8) + (build))

#if !(PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= PACK_VERSION(3, 7, 1))
  #error *** Uncompatible OPAL version (required >= 3.7.1) ***
#endif
/////////////////////////////////////////////////////////////////////////////

#include <sip/sipcon.h>

#include "manager.h"
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

    virtual void AdjustMediaFormats(
      OpalMediaFormatList & mediaFormats,       ///<  Media formats to use
      OpalConnection * otherConnection          ///<  Other connection we are adjusting media for
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
    "-sip-audio:"
    "-sip-audio-list."
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
      "  --sip-audio [!]wildcard   : Enable the audio format(s) matching the\n"
      "                              wildcard. The '*' character match any\n"
      "                              substring. The leading '!' character indicates\n"
      "                              a negative test.\n"
      "                              Default: " OPAL_G711_ULAW_64K " and " OPAL_G711_ALAW_64K ".\n"
      "                              May be used multiple times.\n"
      "  --sip-audio-list          : Display available audio formats.\n"
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

PStringArray MySIPEndPoint::Descriptions(const PConfigArgs & args)
{
  PStringArray descriptions;

  if (args.HasOption("sip-audio-list")) {
    descriptions.Append(new PString("Available audio formats for SIP:"));

    OpalMediaFormatList list = OpalMediaFormat::GetAllRegisteredMediaFormats();

    for (OpalMediaFormatList::iterator f = list.begin(); f != list.end(); ++f) {
      if (f->GetMediaType() == OpalMediaType::Audio() && f->IsValidForProtocol("sip") && f->IsTransportable())
        descriptions.Append(new PString(PString("  ") + f->GetName()));
    }
  }

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
  if (args.HasOption("sip-audio")) {
    const PStringArray wildcards = args.GetOptionString("sip-audio").Lines();
    OpalMediaFormatList list = GetMediaFormats();

    for (PINDEX w = 0 ; w < wildcards.GetSize() ; w++) {
      OpalMediaFormatList::const_iterator f;

      while ((f = list.FindFormat(wildcards[w], f)) != list.end()) {
        if (f->GetMediaType() == OpalMediaType::Audio() && f->IsValidForProtocol("sip") && f->IsTransportable())
          AddMediaFormatList(*f);

        if (++f == list.end())
          break;
      }
    }
  } else {
    AddMediaFormatList(OpalG711_ULAW_64K);
    AddMediaFormatList(OpalG711_ALAW_64K);
  }

  cout << "Enabled audio formats for SIP (in preference order):" << endl;

  for (PINDEX i = 0 ; i < mediaFormatList.GetSize() ; i++)
    cout << "  " << mediaFormatList[i] << endl;

  AddMediaFormatList(OpalPCM16);
  AddMediaFormatList(OpalT38);

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

void MySIPConnection::AdjustMediaFormats(
    OpalMediaFormatList & mediaFormats,
    OpalConnection * otherConnection) const
{
  //PTRACE(3, "MySIPConnection::AdjustMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  SIPConnection::AdjustMediaFormats(mediaFormats, otherConnection);

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    PBoolean found = FALSE;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++) {
      if (mediaFormats[i] == mediaFormatList[j]) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      //PTRACE(3, "MySIPConnection::AdjustMediaFormats Remove " << mediaFormats[i]);
      mediaFormats -= mediaFormats[i];
      i--;
    }
  }

  PStringArray order;

  for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++)
    order += mediaFormatList[j].GetName();

  mediaFormats.Reorder(order);

  //PTRACE(3, "MySIPConnection::AdjustMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));
}
/////////////////////////////////////////////////////////////////////////////

