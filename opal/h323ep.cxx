/*
 * h323ep.cxx
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
 * $Log: h323ep.cxx,v $
 * Revision 1.26  2011-02-11 09:41:07  vfrolov
 * Added more tracing
 *
 * Revision 1.26  2011/02/11 09:41:07  vfrolov
 * Added more tracing
 *
 * Revision 1.25  2011/01/19 11:41:17  vfrolov
 * Replaced deprecated ApplyStringOptions() by OnApplyStringOptions()
 *
 * Revision 1.24  2011/01/13 06:39:08  vfrolov
 * Disabled OPAL version < 3.9.0
 * Added route options help topic
 *
 * Revision 1.23  2010/03/15 14:31:30  vfrolov
 * Added options
 *   --h323-t38-udptl-redundancy
 *   --h323-t38-udptl-keep-alive-interval
 *
 * Revision 1.22  2010/02/24 14:20:09  vfrolov
 * Added variant of patch #2954967 "opal sip/h323 build-time detection"
 * Thanks Mariusz Mazur
 *
 * Revision 1.21  2010/02/12 08:55:07  vfrolov
 * Implemented fake codecs
 *
 * Revision 1.20  2010/02/08 17:30:31  vfrolov
 * Disabled OPAL version < 3.8.0
 *
 * Revision 1.19  2010/01/22 11:20:20  vfrolov
 * Added --h323-disable-t38-mode option
 *
 * Revision 1.18  2010/01/21 16:05:33  vfrolov
 * Changed --h323-audio to accept multiple wildcards
 * Implemented OPAL-Enable-Audio route option
 * Renamed route option OPAL-H323-Bearer-Capability to OPAL-Bearer-Capability
 *
 * Revision 1.17  2010/01/21 09:22:45  vfrolov
 * Fixed tracing typo
 *
 * Revision 1.16  2010/01/13 09:59:19  vfrolov
 * Fixed incompatibility with OPAL trunk
 * Fixed incorrect codec selection for the incoming offer
 *
 * Revision 1.15  2009/12/23 17:54:24  vfrolov
 * Implemented --h323-bearer-capability option
 *
 * Revision 1.14  2009/12/09 13:27:22  vfrolov
 * Fixed Disable-T38-Mode
 *
 * Revision 1.13  2009/12/08 15:06:22  vfrolov
 * Fixed incompatibility with OPAL trunk
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

#include <opal_config.h>

#if OPAL_H323
/////////////////////////////////////////////////////////////////////////////
#define PACK_VERSION(major, minor, build) (((((major) << 8) + (minor)) << 8) + (build))

#if !(PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= PACK_VERSION(3, 16, 1))
  #error *** Incompatible OPAL version (required >= 3.16.1) ***
#endif

#undef PACK_VERSION
/////////////////////////////////////////////////////////////////////////////

#include <h323/h323pdu.h>

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
    virtual void OnApplyStringOptions();

    virtual PBoolean OnSendSignalSetup(
      H323SignalPDU & setupPDU                 ///<  Setup PDU to send
    );

    virtual AnswerCallResponse OnAnswerCall(
      const PString & callerName,              ///< Name of caller
      const H323SignalPDU & setupPDU,          ///< Received setup PDU
      H323SignalPDU & connectPDU,              ///< Connect PDU to send
      H323SignalPDU & progressPDU              ///< Progress PDU to send
    );

    virtual bool SwitchFaxMediaStreams(
      bool toT38                               ///< Enable FAX or return to audio mode
    );

    virtual void OnSwitchedFaxMediaStreams(
      bool toT38,                              ///< Enabled FAX or audio mode
      bool success                             ///< True if switch succeeded
    );

    virtual PBoolean OnOpenMediaStream(
      OpalMediaStream & stream                 ///<  New media stream being opened
    );

    virtual OpalMediaFormatList GetMediaFormats() const;
    virtual OpalMediaFormatList GetLocalMediaFormats();

    virtual void AdjustMediaFormats(
      bool local,                              ///<  Media formats a local ones to be presented to remote
      OpalConnection * otherConnection,        ///<  Other connection we are adjusting media for
      OpalMediaFormatList & mediaFormats       ///<  Media formats to use
    ) const;

  protected:
    mutable OpalMediaFormatList mediaFormatList;
    PIntArray bearerCapability;
    bool switchingToFaxMode;
};
/////////////////////////////////////////////////////////////////////////////
//
//  Implementation
//
/////////////////////////////////////////////////////////////////////////////
PStringToString MyH323EndPoint::defaultStringOptions;

PString MyH323EndPoint::ArgSpec()
{
  return
    "-no-h323."
    "-h323-disable-t38-mode."
    "-h323-t38-udptl-redundancy:"
    "-h323-t38-udptl-keep-alive-interval:"
    "F-fastenable."
    "T-h245tunneldisable."
    "-h323-listen:"
    "-h323-no-listen."
    "g-gatekeeper:"
    "n-no-gatekeeper."
    "-require-gatekeeper."
    "-h323-bearer-capability:"
  ;
}

PStringArray MyH323EndPoint::Descriptions()
{
  PStringArray descriptions = PString(
      "H.323 options:\n"
      "  --no-h323                 : Disable H.323 protocol.\n"
      "  --h323-disable-t38-mode   : Use OPAL-Disable-T38-Mode=true route option by\n"
      "                              default.\n"
      "  --h323-t38-udptl-redundancy str\n"
      "                            : Use OPAL-T38-UDPTL-Redundancy=str route option by\n"
      "                              default.\n"
      "  --h323-t38-udptl-keep-alive-interval ms\n"
      "                            : Use OPAL-T38-UDPTL-Keep-Alive-Interval=ms route\n"
      "                              option by default.\n"
      "  -F --fastenable           : Enable fast start.\n"
      "  -T --h245tunneldisable    : Disable H245 tunnelling.\n"
      "  --h323-listen iface       : Interface/port(s) to listen for H.323 requests\n"
      "                            : '*' is all interfaces, (default tcp$*:1720).\n"
      "  --h323-no-listen          : Disable listen for incoming calls.\n"
      "  -g --gatekeeper host[,user,password]\n"      
      "                            : Specify gatekeeper host.\n"
      "  -n --no-gatekeeper        : Disable gatekeeper discovery.\n"
      "  --require-gatekeeper      : Exit if gatekeeper discovery fails.\n"
      "  --h323-bearer-capability str\n"
      "                            : Use OPAL-Bearer-Capability=str route option by\n"
      "                              default.\n"
      "\n"
      "H.323 route options:\n"
      "  OPAL-Enable-Audio=[!]wildcard[,[!]...]\n"
      "    Enable the audio format(s) matching the wildcard(s). The '*' character\n"
      "    match any substring. The leading '!' character indicates a negative test.\n"
      "    Default: " OPAL_G711_ULAW_64K "," OPAL_G711_ALAW_64K ".\n"
      "  OPAL-Disable-T38-Mode={true|false}\n"
      "    Enable or disable T.38 fax mode.\n"
      "    Default: false (enable T.38 fax mode).\n"
      "  OPAL-T38-UDPTL-Redundancy=[maxsize:redundancy[,maxsize:redundancy...]]\n"
      "    Set error recovery redundancy for IFP packets dependent from their size.\n"
      "    For example the string '2:I,9:L,32767:H' (where I, L and H are numbers)\n"
      "    sets redundancy for (I)ndication, (L)ow speed and (H)igh speed IFP packets.\n"
      "    Default: empty string (no redundancy).\n"
      "  OPAL-T38-UDPTL-Redundancy-Interval=ms\n"
      "    Continuously resend last UDPTL packet each ms milliseconds on idle till it\n"
      "    contains IFP packets not sent redundancy times.\n"
      "    Default: 50.\n"
      "  OPAL-T38-UDPTL-Keep-Alive-Interval=ms\n"
      "    Continuously resend last UDPTL packet each ms milliseconds on idle.\n"
      "    Default: 0 (no resend).\n"
      "  OPAL-T38-UDPTL-Optimise-On-Retransmit={true|false}\n"
      "    Optimize UDPTL packets on resending in accordance with required redundancy\n"
      "    (exclude redundancy IFP packets sent redundancy times).\n"
      "    Default: true (optimize).\n"
      "  OPAL-Bearer-Capability=S:C:R:P\n"
      "    Set bearer capability information element (Q.931) with\n"
      "      S - coding standard (0-3)\n"
      "      C - information transfer capability (0-31)\n"
      "      R - information transfer rate (1-127)\n"
      "      P - user information layer 1 protocol (2-5).\n"
  ).Lines();

  return descriptions;
}

PStringArray MyH323EndPoint::Descriptions(const PConfigArgs & args)
{
  PStringArray descriptions;

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
  if (args.HasOption("h323-disable-t38-mode"))
    defaultStringOptions.SetAt("Disable-T38-Mode", "true");

  DisableFastStart(!args.HasOption("fastenable"));
  DisableH245Tunneling(args.HasOption("h245tunneldisable"));

  defaultStringOptions.SetAt("T38-UDPTL-Redundancy-Interval", "50");
  defaultStringOptions.SetAt("T38-UDPTL-Optimise-On-Retransmit", "true");

  defaultStringOptions.SetAt("T38-UDPTL-Redundancy",
                             args.HasOption("h323-t38-udptl-redundancy")
                             ? args.GetOptionString("h323-t38-udptl-redundancy")
                             : "");

  defaultStringOptions.SetAt("T38-UDPTL-Keep-Alive-Interval",
                             args.HasOption("h323-t38-udptl-keep-alive-interval")
                             ? args.GetOptionString("h323-t38-udptl-keep-alive-interval")
                             : "0");

  if (args.HasOption("h323-bearer-capability"))
    defaultStringOptions.SetAt("Bearer-Capability", args.GetOptionString("h323-bearer-capability"));

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
    PString r = args.GetOptionString("gatekeeper");
    PStringArray regs = r.Tokenise("\r\n", FALSE);

    for (PINDEX i = 0 ; i < regs.GetSize() ; i++) {
      PStringArray prms = regs[i].Tokenise(",",TRUE);

      PAssert(prms.GetSize() >= 1, "empty registration information");

      if (prms.GetSize() >= 1) {
        PString gkName = prms[0];

        if (prms.GetSize() >= 3) {
          PString gkUser = prms[1];
          PString gkPass = prms[2];
          SetGatekeeperPassword(gkPass,gkUser);
        }
        
        if (SetGatekeeper(gkName))
          cout << "Gatekeeper set: " << *GetGatekeeper() << endl;
        else {
          cerr << "Error registering with gatekeeper at \"" << gkName << '"' << endl;
          return FALSE;
        }
      }
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

  PTRACE(6, "MyH323EndPoint::CreateConnection new " << connection->GetClass() << ' ' << (void *)connection);

  OpalConnection::StringOptions newOptions;

  for (PINDEX i = 0 ; i < defaultStringOptions.GetSize() ; i++) {
    if (!connection->GetStringOptions().Contains(defaultStringOptions.GetKeyAt(i)))
      newOptions.SetAt(defaultStringOptions.GetKeyAt(i), defaultStringOptions.GetDataAt(i));
  }

  connection->SetStringOptions(newOptions, false);

  return connection;
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

void MyH323Connection::OnApplyStringOptions()
{
  H323Connection::OnApplyStringOptions();

  if (LockReadWrite()) {
    mediaFormatList = OpalMediaFormatList();

    if (GetStringOptions().Contains("Enable-Audio")) {
      const PStringArray wildcards = GetStringOptions()("Enable-Audio").Tokenise(",", FALSE);
      OpalMediaFormatList list = m_endpoint.GetMediaFormats();

      for (PINDEX w = 0 ; w < wildcards.GetSize() ; w++) {
        OpalMediaFormatList::const_iterator f;

        while ((f = list.FindFormat(wildcards[w], f)) != list.end()) {
          if (f->GetMediaType() == OpalMediaType::Audio() && f->IsValidForProtocol("h323") && f->IsTransportable())
             mediaFormatList += *f;

          if (++f == list.end())
            break;
        }
      }
    } else {
      mediaFormatList += OpalG711_ULAW_64K;
      mediaFormatList += OpalG711_ALAW_64K;
    }

    if (GetStringOptions().GetBoolean("Disable-T38-Mode")) {
      PTRACE(3, "MyH323Connection::OnApplyStringOptions: Disable-T38-Mode=true");
    } else {
      mediaFormatList += OpalT38;
    }

    mediaFormatList += OpalRFC2833;

    PTRACE(4, "MyH323Connection::OnApplyStringOptions Enabled formats (in preference order):\n"
           << setfill('\n') << mediaFormatList << setfill(' '));

    if (GetStringOptions().Contains("Bearer-Capability")) {
      PString bc = GetStringOptions()["Bearer-Capability"];
      PStringArray sBC = bc.Tokenise(":", FALSE);
      PIntArray iBC(4);

      if (sBC.GetSize() == iBC.GetSize()) {
        for (PINDEX i = 0 ; i < iBC.GetSize() ; i++)
          iBC[i] = sBC[i].AsUnsigned();

        if (iBC[0] >= 0 && iBC[0] <= 3 &&
            iBC[1] >= 0 && iBC[1] <= 31 &&
            iBC[2] >= 1 && iBC[2] <= 127 &&
            iBC[3] >= 2 && iBC[3] <= 5)
        {
          PTRACE(3, "MyH323Connection::OnApplyStringOptions: Bearer-Capability=" << bc);
          bearerCapability = iBC;
        } else {
          iBC[0] = -1;
        }
      } else {
        iBC[0] = -1;
      }

      if (iBC[0] < 0) {
        PTRACE(3, "MyH323Connection::OnApplyStringOptions: Wrong Bearer-Capability=" << bc << " (ignored)");
      }
    }

    UnlockReadWrite();
  }
}

PBoolean MyH323Connection::OnSendSignalSetup(H323SignalPDU & setupPDU)
{
  if (!bearerCapability.IsEmpty()) {
    PTRACE(3, "MyH323Connection::OnSendSignalSetup: Set Bearer capability '" << bearerCapability << "'");

    setupPDU.GetQ931().SetBearerCapabilities(
        Q931::InformationTransferCapability(bearerCapability[1]),
        bearerCapability[2],
        bearerCapability[0],
        bearerCapability[3]);
  }

  return H323Connection::OnSendSignalSetup(setupPDU);
}

H323Connection::AnswerCallResponse MyH323Connection::OnAnswerCall(
    const PString & caller,
    const H323SignalPDU & setupPDU,
    H323SignalPDU & connectPDU,
    H323SignalPDU & progressPDU)
{
  if (!bearerCapability.IsEmpty()) {
    PTRACE(3, "MyH323Connection::OnAnswerCall: Set Bearer capability '" << bearerCapability << "'");

    connectPDU.GetQ931().SetBearerCapabilities(
        Q931::InformationTransferCapability(bearerCapability[1]),
        bearerCapability[2],
        bearerCapability[0],
        bearerCapability[3]);

    progressPDU.GetQ931().SetBearerCapabilities(
        Q931::InformationTransferCapability(bearerCapability[1]),
        bearerCapability[2],
        bearerCapability[0],
        bearerCapability[3]);
  }

  return H323Connection::OnAnswerCall(caller, setupPDU, connectPDU, progressPDU);
}

bool MyH323Connection::SwitchFaxMediaStreams(bool enableFax)
{
  OpalMediaFormatList mediaFormats = GetMediaFormats();
  AdjustMediaFormats(true, NULL, mediaFormats);

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

void MyH323Connection::OnSwitchedFaxMediaStreams(bool toT38, bool success)
{
  PTRACE(3, "MyH323Connection::OnSwitchedFaxMediaStreams: "
         << (success ? "succeeded" : "NOT ") << "switched to "
         << (toT38 ? "T.38" : "audio"));

  H323Connection::OnSwitchedFaxMediaStreams(toT38, success);

  if (toT38 && !success) {
      PTRACE(3, "MyH323Connection::OnSwitchedFaxMediaStreams: fallback to audio");
      mediaFormatList -= OpalT38;
      SwitchFaxMediaStreams(false);
  }
}

PBoolean MyH323Connection::OnOpenMediaStream(OpalMediaStream & stream)
{
  PTRACE(4, "MyH323Connection::OnOpenMediaStream: " << stream);

  //OpalRTP_Session *session = GetSession(stream.GetSessionID());

  //if (session)
  //  OpalRTP_Session::EncodingLock(*session)->ApplyStringOptions(GetStringOptions());

  return H323Connection::OnOpenMediaStream(stream);
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
    bool local,
    OpalConnection * otherConnection,
    OpalMediaFormatList & mediaFormats) const
{
  PTRACE(4, "MyH323Connection::AdjustMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  H323Connection::AdjustMediaFormats(local, otherConnection, mediaFormats);

  if (local) {
    PStringArray order;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++)
      order += mediaFormatList[j].GetName();

    mediaFormats.Reorder(order);

    PTRACE(4, "MyH323Connection::AdjustMediaFormats: reordered");
  }

  PTRACE(4, "MyH323Connection::AdjustMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));
}
/////////////////////////////////////////////////////////////////////////////
#endif // OPAL_H323
/////////////////////////////////////////////////////////////////////////////

