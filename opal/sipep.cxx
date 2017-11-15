/*
 * sipep.cxx
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
 * $Log: sipep.cxx,v $
 * Revision 1.29  2011-02-11 09:41:07  vfrolov
 * Added more tracing
 *
 * Revision 1.29  2011/02/11 09:41:07  vfrolov
 * Added more tracing
 *
 * Revision 1.28  2011/01/19 11:41:17  vfrolov
 * Replaced deprecated ApplyStringOptions() by OnApplyStringOptions()
 *
 * Revision 1.27  2011/01/13 06:39:08  vfrolov
 * Disabled OPAL version < 3.9.0
 * Added route options help topic
 *
 * Revision 1.26  2010/03/15 14:32:02  vfrolov
 * Added options
 *   --sip-t38-udptl-redundancy
 *   --sip-t38-udptl-keep-alive-interval
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
#include <iostream>
#include <fstream>

#include <opal_config.h>

#if OPAL_SIP
/////////////////////////////////////////////////////////////////////////////
#define PACK_VERSION(major, minor, build) (((((major) << 8) + (minor)) << 8) + (build))

#if !(PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= PACK_VERSION(3, 16, 1))
  #error *** Incompatible OPAL version (required >= 3.16.1) ***
#endif

#undef PACK_VERSION
/////////////////////////////////////////////////////////////////////////////

#include <sip/sipcon.h>

#include "sipep.h"
#include "manager.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////

MyRTPEndPoint::MyRTPEndPoint(MyManager & manager, OpalRTPEndPoint * endpoint)
  : MyManagerEndPoint(manager)
  , m_endpoint(*endpoint)
{
  cout << "MyRTPEndPoint" << endl;
}


bool MyRTPEndPoint::SetUIMode(const PCaselessString & str)
{
  if (str.IsEmpty())
    return true;

  if (str == "inband")
    m_endpoint.SetSendUserInputMode(OpalConnection::SendUserInputInBand);
  else if (str == "rfc2833")
    m_endpoint.SetSendUserInputMode(OpalConnection::SendUserInputAsRFC2833);
  else if (str == "signal" || str == "info-tone" || str == "h245-signal")
    m_endpoint.SetSendUserInputMode(OpalConnection::SendUserInputAsTone);
  else if (str == "string" || str == "info-string" || str == "h245-string")
    m_endpoint.SetSendUserInputMode(OpalConnection::SendUserInputAsString);
  else
    return false;

  return true;
}


PString MyRTPEndPoint::GetArgumentSpec()
{
  PString PrefixName = "sip";

  return  '-' + PrefixName + "-crypto:       Set crypto suites in priority order.\n"
          "-" + PrefixName + "-bandwidth:    Set total bandwidth (both directions) to be used for call\n"
          "-" + PrefixName + "-rx-bandwidth: Set receive bandwidth to be used for call\n"
          "-" + PrefixName + "-tx-bandwidth: Set transmit bandwidth to be used for call\n"
          "-" + PrefixName + "-ui:           Set User Indication mode (inband,rfc2833,signal,string)\n"
          "-" + PrefixName + "-option:       Set string option (key[=value]), may be multiple occurrences\n";
}


bool MyRTPEndPoint::Initialise(PArgList & args, ostream & output, bool verbose)
{
  PStringArray cryptoSuites = args.GetOptionString(m_endpoint.GetPrefixName() + "-crypto").Lines();
  if (!cryptoSuites.IsEmpty())
    m_endpoint.SetMediaCryptoSuites(cryptoSuites);

  if (verbose)
    output << m_endpoint.GetPrefixName().ToUpper() << " crypto suites: "
            << setfill(',') << m_endpoint.GetMediaCryptoSuites() << setfill(' ') << '\n';


  if (!m_endpoint.SetInitialBandwidth(OpalBandwidth::RxTx,
                                      args.GetOptionAs(m_endpoint.GetPrefixName() + "-bandwidth",
                                                       m_endpoint.GetInitialBandwidth(OpalBandwidth::RxTx))) ||
      !m_endpoint.SetInitialBandwidth(OpalBandwidth::Rx,
                                      args.GetOptionAs(m_endpoint.GetPrefixName() + "-rx-bandwidth",
                                                       m_endpoint.GetInitialBandwidth(OpalBandwidth::Rx))) ||
      !m_endpoint.SetInitialBandwidth(OpalBandwidth::Tx,
                                      args.GetOptionAs(m_endpoint.GetPrefixName() + "-tx-bandwidth",
                                                       m_endpoint.GetInitialBandwidth(OpalBandwidth::Tx)))) {
    output << "Invalid bandwidth for " << m_endpoint.GetPrefixName() << endl;
    return false;
  }


  if (!SetUIMode(args.GetOptionString(m_endpoint.GetPrefixName()+"-ui"))) {
    output << "Unknown user indication mode for " << m_endpoint.GetPrefixName() << endl;
    return false;
  }

  if (verbose)
    output << m_endpoint.GetPrefixName() << "user input mode: " << m_endpoint.GetSendUserInputMode() << '\n';


  m_endpoint.SetDefaultStringOptions(args.GetOptionString(m_endpoint.GetPrefixName() + "-option"));

  PStringArray interfaces = args.GetOptionString(m_endpoint.GetPrefixName()).Lines();
  if ((m_endpoint.GetListeners().IsEmpty() || !interfaces.IsEmpty()) && !m_endpoint.StartListeners(interfaces)) {
    output << "Could not start listeners for " << m_endpoint.GetPrefixName() << endl;
    return false;
  }

  if (verbose)
    output << m_endpoint.GetPrefixName() << " listening on: " << setfill(',') << m_endpoint.GetListeners() << setfill(' ') << '\n';

  return true;
}



/////////////////////////////////////////////////////////////////////////////

MySIPEndPoint::MySIPEndPoint(MyManager & manager)
  : SIPEndPoint(manager)
  , MyRTPEndPoint(manager, this)
{
  cout << "MySIPEndPoint" << endl;
}


void MySIPEndPoint::OnRegistrationStatus(const RegistrationStatus & status)
{
  PTime time;
  SIPEndPoint::OnRegistrationStatus(status);
  PTRACE(2, "MySIPEndPoint::OnRegistrationStatus() " << status.m_reason << " | " << status.m_userData);
  if (status.m_userData) {
    ofstream sipRegResultFile;
    PString *outFilePString = (PString*) status.m_userData;
    string outFile = *outFilePString;
    sipRegResultFile.open(outFile.c_str(),ios::out | ios::trunc);
    if (sipRegResultFile.is_open()) {
      sipRegResultFile << status.m_reason << endl; 
      sipRegResultFile << status.m_addressofRecord << endl; 
      sipRegResultFile << time.AsString(PTime::LongISO8601) << endl;
      sipRegResultFile << (status.m_reRegistering ? "Renewed registration" : "Initial registration") << endl;
      sipRegResultFile << status.m_productInfo.AsString() << endl;
      sipRegResultFile.close();
      PTRACE(2, "MySIPEndPoint::OnRegistrationStatus() file " << outFile << " written successfully");
    }
    else {
      PTRACE(2, "MySIPEndPoint::OnRegistrationStatus() open of " << outFile << " failed: " << strerror(errno));
    }
  }
  else {
    PTRACE(2, "MySIPEndPoint::OnRegistrationStatus() No status.m_userData");
  }


  unsigned reasonClass = status.m_reason/100;
  if (reasonClass == 1 || (status.m_reRegistering && reasonClass == 2))
    return;

   m_mgr.Broadcast(PSTRSTRM('\n' << status));
}


bool MySIPEndPoint::DoRegistration(ostream & output,
                                        bool verbose,
                                        const PString & aor,
                                        const PString & pwd,
                                        const PArgList & args,
                                        const char * authId,
                                        const char * realm,
                                        const char * proxy,
                                        const char * mode,
                                        const char * ttl,
                                        const char * resultFile)
{
  SIPRegister::Params params;
  params.m_addressOfRecord  = aor;
  params.m_password         = pwd;
  params.m_authID           = args.GetOptionString(authId);
  params.m_realm            = args.GetOptionString(realm);
  params.m_proxyAddress     = args.GetOptionString(proxy);

  PCaselessString str = args.GetOptionString(mode);
  if (str == "normal")
    params.m_compatibility = SIPRegister::e_FullyCompliant;
  else if (str == "single")
    params.m_compatibility = SIPRegister::e_CannotRegisterMultipleContacts;
  else if (str == "public")
    params.m_compatibility = SIPRegister::e_CannotRegisterPrivateContacts;
  else if (str == "ALG")
    params.m_compatibility = SIPRegister::e_HasApplicationLayerGateway;
  else if (str == "RFC5626")
    params.m_compatibility = SIPRegister::e_RFC5626;
  else if (!str.IsEmpty()) {
    output << "Unknown SIP registration mode \"" << str << '"' << endl;
    return false;
  }

  params.m_expire = args.GetOptionAs(ttl, 300);
  if (params.m_expire < 30) {
    output << "SIP registrar Time To Live must be more than 30 seconds\n";
    return false;
  }

  params.m_userData = new PString(args.GetOptionString(resultFile));

  if (verbose)
    output << "SIP registrar: " << flush;

  PString finalAoR;
  SIP_PDU::StatusCodes status;
  if (!Register(params, finalAoR, &status)) {
    output << "\nSIP registration to " << params.m_addressOfRecord
            << " failed (" << status << ')' << endl;
    return false;
  }

  if (verbose)
    output << finalAoR << endl;

  return true;
}

PString MySIPEndPoint::GetArgumentSpec()
{
  return "[SIP options:]"
          "-no-sip.           Disable SIP\n"
          "S-sip:             Listen on interface(s), defaults to udp$*:5060.\n"
          + MyRTPEndPoint::GetArgumentSpec() +
          "-sip-qos:          Set SIP Quality of Service to DSCP value or name\rDefaults to DF (Default Forwarding)\r"
          "Value can be 0-63\r"
          "Name as defined by RFC4594:\r"
          "    EF for Expedited Forwarding\r"
          "    DF for Default Forwarding\r"
          "    AFxx for Assured Forwarding, valid AF names:\r"
          "       AF11, AF12, AF13, AF21, AF22, AF23, AF31, AF32, AF33, AF41, AF42, AF43\r"
          "    CSn for Class Selector (n is 0-7)\n"

          "r-register:        Registration to server.\n"
          "-register-auth-id: Registration authorisation id, default is username.\n"
          "-register-realm:   Registration authorisation realm, default is any.\n"
          "-register-proxy:   Registration proxy, default is none.\n"
          "-register-ttl:     Registration Time To Live, default 300 seconds.\n"
          "-register-mode:    Registration mode (normal, single, public, ALG, RFC5626).\n"
          "-register-result:  Filename for registration result.\n"
          "-proxy:            Outbound proxy.\n";
}


bool MySIPEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  MyManager::LockedStream lockedOutput(m_mgr);
  ostream & output = lockedOutput;

  // Set up SIP
  if (args.HasOption("no-sip")) {
    if (verbose)
      output << "SIP protocol disabled.\n";
    return true;
  }

  if (!MyRTPEndPoint::Initialise(args, output, verbose))
    return false;

  if (args.HasOption("proxy")) {
    SetProxy(args.GetOptionString("proxy"), args.GetOptionString("user"), args.GetOptionString("password"));
    if (verbose)
      output << "SIP proxy: " << GetProxy() << '\n';
  }

  output << args << endl;
  if (args.HasOption("register")) {
    output << "Register\n";
    if (!DoRegistration(output, verbose,
                        args.GetOptionString("register"),
                        args.GetOptionString("password"),
                        args,
                        "register-auth-id",
                        "register-realm",
                        "register-proxy",
                        "register-mode",
                        "register-ttl",
                        "register-result"))
      return false;
  }

#if 0
  if (args.HasOption("sip-qos")) {
    output << "SIP QoS set to " << args.GetOptionString("sip-qos") << ".\n";
    SetMediaQoS(OpalMediaType::Audio(), args.GetOptionString("sip-qos"));
  }
  else {
    // By default set the QoS for Audio and T.38 to Expedited Forwarding (EF)
    SetMediaQoS(OpalMediaType::Audio(), PString("AF32"));
  }
#endif


  AddRoutesFor(this, defaultRoute);
  return true;
}

/////////////////////////////////////////////////////////////////////////////
#endif // OPAL_SIP
/////////////////////////////////////////////////////////////////////////////

