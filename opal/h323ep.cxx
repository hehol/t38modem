/*
 * h323ep.cxx
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
 * $Log: h323ep.cxx,v $
 * Revision 1.12  2009-11-10 11:30:57  vfrolov
 * Implemented G.711 fallback to fax pass-through mode
 *
 * Revision 1.12  2009/11/10 11:30:57  vfrolov
 * Implemented G.711 fallback to fax pass-through mode
 *
 * Revision 1.11  2009/10/28 17:30:41  vfrolov
 * Fixed uncompatibility with OPAL trunk
 *
 * Revision 1.10  2009/10/06 17:13:10  vfrolov
 * Fixed uncompatibility with OPAL trunk
 *
 * Revision 1.9  2009/07/31 17:34:40  vfrolov
 * Removed --h323-old-asn and --sip-old-asn options
 *
 * Revision 1.8  2009/07/22 17:26:54  vfrolov
 * Added ability to enable other audio formats
 *
 * Revision 1.7  2009/07/22 14:42:49  vfrolov
 * Added Descriptions(args) to endpoints
 *
 * Revision 1.6  2009/07/15 18:25:53  vfrolov
 * Added reordering of formats
 *
 * Revision 1.5  2009/05/29 13:01:41  vfrolov
 * Ported to OPAL trunk
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

/////////////////////////////////////////////////////////////////////////////
#define PACK_VERSION(major, minor, build) (((((major) << 8) + (minor)) << 8) + (build))

#if !(PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= PACK_VERSION(3, 7, 1))
  #error *** Uncompatible OPAL version (required >= 3.7.1) ***
#endif

#undef PACK_VERSION
/////////////////////////////////////////////////////////////////////////////

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
    : H323Connection(call, endpoint, token, alias, address, options, stringOptions)
    , switchingToFaxMode(false)
    {}
  //@}

    virtual PBoolean SetUpConnection();

    virtual bool SwitchFaxMediaStreams(
      bool enableFax                           ///< Enable FAX or return to audio mode
    );

    virtual void OnSwitchedFaxMediaStreams(
      bool enabledFax                          ///< Enabled FAX or audio mode
    );

    virtual OpalMediaFormatList GetMediaFormats() const;
    virtual OpalMediaFormatList GetLocalMediaFormats();

    virtual void AdjustMediaFormats(
      OpalMediaFormatList & mediaFormats,      ///<  Media formats to use
      OpalConnection * otherConnection         ///<  Other connection we are adjusting media for
    ) const;

    void AddMediaFormatList(const OpalMediaFormatList & list) { mediaFormatList += list; }

  protected:
    OpalMediaFormatList mediaFormatList;
    bool switchingToFaxMode;
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
    "-h323-audio:"
    "-h323-audio-list."
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
      "  --h323-audio [!]wildcard  : Enable the audio format(s) matching the\n"
      "                              wildcard. The '*' character match any\n"
      "                              substring. The leading '!' character indicates\n"
      "                              a negative test.\n"
      "                              Default: " OPAL_G711_ULAW_64K " and " OPAL_G711_ALAW_64K ".\n"
      "                              May be used multiple times.\n"
      "  --h323-audio-list         : Display available audio formats.\n"
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

PStringArray MyH323EndPoint::Descriptions(const PConfigArgs & args)
{
  PStringArray descriptions;

  if (args.HasOption("h323-audio-list")) {
    descriptions.Append(new PString("Available audio formats for H.323:"));

    OpalMediaFormatList list = OpalMediaFormat::GetAllRegisteredMediaFormats();

    for (OpalMediaFormatList::iterator f = list.begin(); f != list.end(); ++f) {
      if (f->GetMediaType() == OpalMediaType::Audio() && f->IsValidForProtocol("h.323") && f->IsTransportable())
        descriptions.Append(new PString(PString("  ") + f->GetName()));
    }
  }

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
  if (args.HasOption("h323-audio")) {
    const PStringArray wildcards = args.GetOptionString("h323-audio").Lines();
    OpalMediaFormatList list = GetMediaFormats();

    for (PINDEX w = 0 ; w < wildcards.GetSize() ; w++) {
      OpalMediaFormatList::const_iterator f;

      while ((f = list.FindFormat(wildcards[w], f)) != list.end()) {
        if (f->GetMediaType() == OpalMediaType::Audio() && f->IsValidForProtocol("h.323") && f->IsTransportable())
          AddMediaFormatList(*f);

        if (++f == list.end())
          break;
      }
    }
  } else {
    AddMediaFormatList(OpalG711_ULAW_64K);
    AddMediaFormatList(OpalG711_ALAW_64K);
  }

  cout << "Enabled audio formats for H.323 (in preference order):" << endl;

  for (PINDEX i = 0 ; i < mediaFormatList.GetSize() ; i++)
    cout << "  " << mediaFormatList[i] << endl;

  AddMediaFormatList(OpalT38);

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

  OpalMediaFormatList mediaFormats = mediaFormatList;

  if (connection->GetStringOptions().Contains("Disable-T38-Mode")) {
    PTRACE(3, "MyH323EndPoint::CreateConnection: Disable-T38-Mode");
    mediaFormats -= OpalT38;
  }

  connection->AddMediaFormatList(mediaFormats);

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

bool MyH323Connection::SwitchFaxMediaStreams(bool enableFax)
{
  OpalMediaFormatList mediaFormats = GetMediaFormats();
  AdjustMediaFormats(mediaFormats, NULL);

  PTRACE(3, "MyH323Connection::SwitchFaxMediaStreams:\n" << setfill('\n') << mediaFormats << setfill(' '));

  const OpalMediaType &mediaType = enableFax ? OpalMediaType::Fax() : OpalMediaType::Audio();

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    if (mediaFormats[i].GetMediaType() == mediaType) {
      switchingToFaxMode = enableFax;
      return H323Connection::SwitchFaxMediaStreams(enableFax);
    }
  }

  PTRACE(3, "MyH323Connection::SwitchFaxMediaStreams: " << mediaType << " is not supported");

  return false;
}

void MyH323Connection::OnSwitchedFaxMediaStreams(bool enabledFax)
{
  PTRACE(3, "MyH323Connection::OnSwitchedFaxMediaStreams: "
         << (enabledFax == switchingToFaxMode ? "" : "NOT ") << "switched to "
         << (switchingToFaxMode ? "fax" : "audio"));

  if (switchingToFaxMode && !enabledFax) {
      PTRACE(3, "MyH323Connection::SwitchFaxMediaStreams: fallback to audio");
      mediaFormatList -= OpalT38;
      SwitchFaxMediaStreams(false);
  }
}

OpalMediaFormatList MyH323Connection::GetMediaFormats() const
{
  OpalMediaFormatList mediaFormats = H323Connection::GetMediaFormats();

  PTRACE(4, "MyH323Connection::GetMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    PBoolean found = FALSE;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++) {
      if (mediaFormats[i] == mediaFormatList[j]) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      PTRACE(3, "MyH323Connection::GetMediaFormats Remove " << mediaFormats[i]);
      mediaFormats -= mediaFormats[i];
      i--;
    }
  }

  PTRACE(4, "MyH323Connection::GetMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  return mediaFormats;
}

OpalMediaFormatList MyH323Connection::GetLocalMediaFormats()
{
  OpalMediaFormatList mediaFormats = H323Connection::GetLocalMediaFormats();

  PTRACE(4, "MyH323Connection::GetLocalMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    PBoolean found = FALSE;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++) {
      if (mediaFormats[i] == mediaFormatList[j]) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      PTRACE(3, "MyH323Connection::GetLocalMediaFormats Remove " << mediaFormats[i]);
      mediaFormats -= mediaFormats[i];
      i--;
    }
  }

  PTRACE(4, "MyH323Connection::GetLocalMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  return mediaFormats;
}

void MyH323Connection::AdjustMediaFormats(
    OpalMediaFormatList & mediaFormats,
    OpalConnection * otherConnection) const
{
  PTRACE(4, "MyH323Connection::AdjustMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  H323Connection::AdjustMediaFormats(mediaFormats, otherConnection);

  PStringArray order;

  for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++)
    order += mediaFormatList[j].GetName();

  mediaFormats.Reorder(order);

  PTRACE(4, "MyH323Connection::AdjustMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));
}
/////////////////////////////////////////////////////////////////////////////

