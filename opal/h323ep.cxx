/*
 * h323ep.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2008 Vyacheslav Frolov
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
 * $Log: h323ep.cxx,v $
 * Revision 1.4  2008-09-24 14:39:21  frolov
 * Removed capabilities adding
 *
 * Revision 1.4  2008/09/24 14:39:21  frolov
 * Removed capabilities adding
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

#include "manager.h"
#include "ifpmediafmt.h"
#include "h323ep.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
//
//  MyH323Connection
//    removes local capabilities for not allowed media formats
//
class MyH323Connection : public H323Connection
{
  PCLASSINFO(MyH323Connection, H323Connection);

  public:
  /**@name Construction */
  //@{
    /**Create a new connection.
     */
    MyH323Connection(
      OpalCall & call,                         ///<  Call object connection belongs to
      H323EndPoint & endpoint,                 ///<  H323 End Point object
      const PString & token,                   ///<  Token for new connection
      const PString & alias,                   ///<  Alias for outgoing call
      const H323TransportAddress & address,    ///<  Address for outgoing call
      unsigned options = 0,                    ///<  Connection option bits
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    )
    : H323Connection(call, endpoint, token, alias, address, options, stringOptions) {}
  //@}

    virtual PBoolean SetUpConnection();

    virtual RTP_Session * CreateSession(
      const OpalTransport & transport,
      unsigned sessionID,
      RTP_QOS * rtpqos
    );

    virtual void AdjustMediaFormats(
      OpalMediaFormatList & mediaFormats  ///<  Media formats to use
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
PString MyH323EndPoint::ArgSpec()
{
  return
    "-no-h323."
    "-h323-old-asn."
    /*
    "-h323-redundancy:"
    "-h323-repeat:"
    */
    "F-fastenable."
    "T-h245tunneldisable."
    "-h323-listen:"
    "-h323-no-listen."
    "g-gatekeeper:"
    "n-no-gatekeeper."
    "-require-gatekeeper."
  ;
}

PStringArray MyH323EndPoint::Descriptions()
{
  PStringArray descriptions = PString(
      "H.323 options:\n"
      "  --no-h323                 : Disable H.323 protocol.\n"
      "  --h323-old-asn            : Use original ASN.1 sequence in T.38 (06/98)\n"
      "                              Annex A (w/o CORRIGENDUM No. 1 fix).\n"
      /*
      "  --h323-redundancy I[L[H]] : Set redundancy for error recovery for\n"
      "                              (I)ndication, (L)ow speed and (H)igh\n"
      "                              speed IFP packets.\n"
      "                              'I', 'L' and 'H' are digits.\n"
      "  --h323-repeat ms          : Continuously resend last UDPTL packet each ms\n"
      "                              milliseconds.\n"
      */
      "  -F --fastenable           : Enable fast start.\n"
      "  -T --h245tunneldisable    : Disable H245 tunnelling.\n"
      "  --h323-listen iface       : Interface/port(s) to listen for H.323 requests\n"
      "                            : '*' is all interfaces, (default tcp$*:1720).\n"
      "  --h323-no-listen          : Disable listen for incoming calls.\n"
      "  -g --gatekeeper host      : Specify gatekeeper host.\n"
      "  -n --no-gatekeeper        : Disable gatekeeper discovery.\n"
      "  --require-gatekeeper      : Exit if gatekeeper discovery fails.\n"
  ).Lines();

  return descriptions;
}

PBoolean MyH323EndPoint::Create(OpalManager & mgr, const PConfigArgs & args)
{
  if (args.HasOption("no-h323")) {
    cout << "Disabled H.323 protocol" << endl;
    return TRUE;
  }

  if ((new MyH323EndPoint(mgr))->Initialise(args))
    return TRUE;

  return FALSE;
}

PBoolean MyH323EndPoint::Initialise(const PConfigArgs & args)
{
  AddMediaFormatList(OpalG711_ULAW_64K);
  AddMediaFormatList(OpalG711_ALAW_64K);
  AddMediaFormatList(args.HasOption("h323-old-asn") ? OpalT38_IFP_PRE : OpalT38_IFP_COR);

  DisableFastStart(!args.HasOption("fastenable"));
  DisableH245Tunneling(args.HasOption("h245tunneldisable"));

  //cout << "Codecs (in preference order):\n" << setprecision(2) << capabilities << endl;

  /*
  if (args.HasOption("h323-redundancy")) {
    const char *r = args.GetOptionString("h323-redundancy");
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

  if (args.HasOption("h323-repeat"))
    re_interval = (int)args.GetOptionString("h323-repeat").AsInteger();
  */

  if (!args.HasOption("h323-no-listen")) {
    PStringArray listeners;

    if (args.HasOption("h323-listen"))
      listeners = args.GetOptionString("h323-listen").Lines();
    else
      listeners = GetDefaultListeners();

    if (!StartListeners(listeners)) {
      cerr << "Could not open any H.323 listener from "
           << setfill(',') << listeners << endl;
      return FALSE;
    }
    cout << "Waiting for incoming H.323 calls from "
         << setfill(',') << listeners << endl;
  }

  if (args.HasOption("gatekeeper")) {
    PString gkName = args.GetOptionString("gatekeeper");
    if (SetGatekeeper(gkName))
      cout << "Gatekeeper set: " << *GetGatekeeper() << endl;
    else {
      cerr << "Error registering with gatekeeper at \"" << gkName << '"' << endl;
      return FALSE;
    }
  }
  else
  if (!args.HasOption("no-gatekeeper")) {
    cout << "Searching for gatekeeper..." << flush;
    if (DiscoverGatekeeper())
      cout << "\nGatekeeper found: " << *GetGatekeeper() << endl;
    else {
      cerr << "\nNo gatekeeper found." << endl;
      if (args.HasOption("require-gatekeeper"))
        return FALSE;
    }
  }

  return TRUE;
}

H323Connection * MyH323EndPoint::CreateConnection(
    OpalCall & call,
    const PString & token,
    void * /*userData*/,
    OpalTransport & /*transport*/,
    const PString & alias,
    const H323TransportAddress & address,
    H323SignalPDU * /*setupPDU*/,
    unsigned options,
    OpalConnection::StringOptions * stringOptions)
{
  PTRACE(2, "MyH323EndPoint::CreateConnection for " << call);

  MyH323Connection *connection =
      new MyH323Connection(call, *this, token, alias, address, options, stringOptions);

  connection->AddMediaFormatList(mediaFormatList);

  return connection;
}

/*
void MyH323EndPoint::SetWriteInterval(
    OpalConnection &connection,
    const PTimeInterval &interval)
{
  ((MyManager &)GetManager()).SetWriteInterval(connection, interval);
}
*/

PBoolean MyH323EndPoint::RequestModeChange(
    OpalConnection & connection,
    const OpalMediaType & mediaType)
{
  if (mediaType != OpalMediaType::Fax())
    return PFalse;

  for (PINDEX i = 0 ; i < mediaFormatList.GetSize() ; i++) {
    if (mediaFormatList[i].GetMediaType() == mediaType) {
      PAssert(PIsDescendant(&connection, MyH323Connection), PInvalidCast);

      return ((MyH323Connection &)connection).RequestModeChangeT38(mediaFormatList[i].GetName());
    }
  }

  return PFalse;
}
/////////////////////////////////////////////////////////////////////////////
PBoolean MyH323Connection::SetUpConnection()
{
  PTRACE(2, "MyH323Connection::SetUpConnection " << *this << " name=" << GetLocalPartyName());

  PSafePtr<OpalConnection> conn = GetCall().GetConnection(0);

  if (conn != NULL && conn != this) {
    // Set the calling number of an outgoing connection

    PString name = conn->GetRemotePartyNumber();

    if (!name.IsEmpty() && name != "*") {
      SetLocalPartyName(name);

      PTRACE(1, "MyH323Connection::SetUpConnection new name=" << GetLocalPartyName());
    }
  }

  return H323Connection::SetUpConnection();
}

RTP_Session * MyH323Connection::CreateSession(
    const OpalTransport & transport,
    unsigned sessionID,
    RTP_QOS * rtpqos)
{
  PTRACE(3, "MyH323Connection::CreateSession " << sessionID << " t=" << transport);

  return H323Connection::CreateSession(transport, sessionID, rtpqos);
}

void MyH323Connection::AdjustMediaFormats(OpalMediaFormatList & mediaFormats) const
{
  //PTRACE(3, "MyH323Connection::AdjustMediaFormats:\n" << setprecision(2) << mediaFormats);

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    PBoolean found = FALSE;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++) {
      if (mediaFormats[i] == mediaFormatList[j]) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      //PTRACE(3, "MyH323Connection::AdjustMediaFormats Remove " << mediaFormats[i]);
      mediaFormats -= mediaFormats[i];
      i--;
    }
  }

  H323Connection::AdjustMediaFormats(mediaFormats);

  //PTRACE(3, "MyH323Connection::AdjustMediaFormats:\n" << setprecision(2) << mediaFormats);
}
/////////////////////////////////////////////////////////////////////////////

