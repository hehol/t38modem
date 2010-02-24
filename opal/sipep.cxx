/*
 * sipep.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2010 Vyacheslav Frolov
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
 * Revision 1.25  2010-02-24 14:20:10  vfrolov
 * Added variant of patch #2954967 "opal sip/h323 build-time detection"
 * Thanks Mariusz Mazur
 *
 * Revision 1.25  2010/02/24 14:20:10  vfrolov
 * Added variant of patch #2954967 "opal sip/h323 build-time detection"
 * Thanks Mariusz Mazur
 *
 * Revision 1.24  2010/02/12 08:55:07  vfrolov
 * Implemented fake codecs
 *
 * Revision 1.23  2010/02/08 17:30:31  vfrolov
 * Disabled OPAL version < 3.8.0
 *
 * Revision 1.22  2010/01/22 11:19:38  vfrolov
 * Added --sip-disable-t38-mode option
 *
 * Revision 1.21  2010/01/22 09:29:38  vfrolov
 * Added workaround to allow switching codecs to g711alaw if disabled g711ulaw
 *
 * Revision 1.20  2010/01/21 16:00:54  vfrolov
 * Changed --sip-audio to accept multiple wildcards
 * Implemented OPAL-Enable-Audio route option
 *
 * Revision 1.19  2010/01/21 08:28:09  vfrolov
 * Removed previously added workaround (now switching codecs fixed in OPAL)
 *
 * Revision 1.18  2010/01/15 11:53:31  vfrolov
 * Added workaround for switching codecs from non-G.711 to G.711
 *
 * Revision 1.17  2010/01/13 09:59:19  vfrolov
 * Fixed incompatibility with OPAL trunk
 * Fixed incorrect codec selection for the incoming offer
 *
 * Revision 1.16  2010/01/11 14:26:49  vfrolov
 * Duplicated code moved to ApplyStringOptions()
 *
 * Revision 1.15  2009/12/09 13:27:22  vfrolov
 * Fixed Disable-T38-Mode
 *
 * Revision 1.14  2009/12/08 15:06:22  vfrolov
 * Fixed incompatibility with OPAL trunk
 *
 * Revision 1.13  2009/11/10 11:30:57  vfrolov
 * Implemented G.711 fallback to fax pass-through mode
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

#if OPAL_SIP
/////////////////////////////////////////////////////////////////////////////
#define PACK_VERSION(major, minor, build) (((((major) << 8) + (minor)) << 8) + (build))

#if !(PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= PACK_VERSION(3, 8, 0))
  #error *** Uncompatible OPAL version (required >= 3.8.0) ***
#endif

#undef PACK_VERSION
/////////////////////////////////////////////////////////////////////////////

#include <sip/sipcon.h>

#include "sipep.h"
#include "fake_codecs.h"

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
    : SIPConnection(call, endpoint, token, address, transport, options, stringOptions)
    , switchingToFaxMode(false)
    {}
  //@}

    virtual PBoolean SetUpConnection();
    virtual void ApplyStringOptions(OpalConnection::StringOptions & stringOptions);

    virtual bool SwitchFaxMediaStreams(
      bool enableFax                            ///< Enable FAX or return to audio mode
    );

    virtual void OnSwitchedFaxMediaStreams(
      bool enabledFax                           ///< Enabled FAX or audio mode
    );

    virtual OpalMediaFormatList GetMediaFormats() const;
    virtual OpalMediaFormatList GetLocalMediaFormats();

    virtual void AdjustMediaFormats(
      bool local,                               ///<  Media formats a local ones to be presented to remote
      OpalMediaFormatList & mediaFormats,       ///<  Media formats to use
      OpalConnection * otherConnection          ///<  Other connection we are adjusting media for
    ) const;

  protected:
    mutable OpalMediaFormatList mediaFormatList;
    bool switchingToFaxMode;
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
    "-sip-disable-t38-mode."
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
      "  --sip-audio [!]wildcard[,[!]...]\n"
      "                            : Enable the audio format(s) matching the\n"
      "                              wildcard(s). The '*' character match any\n"
      "                              substring. The leading '!' character indicates\n"
      "                              a negative test.\n"
      "                              Default: " OPAL_G711_ULAW_64K "," OPAL_G711_ALAW_64K ".\n"
      "                              May be used multiple times.\n"
      "                              Can be overriden by route option\n"
      "                                OPAL-Enable-Audio=[!]wildcard[,[!]...]\n"
      "  --sip-audio-list          : Display available audio formats.\n"
      "  --sip-disable-t38-mode    : Disable T.38 fax mode.\n"
      "                              Can be overriden by route option\n"
      "                                OPAL-Disable-T38-Mode=false\n"
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

  if (args.HasOption("sip-audio-list"))
    descriptions += FakeCodecs::GetAvailableAudioFormatsDescription("SIP", "sip");

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
    PStringStream s;

    s << setfill(',') << args.GetOptionString("sip-audio").Lines();

    defaultStringOptions.SetAt("Enable-Audio", s);
  }

  if (args.HasOption("sip-disable-t38-mode"))
    defaultStringOptions.SetAt("Disable-T38-Mode", "true");

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

  OpalConnection::StringOptions newOptions;

  for (PINDEX i = 0 ; i < defaultStringOptions.GetSize() ; i++) {
    if (!connection->GetStringOptions().Contains(defaultStringOptions.GetKeyAt(i)))
      newOptions.SetAt(defaultStringOptions.GetKeyAt(i), defaultStringOptions.GetDataAt(i));
  }

  connection->SetStringOptions(newOptions, false);

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

void MySIPConnection::ApplyStringOptions(OpalConnection::StringOptions & stringOptions)
{
  if (LockReadWrite()) {
    mediaFormatList = OpalMediaFormatList();

    if (GetStringOptions().Contains("Enable-Audio")) {
      const PStringArray wildcards = GetStringOptions()("Enable-Audio").Tokenise(",", FALSE);
      OpalMediaFormatList list = endpoint.GetMediaFormats();

      for (PINDEX w = 0 ; w < wildcards.GetSize() ; w++) {
        OpalMediaFormatList::const_iterator f;

        while ((f = list.FindFormat(wildcards[w], f)) != list.end()) {
          if (f->GetMediaType() == OpalMediaType::Audio() && f->IsValidForProtocol("sip") && f->IsTransportable())
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
      PTRACE(3, "MySIPConnection::ApplyStringOptions: Disable-T38-Mode=true");
    } else {
      mediaFormatList += OpalT38;
    }

    mediaFormatList += OpalRFC2833;

    PTRACE(4, "MySIPConnection::ApplyStringOptions Enabled formats (in preference order):\n"
           << setfill('\n') << mediaFormatList << setfill(' '));

    UnlockReadWrite();
  }

  SIPConnection::ApplyStringOptions(stringOptions);
}

bool MySIPConnection::SwitchFaxMediaStreams(bool enableFax)
{
  PTRACE(3, "MySIPConnection::SwitchFaxMediaStreams: " << (enableFax ? "fax" : "audio"));

  bool res = false;

  if (!enableFax) {
    OpalMediaFormatList::iterator i = mediaFormatList.begin();

    while (i != mediaFormatList.end()) {
      if (i->GetMediaType() != OpalMediaType::Audio() || *i == OpalG711_ULAW_64K || *i == OpalG711_ALAW_64K)
        ++i;
      else
        mediaFormatList -= *i++;
    }
  }

  OpalMediaFormatList mediaFormats = GetMediaFormats();
  AdjustMediaFormats(true, mediaFormats, NULL);

  PTRACE(3, "MySIPConnection::SwitchFaxMediaStreams:\n" << setfill('\n') << mediaFormats << setfill(' '));

  const OpalMediaType &mediaType = enableFax ? OpalMediaType::Fax() : OpalMediaType::Audio();

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    if (mediaFormats[i].GetMediaType() == mediaType) {
      res = SIPConnection::SwitchFaxMediaStreams(switchingToFaxMode = enableFax);
      break;
    }
  }

  PTRACE(3, "MySIPConnection::SwitchFaxMediaStreams: " << (res ? "OK" : "FAIL"));
  return res;
}

void MySIPConnection::OnSwitchedFaxMediaStreams(bool enabledFax)
{
  PTRACE(3, "MySIPConnection::OnSwitchedFaxMediaStreams: "
         << (enabledFax == switchingToFaxMode ? "" : "NOT ") << "switched to "
         << (switchingToFaxMode ? "fax" : "audio"));

  SIPConnection::OnSwitchedFaxMediaStreams(enabledFax);

  if (switchingToFaxMode && !enabledFax) {
      PTRACE(3, "MySIPConnection::OnSwitchedFaxMediaStreams: fallback to audio");
      mediaFormatList -= OpalT38;
      SwitchFaxMediaStreams(false);
  }
}

OpalMediaFormatList MySIPConnection::GetMediaFormats() const
{
  OpalMediaFormatList mediaFormats = SIPConnection::GetMediaFormats();

  PTRACE(4, "MySIPConnection::GetMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    PBoolean found = FALSE;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++) {
      if (mediaFormats[i] == mediaFormatList[j]) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      PTRACE(3, "MySIPConnection::GetMediaFormats Remove " << mediaFormats[i]);
      mediaFormats -= mediaFormats[i];
      i--;
    }
  }

  PTRACE(4, "MySIPConnection::GetMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  return mediaFormats;
}

OpalMediaFormatList MySIPConnection::GetLocalMediaFormats()
{
  OpalMediaFormatList mediaFormats = SIPConnection::GetLocalMediaFormats();

  PTRACE(4, "MySIPConnection::GetLocalMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    PBoolean found = FALSE;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++) {
      if (mediaFormats[i] == mediaFormatList[j]) {
        found = TRUE;
        break;
      }
    }

    if (!found) {
      PTRACE(3, "MySIPConnection::GetLocalMediaFormats Remove " << mediaFormats[i]);
      mediaFormats -= mediaFormats[i];
      i--;
    }
  }

  PTRACE(4, "MySIPConnection::GetLocalMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  return mediaFormats;
}

void MySIPConnection::AdjustMediaFormats(
    bool local,
    OpalMediaFormatList & mediaFormats,
    OpalConnection * otherConnection) const
{
  PTRACE(4, "MySIPConnection::AdjustMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));

  SIPConnection::AdjustMediaFormats(local, mediaFormats, otherConnection);

  if (local) {
    PStringArray order;

    for (PINDEX j = 0 ; j < mediaFormatList.GetSize() ; j++)
      order += mediaFormatList[j].GetName();

    mediaFormats.Reorder(order);

    PTRACE(4, "MySIPConnection::AdjustMediaFormats: reordered");
  }

  PTRACE(4, "MySIPConnection::AdjustMediaFormats:\n" << setfill('\n') << mediaFormats << setfill(' '));
}
/////////////////////////////////////////////////////////////////////////////
#endif // OPAL_SIP
/////////////////////////////////////////////////////////////////////////////

