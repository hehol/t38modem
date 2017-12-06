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

#define new PNEW

extern const char Manufacturer[] = "Frolov,Holtschneider,Davidson";
extern const char Application[] = "T38Modem";
typedef MyManagerProcess<MyManager, Manufacturer, Application> MyApp;
PCREATE_PROCESS(MyApp);

/////////////////////////////////////////////////////////////////////////////
static PString GetListOfLibs()
{
  return
    PString("OPAL-")
    + PString(OPAL_VERSION)
    + PString("/")
    + OpalGetVersion()
#ifdef PTLIB_VERSION
    + PString(", PTLIB-")
    + PString(PTLIB_VERSION)
  #if PTLIB_MAJOR > 2 || (PTLIB_MAJOR == 2 && PTLIB_MINOR >= 6)
    + PString("/")
    + PProcess::GetLibVersion()
  #endif
#endif
#ifdef PWLIB_VERSION
    + PString(", PWLIB-") + PString(PWLIB_VERSION)
#endif
  ;
}
/////////////////////////////////////////////////////////////////////////////

static bool SetMediaFormatOption(ostream & output, bool verbose, const PString & format, const PString & name, const PString & value)
{
  if (format[0] == '@') {
    OpalMediaType mediaType = format.Mid(1);
    if (mediaType.empty()) {
      output << "Unknown media type \"" << format << '"' << endl;
      return false;
    }

    OpalMediaFormatList allFormats;
    OpalMediaFormat::GetAllRegisteredMediaFormats(allFormats);
    for (OpalMediaFormatList::iterator it = allFormats.begin(); it != allFormats.end(); ++it) {
      if (it->IsMediaType(mediaType) && !SetMediaFormatOption(output, verbose, it->GetName(), name, value))
        return false;
    }

    return true;
  }

  OpalMediaFormat mediaFormat(format);
  if (!mediaFormat.IsValid()) {
    output << "Unknown media format \"" << format << '"' << endl;
    return false;
  }

  if (!mediaFormat.HasOption(name)) {
    output << "Unknown option name \"" << name << "\" in media format \"" << format << '"' << endl;
    return false;
  }

  if (!mediaFormat.SetOptionValue(name, value)) {
    output << "Ilegal value \"" << value << "\""
              " for option name \"" << name << "\""
              " in media format \"" << format << '"' << endl;
    return false;
  }

  if (!OpalMediaFormat::SetRegisteredMediaFormat(mediaFormat)) {
    output << "Could not set registered media format \"" << format << '"' << endl;
    return false;
  }

  if (verbose)
    output << "Media format \"" << format << "\" option \"" << name << "\" set to \"" << value << "\"\n";

  return true;
}


static void PrintVersion(ostream & strm)
{
  const PProcess & process = PProcess::Current();
  strm << process.GetName()
        << " version " << process.GetVersion(true) << "\n"
          "  by   " << process.GetManufacturer() << "\n"
          "  on   " << process.GetOSClass() << ' ' << process.GetOSName()
        << " (" << process.GetOSVersion() << '-' << process.GetOSHardware() << ")\n"
          "  with PTLib v" << PProcess::GetLibVersion() << "\n"
          "  and  OPAL  v" << OpalGetVersion()
        << endl;
}

PString MyManager::GetArgumentSpec() 
{
  return "[Fax options:]"
         "a-audio. Send fax as G.711 audio.\n"
         "-disable-t38-mode. Disable T.38 mode, send fax as G.711 audio.\n"
         "A-no-audio. No audio phase at all, starts T.38 immediately.\n"
         "F-no-fallback. Do not fall back to audio if T.38 switch fails.\n"
         "e-switch-on-ced. Switch to T.38 on receipt of CED tone as caller.\n"
         "X-switch-time: Set fail safe T.38 switch time in seconds.\n"
         "-force-fax-mode. Force switch to T.38.\n"
         "-force-fax-mode-delay: Number of seconds to wait before forcing fax mode.\r"
         "Default is 7 seconds.\n"
         "-immediate-switch-on-receive. Switch to T.38 on receive right after call established.\n"
         "q-quiet. Only output error conditions.\n"
#if OPAL_STATISTICS
         "v-verbose. Output statistics during fax operation\n"
#endif
         "[Global options:]"
         "u-user:            Set local username, defaults to OS username.\n"
         "p-password:        Set password for authentication.\n"
         "-route:            Routes, may be used more than once.\r"
         "    pat=dst[;option[=value][;...]]\r"
         "Route the calls with incoming destination address\r"
         "matching the regexp pat to the outgoing\r"
         "destination address dst.\r"
         "All '<dn>' meta-strings found in dst or in\r"
         "following route options will be replaced by all\r"
         "valid consecutive E.164 digits from the incoming\r"
         "destination address. To strip N first digits use\r"
         "'<dn!N>' meta-string.\r"
         "If the specification is of the form @filename,\r"
         "then the file is read with each line consisting\r"
         "of a pat=dst[;...] route specification.\r"

         "[Audio options:]"
         "-jitter:           Set audio jitter buffer size (min[,max] default 50,250)\n"
         "-silence-detect:   Set audio silence detect mode (\"none\", \"fixed\" or default \"adaptive\")\n"
         "-no-inband-detect. Disable detection of in-band tones.\n"
#if OPAL_PTLIB_SSL
         "[SSL/TLS options:]"
         "-ssl-ca:           Set SSL/TLS certificate authority directory/file.\n"
         "-ssl-cert:         Set SSL/TLS certificate for local client.\n"
         "-ssl-key:          Set SSL/TLS private key lor local certificate.\n"
         "-ssl-no-create.    Do not auto-create SSL/TLS certificate/private key if does not exist.\n"
#endif
         "[T.38 options:]"
         "-T38FaxUdpEC:      Error Correction method for T.38 UDPTL, t38EDPFEC or t38UDPRedundancy.\r"
         "Default is t38UDPRedundancy.\n"
         "-" OPAL_UDPTLRedundancy ": Redundancy settings for T.38 UDPTL.\r"
         "maxsize:redundancy[,maxsize:redundancy[,maxsize:reduncancy...]]\r"
         "Sets the error correction redundancy for UDPTL packets by size.\r"
         "For example, the string '2:I,9:L,32767:H' (where I, L, and H are numbers)\r"
         "sets redundancy for (I)ndicators, (L)ow speed, and (H)igh speed packets.\r"
         "Default is '32767:1' or 1 packet of redndnacy for all packets.\n"
         "-" OPAL_UDPTLKeepAliveInterval ": Keep Alive Interval for T.38 UDPTL. Default is 0.\n"
         "-" OPAL_UDPTLRedundancyInterval ": Redundancy Interval for T.38 UDPTL. Default is 0.\n"
         "-" OPAL_UDPTLOptimiseRetransmit ". Optimise on Retransmit of T.38 UDPTL.\n"
         "-" OPAL_UDPTLRawMode ". Optimise on Retransmit of T.38 UDPTL.\n"
         "-T38FaxMaxDatagram: Maximum size datagram to use for T.38 UDPTL. Default is 1400.\n"
         "-T38FaxMaxBuffer: Maximum size T.38 Buffer. Default is 2000.\n"
         "[IP options:]"
#if OPAL_PTLIB_NAT
         "-nat-method:       Set NAT method, defaults to STUN\n"
         "-nat-server:       Set NAT server for the above method\n"
#if P_STUN
         "-stun:             Set NAT traversal STUN server\n"
#endif
         "-translate:        Set external IP address if masqueraded\n"
#endif
         "-portbase:         Set TCP/UDP/RTP port base\n"
         "-portmax:          Set TCP/UDP/RTP port max\n"
         "-tcp-base:         Set TCP port base (default 0)\n"
         "-tcp-max:          Set TCP port max (default base+99)\n"
         "-udp-base:         Set UDP port base (default 6000)\n"
         "-udp-max:          Set UDP port max (default base+199)\n"
         "-rtp-base:         Set RTP port base (default 5000)\n"
         "-rtp-max:          Set RTP port max (default base+199)\n"
         "-rtp-tos:          Set RTP packet IP TOS bits to n\n"
         "-rtp-size:         Set RTP maximum payload size in bytes.\n"
         "-aud-qos:          Set Audio RTP and T.38 Quality of Service to DSCP value or name\rDefaults to DF (Default Forwarding)\r"
         "Value can be 0-63\r"
         "Name as defined by RFC4594:\r"
         "    EF for Expedited Forwarding\r"
         "    DF for Default Forwarding\r"
         "    AFxx for Assured Forwarding, valid AF names:\r"
         "       AF11, AF12, AF13, AF21, AF22, AF23, AF31, AF32, AF33, AF41, AF42, AF43\r"
         "    CSn for Class Selector (n is 0-7)\n"
         "[Debug & General:]"
#if OPAL_STATISTICS
         "-statistics.       Output statistics periodically\n"
         "-stat-time:        Time between statistics output\n"
         "-stat-file:        File to output statistics too, default is stdout\n"
#endif
         PTRACE_ARGLIST
         "V-version.         Display application version.\n"
         "h-help.            This help message.\n"
         + MySIPEndPoint::GetArgumentSpec()
         + ModemEndPoint::GetArgumentSpec()
         ;
}

void MyManager::Usage(ostream & strm, const PArgList & args)
{
  strm << "\nt38modem [options]\n";
  args.Usage(strm);
}

bool MyManager::PreInitialise(PArgList & args, bool verbose)
{
  m_verbose = verbose;

  if (!args.IsParsed())
    args.Parse(GetArgumentSpec());

  if (!args.IsParsed() || args.HasOption("help")) {
    Usage(LockedOutput(), args);
    return false;
  }

  if (args.HasOption("version")) {
    PrintVersion(LockedOutput());
    return false;
  }

  PTRACE_INITIALISE(args);

  return true;
}

bool MyManager::Initialise(PArgList & args, bool verbose, const PString &defaultRoute)
{
  const PProcess & process = PProcess::Current();

  if (!PreInitialise(args, verbose))
    return false;

  LockedStream lockedOutput(*this);
  ostream & output = lockedOutput;

  output << endl;
  PrintVersion(output);
  output << endl;

  myPTRACE(1, "T38Modem\t" << process.GetName()
      << " Version " << process.GetVersion(TRUE)
      << " (" << GetListOfLibs() << ")"
      << " on " << process.GetOSClass() << " " << process.GetOSName()
      << " (" << process.GetOSVersion() << '-' << process.GetOSHardware() << ")");

  output << "args:" << endl;
  args.PrintOn(output);
  output << endl;
  output << endl;

  static char const * FormatMask[] = { "!G.711*", "!@fax", "!@userinput" };
  SetMediaFormatMask(PStringArray(PARRAYSIZE(FormatMask), FormatMask));

  if (args.HasOption("route")) {
    SetRouteTable(args.GetOptionString("route").Tokenise("\r\n", FALSE));
  }
 
  bool quiet = args.HasOption('q');
  if (quiet)
    output.rdbuf(NULL);

  if (args.HasOption("user")) {
    SetDefaultUserName(args.GetOptionString("user"));
  }
  if (verbose) {
    output << "Default user name: " << GetDefaultUserName();
    if (args.HasOption("password"))
      output << " (with password)";
    output << '\n';
  }

  if (args.HasOption("jitter")) {
    PStringArray params = args.GetOptionString("jitter").Tokenise("-,:",true);
    unsigned minJitter, maxJitter;
    switch (params.GetSize()) {
      case 1 :
        minJitter = maxJitter = params[0].AsUnsigned();
        break;

      case 2 :
        minJitter = params[0].AsUnsigned();
        maxJitter = params[1].AsUnsigned();
        break;

      default :
        output << "Invalid jitter specification\n";
        return false;
    }
    SetAudioJitterDelay(minJitter, maxJitter);
  }

  if (args.HasOption("silence-detect")) {
    OpalSilenceDetector::Params params = GetSilenceDetectParams();
    PCaselessString arg = args.GetOptionString("silence-detect");
    if (arg.NumCompare("adaptive") == EqualTo)
      params.m_mode = OpalSilenceDetector::AdaptiveSilenceDetection;
    else if (arg.NumCompare("fixed") == EqualTo)
      params.m_mode = OpalSilenceDetector::FixedSilenceDetection;
    else
      params.m_mode = OpalSilenceDetector::NoSilenceDetection;
    SetSilenceDetectParams(params);
  }

  if (args.HasOption("no-inband-detect"))
    DisableDetectInBandDTMF(true);

#if OPAL_PTLIB_SSL
  SetSSLCertificateAuthorityFiles(args.GetOptionString("ssl-ca", GetSSLCertificateAuthorityFiles()));
  SetSSLCertificateFile(args.GetOptionString("ssl-cert", GetSSLCertificateFile()));
  SetSSLPrivateKeyFile(args.GetOptionString("ssl-key", GetSSLPrivateKeyFile()));
  SetSSLAutoCreateCertificate(!args.HasOption("ssl-no-create"));
  if (verbose)
    output << "SSL/TLS certificate authority: " << GetSSLCertificateAuthorityFiles() << "\n"
              "SSL/TLS certificate: " << GetSSLCertificateFile() << "\n"
              "SSL/TLS private key: " << GetSSLPrivateKeyFile() << "\n"
              "SSL/TLS auto-create certificate/key: " << (GetSSLAutoCreateCertificate() ? "Yes" : "No") << '\n';
#endif

  if (args.HasOption("portbase")) {
    unsigned portbase = args.GetOptionString("portbase").AsUnsigned();
    unsigned portmax  = args.GetOptionString("portmax").AsUnsigned();
    SetTCPPorts  (portbase, portmax);
    SetUDPPorts  (portbase, portmax);
    SetRtpIpPorts(portbase, portmax);
  }

  if (args.HasOption("tcp-base"))
    SetTCPPorts(args.GetOptionString("tcp-base").AsUnsigned(),
                args.GetOptionString("tcp-max").AsUnsigned());

  if (args.HasOption("udp-base"))
    SetUDPPorts(args.GetOptionString("udp-base").AsUnsigned(),
                args.GetOptionString("udp-max").AsUnsigned());

  if (args.HasOption("rtp-base"))
    SetRtpIpPorts(args.GetOptionString("rtp-base").AsUnsigned(),
                  args.GetOptionString("rtp-max").AsUnsigned());

  if (args.HasOption("rtp-tos")) {
    unsigned tos = args.GetOptionString("rtp-tos").AsUnsigned();
    if (tos > 255) {
      output << "IP Type Of Service bits must be 0 to 255.\n";
      return false;
    }
    SetMediaTypeOfService(tos);
  }

  // Set the QoS for g.711 and T.38
  if (args.HasOption("aud-qos")) {
    output << "Audio QoS set to " << args.GetOptionString("aud-qos") << ".\n";
    //SetMediaQoS(OpalMediaType::Audio(), DSCP(args.GetOptionString("aud-qos")).String());
    SetMediaQoS(OpalMediaType::Audio(), args.GetOptionString("aud-qos"));
  }
  else {
    // By default set the QoS for Audio and T.38 to Default Forwarding (DF)
    SetMediaQoS(OpalMediaType::Audio(), PString("DF"));
  }

  if (args.HasOption("rtp-size")) {
    unsigned size = args.GetOptionString("rtp-size").AsUnsigned();
    if (size < 32 || size > 65500) {
      output << "RTP maximum payload size 32 to 65500.\n";
      return false;
    }
    SetMaxRtpPayloadSize(size);
  }

  OpalMediaFormat t38 = OpalT38;

  // Set the T.38 Error Correction method
  if (args.HasOption("T38FaxUdpEC")) {
    if (args.GetOptionString("T38FaxUdpEC") == "t38UDPFEC") {
      t38.SetOptionEnum("T38FaxUdpEC",0);
    }
    else if (args.GetOptionString("T38FaxUdpEC") == "t38UDPRedundancy") {
      t38.SetOptionEnum("T38FaxUdpEC",1);
    }
    else {
      output << "Bad T38FaxUdpEC: " << args.GetOptionString("T38FaxUdpEC") << endl;
      return false;
    }
  }
  output << "T38FaxUdpEC: " << (t38.GetOptionEnum("T38FaxUdpEC",1) == 0 ? "t38UDPFEC" : "t38UDPRedundancy") << endl;

  // Set the T.38 Max Datagram size
  if (args.HasOption("T38FaxMaxDatagram")) {
    t38.SetOptionInteger("T38FaxMaxDatagram",args.GetOptionString("T38FaxMaxDatagram").AsUnsigned());
  }
  output << "T38FaxMaxDatagram: " << t38.GetOptionInteger("T38FaxMaxDatagram") << endl;

  // Set the T.38 Max Buffer size
  if (args.HasOption("T38FaxMaxBuffer")) {
    t38.SetOptionInteger("T38FaxMaxBuffer",args.GetOptionString("T38FaxMaxBuffer").AsUnsigned());
  }
  output << "T38FaxMaxBuffer: " << t38.GetOptionInteger("T38FaxMaxBuffer") << endl;

  // Set the T.38 UDPTL Redundancy info
  if (args.HasOption(OPAL_UDPTLRedundancy)) {
    OpalMediaOptionString *Redun = new OpalMediaOptionString(OPAL_UDPTLRedundancy,false);
    t38.AddOption(Redun,false);
    t38.SetOptionString(OPAL_UDPTLRedundancy, args.GetOptionString(OPAL_UDPTLRedundancy));
  }
  output << OPAL_UDPTLRedundancy << ": " << t38.GetOptionString(OPAL_UDPTLRedundancy) << endl;

  // Set the T.38 UDPTL Redundancy Interval
  if (args.HasOption(OPAL_UDPTLRedundancyInterval)) {
    OpalMediaOptionInteger *Interval = new OpalMediaOptionInteger(OPAL_UDPTLRedundancyInterval,false);
    t38.AddOption(Interval,false);
    t38.SetOptionInteger(OPAL_UDPTLRedundancyInterval, args.GetOptionString(OPAL_UDPTLRedundancyInterval).AsInteger());
  }
  output << OPAL_UDPTLRedundancyInterval << ": " << t38.GetOptionInteger(OPAL_UDPTLRedundancyInterval,0) << endl;

  // Set the T.38 UDPTL Keep Alive Interval
  if (args.HasOption(OPAL_UDPTLKeepAliveInterval)) {
    OpalMediaOptionInteger *Interval = new OpalMediaOptionInteger(OPAL_UDPTLKeepAliveInterval,false);
    t38.AddOption(Interval,false);
    t38.SetOptionInteger(OPAL_UDPTLKeepAliveInterval, args.GetOptionString(OPAL_UDPTLKeepAliveInterval).AsInteger());
  }
  output << OPAL_UDPTLKeepAliveInterval << ": " << t38.GetOptionInteger(OPAL_UDPTLKeepAliveInterval,0) << endl;

  // Set T.38 UDPTL Optimize on Re-Transmit
  if (args.HasOption(OPAL_UDPTLOptimiseRetransmit)) {
    OpalMediaOptionBoolean *Interval = new OpalMediaOptionBoolean(OPAL_UDPTLOptimiseRetransmit,false);
    t38.AddOption(Interval,false);
    t38.SetOptionBoolean(OPAL_UDPTLOptimiseRetransmit, true);
  }
  output << OPAL_UDPTLOptimiseRetransmit << ": " << t38.GetOptionBoolean(OPAL_UDPTLOptimiseRetransmit,0) << endl;

  // Set T.38 UDPTL Raw Mode
  if (args.HasOption(OPAL_UDPTLRawMode)) {
    OpalMediaOptionBoolean *Interval = new OpalMediaOptionBoolean(OPAL_UDPTLRawMode,false);
    t38.AddOption(Interval,false);
    t38.SetOptionBoolean(OPAL_UDPTLRawMode, true);
  }
  output << OPAL_UDPTLRawMode << ": " << t38.GetOptionBoolean(OPAL_UDPTLRawMode,0) << endl;

  // Set the Registered Media Format for T.38
  OpalMediaFormat::SetRegisteredMediaFormat(t38);

  if (verbose)
    output << "TCP ports: " << GetTCPPortRange() << "\n"
              "UDP ports: " << GetUDPPortRange() << "\n"
              "RTP ports: " << GetRtpIpPortRange() << "\n"
              "Audio QoS: " << GetMediaQoS(OpalMediaType::Audio()) << "\n"
              "RTP payload size: " << GetMaxRtpPayloadSize() << '\n';

#if OPAL_PTLIB_NAT
  PString natMethod, natServer;
  if (args.HasOption("translate")) {
    natMethod = PNatMethod_Fixed::MethodName();
    natServer = args.GetOptionString("translate");
  }
#if P_STUN
  else if (args.HasOption("stun")) {
    natMethod = PSTUNClient::MethodName();
    natServer = args.GetOptionString("stun");
  }
#endif
  else if (args.HasOption("nat-method")) {
    natMethod = args.GetOptionString("nat-method");
    natServer = args.GetOptionString("nat-server");
  }
  else if (args.HasOption("nat-server")) {
#if P_STUN
    natMethod = PSTUNClient::MethodName();
#else
    natMethod = PNatMethod_Fixed::MethodName();
#endif
    natServer = args.GetOptionString("nat-server");
  }

  if (!natMethod.IsEmpty()) {
    if (verbose)
      output << natMethod << " server: " << flush;
    SetNATServer(natMethod, natServer);
    if (verbose) {
      PNatMethod * nat = GetNatMethods().GetMethodByName(natMethod);
      if (nat == NULL)
        output << "Unavailable";
      else {
        PNatMethod::NatTypes natType = nat->GetNatType();
        output << '"' << nat->GetServer() << "\" replies " << natType;
        PIPSocket::Address externalAddress;
        if (natType != PNatMethod::BlockedNat && nat->GetExternalAddress(externalAddress))
          output << " with external address " << externalAddress;
      }
      output << '\n';
    }
  }
#endif // OPAL_PTLIB_NAT

  if (verbose) {
    PIPSocket::InterfaceTable interfaceTable;
    if (PIPSocket::GetInterfaceTable(interfaceTable))
      output << "Detected " << interfaceTable.GetSize() << " network interfaces:\n"
               << setfill('\n') << interfaceTable << setfill(' ');
  }

  if (verbose)
    output << "---------------------------------\n";

  if (verbose) {
    OpalMediaFormatList formats = OpalMediaFormat::GetAllRegisteredMediaFormats();
    formats.Remove(GetMediaFormatMask());
    formats.Reorder(GetMediaFormatOrder());
    output << "Media Formats: " << setfill(',') << formats << setfill(' ') << '\n';
  }

#if 0
#if OPAL_STATISTICS
  m_statsPeriod.SetInterval(0, args.GetOptionString("stat-time").AsUnsigned());
  m_statsFile = args.GetOptionString("stat-file");
  if (m_statsPeriod == 0 && args.HasOption("statistics"))
    m_statsPeriod.SetInterval(0, 5);
#endif
#endif

  if (m_verbose)
    output.flush();

  output << "Fax Mode: ";
  if (args.HasOption('A')) {
    OpalMediaType::Fax()->SetAutoStart(OpalMediaType::ReceiveTransmit);
    OpalMediaType::Audio()->SetAutoStart(OpalMediaType::DontOffer);
    output << "Offer T.38 only";
  }
  else if (args.HasOption('a') || args.HasOption("disable-t38-mode"))
    output << "Audio Only";
  else
    output << "Switch to T.38";
  output << '\n';

  OpalConnection::StringOptions stringOptions;

  if (args.HasOption('F')) {
    stringOptions.SetBoolean(OPAL_NO_G711_FAX, true);
    output << "Disabled fallback to audio (G.711) mode on T.38 switch failure\n";
  }

  if (args.HasOption('e')) {
    stringOptions.SetBoolean(OPAL_SWITCH_ON_CED, true);
    output << "Enabled switch to T.38 on receipt of CED\n";
  }

  if (args.HasOption('X')) {
    unsigned seconds = args.GetOptionString('X').AsUnsigned();
    stringOptions.SetInteger(OPAL_T38_SWITCH_TIME, seconds);
    output << "Switch to T.38 after " << seconds << " seconds\n";
  }
  else if (args.HasOption("force-fax-mode")) {
    unsigned seconds = 7;
    if (args.HasOption("force-fax-mode-delay")) {
      seconds = args.GetOptionString("force-fax-mode-delay").AsInteger();
      if (seconds == 0)
        seconds = 1;
    }
    stringOptions.SetInteger(OPAL_T38_SWITCH_TIME, seconds);
    output << "Switch to T.38 after " << seconds << " seconds\n";
  }
  else
    output << "No T.38 switch timeout set\n";

  if (args.HasOption("immediate-switch-on-receive")) {
    stringOptions.SetBoolean("T38-Recv-Immed-Switch", true);
    output << "Immediate switch to T.38 on receive\n";
  }
  else {
    stringOptions.SetBoolean("T38-Recv-Immed-Switch", false);
    output << "No immediate switch to T.38 on receive\n";
  }

  SetDefaultConnectionOptions(stringOptions);

  m_showProgress = args.HasOption('v');

  output << "---------------------------------\n";

  epSIP = new MySIPEndPoint(*this);
  epFAX = new ModemEndPoint(*this);

  if (!epSIP->Initialise(args,verbose,""))
    return false;
  if (!epFAX->Initialise(args,verbose,""))
    return false;

  output << "---------------------------------\n\n";
  output << "Route table:" << endl;

  const RouteTable &routeTable = GetRouteTable();

  for (PINDEX i=0 ; i < routeTable.GetSize() ; i++) {
    cout << "  " << routeTable[i].GetPartyA() << "," << routeTable[i].GetPartyB() << "=" << routeTable[i].GetDestination() << endl;
  }

  return true;
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
    myPTRACE(1, "T38Modem\tCall[" << token << "] from " << a_party << " to " << b_party << ", no route!");
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
  myPTRACE(1, "T38Modem\tCall[" << token << "] from " << a_party << " to " << b_party << ", route to " << dst);

  return true;
}

void MyManager::Run()
{
#if OPAL_STATISTICS
  map<PString, OpalMediaStatistics> lastStatisticsByToken;
#endif

  while (!m_endRun.Wait(1000)) {
  }

  *LockedOutput() << "\nShutting down endpoints..." << endl;
}



void MyManager::OnClearedCall(OpalCall & call)
{
  cout << "Call[" << call.GetToken() << "] cleared (" << call.GetCallEndReasonText() << ")" << endl;
  myPTRACE(1, "T38Modem\tCall[" << call.GetToken() << "] cleared (" << call.GetCallEndReason() << ")");

  OpalManager::OnClearedCall(call);
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

void MyManager::EndRun(bool interrupt)
{
  myPTRACE(2, "T38Modem\tShutting down " << (interrupt ? " via interrupt" : " normally"));
  Broadcast(PSTRSTRM("\nShutting down " << PProcess::Current().GetName()
                     << (interrupt ? " via interrupt" : " normally") << "..."));

  m_interrupted = interrupt;
  m_endRun.Signal();
}


void MyManager::Broadcast(const PString & msg)
{
  if (m_verbose)
    *LockedOutput() << msg << endl;
}

/////////////////////////////////////////////////////////////////////////////

