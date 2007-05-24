/*
 * main.cxx
 *
 * T38Modem simulator - main program
 *
 * Copyright (c) 2001-2007 Vyacheslav Frolov
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
 * The Initial Developer of the Original Code is Equivalence Pty Ltd
 *
 * Contributor(s): Vyacheslav Frolov
 *
 * $Log: h323ep.cxx,v $
 * Revision 1.49  2007-05-24 17:05:51  vfrolov
 * Fixed cout buffering delays
 *
 * Revision 1.49  2007/05/24 17:05:51  vfrolov
 * Fixed cout buffering delays
 *
 * Revision 1.48  2007/05/17 08:32:44  vfrolov
 * Moved class T38Modem from main.h and main.cxx to main_process.cxx
 *
 * Revision 1.47  2007/05/10 10:40:33  vfrolov
 * Added ability to continuously resend last UDPTL packet
 *
 * Revision 1.46  2007/03/23 10:14:35  vfrolov
 * Implemented voice mode functionality
 *
 * Revision 1.45  2007/03/01 14:03:06  vfrolov
 * Added ability to set range of ports to use
 *
 * Revision 1.44  2007/01/29 12:44:41  vfrolov
 * Added ability to put args to drivers
 *
 * Revision 1.43  2006/12/11 10:23:38  vfrolov
 * Fixed typo
 *
 * Revision 1.42  2006/04/29 20:31:28  dominance
 * add -v option to keep debian packaging happy ;)
 *
 * Revision 1.41  2005/04/28 09:12:07  vfrolov
 * Made tidy up
 *
 * Revision 1.40  2005/02/24 11:21:11  vfrolov
 * Added ability to put list to --route option
 *
 * Revision 1.39  2005/02/04 10:18:48  vfrolov
 * Fixed warnings for No Trace build
 *
 * Revision 1.38  2004/10/29 10:51:03  vfrolov
 * Added missing OS class to trace
 * Fixed MSVC compiler warning
 *
 * Revision 1.37  2004/10/27 13:22:42  vfrolov
 * Added flags to call to PTrace::Initialise()
 * Added version message to trace
 *
 * Revision 1.36  2004/10/20 13:40:03  vfrolov
 * Put date and time to trace
 *
 * Revision 1.35  2004/07/07 12:38:32  vfrolov
 * The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
 *
 * Revision 1.34  2004/05/09 07:46:11  csoutheren
 * Updated to compile with new PIsDescendant function
 *
 * Revision 1.33  2003/12/19 15:25:06  vfrolov
 * Removed class AudioDelay (utilized PAdaptiveDelay)
 * Renamed pmodemQ to pmodem_pool
 * Fixed modem loss on no route
 *
 * Revision 1.32  2003/12/04 11:23:51  vfrolov
 * Added "h323:" prefix to remoteParty
 *
 * Revision 1.31  2003/04/14 07:04:33  vfrolov
 * Added missing --disable and --prefer options.
 * Reported by Alexandre Aractingi.
 *
 * Revision 1.30  2003/04/07 06:52:43  vfrolov
 * Fixed --save option
 *
 * Revision 1.29  2002/12/20 10:12:46  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.28  2002/11/18 22:57:53  craigs
 * Added patches from Vyacheslav Frolov for CORRIGENDUM
 *
 * Revision 1.27  2002/11/10 09:22:47  robertj
 * Moved constants for "well known" ports to better place (OPAL change).
 *
 * Revision 1.26  2002/11/05 13:46:44  vfrolov
 * Added missed --username option to help
 * Utilized "localpartyname" option from "dial" request
 *
 * Revision 1.25  2002/05/22 12:01:36  vfrolov
 * Implemented redundancy error protection scheme
 *
 * Revision 1.24  2002/05/16 00:11:02  robertj
 * Changed t38 handler creation function for new API
 *
 * Revision 1.23  2002/05/15 16:17:29  vfrolov
 * Implemented per modem routing for I/C calls
 *
 * Revision 1.22  2002/04/30 11:05:24  vfrolov
 * Implemented T.30 Calling Tone (CNG) generation
 *
 * Revision 1.21  2002/04/30 04:06:06  craigs
 * Added option to set G.723.1 codec
 *
 * Revision 1.20  2002/04/27 10:20:41  vfrolov
 * Added ability to disable listen for incoming calls
 *
 * Revision 1.19  2002/04/19 14:09:48  vfrolov
 * Enabled T.38 mode request for O/G connections
 *
 * Revision 1.18  2002/04/18 08:04:13  vfrolov
 * Disabled the in band DTMF detection
 *
 * Revision 1.17  2002/04/17 08:55:18  vfrolov
 * Utilized trace output of H323Channel::Direction enum
 *
 * Revision 1.16  2002/03/22 09:39:13  vfrolov
 * Removed obsoleted option -f
 *
 * Revision 1.15  2002/03/05 12:40:23  vfrolov
 * Changed class hierarchy
 *   PseudoModem is abstract
 *   PseudoModemBody is child of PseudoModem
 *   Added PseudoModemQ::CreateModem() to create instances
 *
 * Revision 1.14  2002/03/01 09:45:03  vfrolov
 * Added Copyright header
 * Added sending "established" command on connection established
 * Implemented mode change on receiving "requestmode" command
 * Added setting lastReadCount
 * Added some other changes
 *
 * Revision 1.13  2002/02/12 11:25:23  vfrolov
 * Removed obsoleted code
 *
 * Revision 1.12  2002/02/11 08:48:43  vfrolov
 * Changed some trace and cout messages
 *
 * Revision 1.11  2002/01/10 06:16:00  craigs
 * Added muLaw codec as well as ALaw
 *
 * Revision 1.10  2002/01/10 06:09:47  craigs
 * Changed to use RequestModeChangeT38
 * Added MPL header
 *
 * 
 */

#include <ptlib.h>
#include <h323pdu.h>
#include <h323t38.h>

#include "main.h"
#include "t38engine.h"
#include "audio.h"
#include "pmodem.h"
#include "g7231_fake.h"
#include "drivers.h"

#define new PNEW

///////////////////////////////////////////////////////////////
PString MyH323EndPoint::ArgSpec()
{
  return     PseudoModemDrivers::ArgSpec() +
             "p-ptty:"
             "-route:"
             "-redundancy:"
             "-repeat:"
             "-old-asn."

             "F-fastenable."
             "T-h245tunneldisable."
             "G-g7231code."
             "D-disable:"            "P-prefer:"

             "g-gatekeeper:"         "n-no-gatekeeper."
             "-require-gatekeeper."  "-no-require-gatekeeper."
             "i-interface:"          "-no-interface."
             "-listenport:"          "-no-listenport."
             "-connectport:"         "-no-connectport."
             "-ports:"
             "u-username:"           "-no-username."
  ;
}

PStringArray MyH323EndPoint::Descriptions()
{
  PStringArray descriptions = PString(
        "  -p --ptty [num@]tty[,...] : Pseudo ttys (mandatory).\n"
        "                              Can be used multiple times.\n"
        "                              If tty prefixed by num@ then tty will\n"
        "                              accept incoming calls only\n"
        "                              for numbers with prefix num.\n"
        "                              Use none@tty to disable incoming calls.\n"
        "                              See Drivers section for supported tty's formats.\n"
        "  --route prefix@host[,...] : Route numbers with prefix num to host.\n"
        "                              Can be used multiple times.\n"
        "                              Discards prefix num from numbers.\n"
        "                              Use 'all' to route all numbers.\n"
        "                              Mandatory if not using GK.\n"
        "  --redundancy I[L[H]]      : Set redundancy for error recovery for\n"
        "                              (I)ndication, (L)ow speed and (H)igh\n"
        "                              speed IFP packets. I, L and H are digits.\n"
        "  --repeat ms               : Continuously resend last UDPTL packet each ms\n"
        "                              milliseconds.\n"
        "  --old-asn                 : Use original ASN.1 sequence in T.38 (06/98)\n"
        "                              Annex A (w/o CORRIGENDUM No. 1 fix).\n"
        "  -i --interface ip         : Bind to a specific interface.\n"
        "  --no-listenport           : Disable listen for incoming calls.\n"
        "  --listenport port         : Listen on a specific port.\n"
        "  --connectport port        : Connect to a specific port.\n"
        "  --ports T:B-M[,...]       : For (T)ype set (B)ase and (M)ax ports to use.\n"
        "                              T is 'udp', 'rtp' or 'tcp'. B and M are numbers.\n"
        "  -g --gatekeeper host      : Specify gatekeeper host.\n"
        "  -n --no-gatekeeper        : Disable gatekeeper discovery.\n"
        "  --require-gatekeeper      : Exit if gatekeeper discovery fails.\n"
        "  -F --fastenable           : Enable fast start.\n"
        "  -T --h245tunneldisable    : Disable H245 tunnelling.\n"
        "  -G --g7231enable          : Enable G.723.1 codec, rather than G.711.\n"
        "  -D --disable codec        : Disable the specified codec.\n"
        "                              Can be used multiple times.\n"
        "  -P --prefer codec         : Prefer the specified codec.\n"
        "                              Can be used multiple times.\n"
        "  -u --username str         : Set the local endpoint name to str.\n"
        "Drivers:\n"
  ).Lines();

  PStringArray ds;

  ds = PseudoModemDrivers::Descriptions();

  for (PINDEX i = 0 ; i < ds.GetSize() ; i++)
    descriptions.Append(new PString(PString("  ") + ds[i]));

  return descriptions;
}

BOOL MyH323EndPoint::Create(const PConfigArgs &args)
{
  MyH323EndPoint *endpoint = new MyH323EndPoint();

  PString userName;

  if (args.HasOption('u'))
    userName = args.GetOptionString('u');
  else
    userName = PProcess::Current().GetName() + " v" + PProcess::Current().GetVersion();

  endpoint->SetLocalUserName(userName);

  if (!endpoint->Initialise(args))
    return FALSE;

  // start the H.323 listener
  H323ListenerTCP * listener = NULL;
  if (!args.HasOption("no-listenport")) {
    PIPSocket::Address interfaceAddress(INADDR_ANY);
    WORD listenPort = H323EndPoint::DefaultTcpPort;

    if (args.HasOption("listenport"))
      listenPort = (WORD)args.GetOptionString("listenport").AsInteger();

    if (args.HasOption('i'))
      interfaceAddress = PIPSocket::Address(args.GetOptionString('i'));

    listener = new H323ListenerTCP(*endpoint, interfaceAddress, listenPort);

    if (!endpoint->StartListener(listener)) {
      cout << "Could not open H.323 listener port on "
           << listener->GetListenerPort() << endl;
      delete listener;
      return FALSE;
    }
  }

  if (args.HasOption('g')) {
    PString gkName = args.GetOptionString('g');
    if (endpoint->SetGatekeeper(gkName, new H323TransportUDP(*endpoint)))
      cout << "Gatekeeper set: " << *endpoint->GetGatekeeper() << endl;
    else {
      cout << "Error registering with gatekeeper at \"" << gkName << '"' << endl;
      return FALSE;
    }
  }
  else if (!args.HasOption('n')) {
    cout << "Searching for gatekeeper..." << flush;
    if (endpoint->DiscoverGatekeeper(new H323TransportUDP(*endpoint)))
      cout << "\nGatekeeper found: " << *endpoint->GetGatekeeper() << endl;
    else {
      cout << "\nNo gatekeeper found." << endl;
      if (args.HasOption("require-gatekeeper"))
        return FALSE;
    }
  }

  cout << "Waiting for incoming calls for \"" << endpoint->GetLocalUserName() << '"'
       << PString(listener ? "" : " disabled") << endl;

  return TRUE;
}

///////////////////////////////////////////////////////////////

MyH323EndPoint::MyH323EndPoint()
{
  pmodem_pool = new PseudoModemQ();
  //autoStartTransmitFax = TRUE;

  in_redundancy = -1;
  ls_redundancy = -1;
  hs_redundancy = -1;
  re_interval = -1;
  old_asn = FALSE;
}

void MyH323EndPoint::OnMyCallback(PObject &from, INT myPTRACE_PARAM(extra))
{
  if (PIsDescendant(&from, PStringToString) ) {
    PStringToString &request = (PStringToString &)from;
    PString command = request("command");

    myPTRACE(1, "MyH323EndPoint::OnMyCallback command=" << command << " extra=" << extra);

    PString modemToken = request("modemtoken");
    PString response = "reject";
  
    if (command == "dial") {
      PseudoModem *modem = pmodem_pool->Dequeue(modemToken);
      if (modem != NULL) {
        PString num = request("number");
        PString remote;

        if (GetGatekeeper() != NULL) {
          remote = num;
        } else {
          for( PINDEX i = 0 ; i < routes.GetSize() ; i++ ) {
            PString r = routes[i];
            PStringArray rs = r.Tokenise("@", FALSE);
            if( rs.GetSize() == 2 ) {
              if( rs[0] == "all" ) {
                remote = rs[1];
                break;
              } else if (num.Find(rs[0]) == 0) {
                remote = rs[1];
                num.Delete(0, rs[0].GetLength());
                break;
              }
            }
          }
        
          // add in the route and port if required
          if (!remote.IsEmpty()) {
            num += "@" + remote;
            if ((num.Find(':') == P_MAX_INDEX) && (connectPort != H323EndPoint::DefaultTcpPort))
              num += psprintf(":%i", connectPort);
          }
        }

        if (remote.IsEmpty()) 
          request.SetAt("diag", "noroute");
        else {
          PTRACE(1, "MyH323EndPoint::OnMyCallback MakeCall(" << num << ")");

          PString LocalPartyName = request("localpartyname");
          PString callToken;

          MakeCall("h323:" + num, callToken, (void *)(LocalPartyName.IsEmpty() ? NULL : (const char*)LocalPartyName));

          request.SetAt("calltoken", callToken);
          H323Connection * _conn = FindConnectionWithLock(callToken);

          if (_conn != NULL) {
            cout << "O/G connection to " << num;
            if (!LocalPartyName.IsEmpty())
              cout << " from " << LocalPartyName;
            cout << endl;
            PAssert(PIsDescendant(_conn, MyH323Connection), PInvalidCast);
            MyH323Connection *conn = (MyH323Connection *)_conn;
            if (conn->Attach(modem)) {
              response = "confirm";
              modem = NULL;
            }
            _conn->Unlock();
          }
        }
        if (modem != NULL)
          pmodem_pool->Enqueue(modem);
      }
    } else if (command == "answer") {
      PString callToken = request("calltoken");
      H323Connection * _conn = FindConnectionWithLock(callToken);
      if( _conn != NULL ) {
        _conn->AnsweringCall(H323Connection::AnswerCallNow);
        _conn->Unlock();
        response = "confirm";
      }
    } else if (command == "requestmode") {
      PString callToken = request("calltoken");
      H323Connection * _conn = FindConnectionWithLock(callToken);
      if( _conn != NULL ) {
        if (request("mode") == "fax") {
          if (_conn->RequestModeChangeT38()) {
            PTRACE(2, "MyH323EndPoint::OnMyCallback RequestMode T38 - OK");
            response = "confirm";
          } else {
            PTRACE(2, "MyH323EndPoint::OnMyCallback RequestMode T38 - fail");
          }
        } else {
          PTRACE(2, "MyH323EndPoint::OnMyCallback unknown mode");
        }
        _conn->Unlock();
      }
    } else if (command == "clearcall") {
      PString callToken = request("calltoken");
      if( ClearCall(callToken) ) {
        response = "confirm";
      }
    } else if (command == "addmodem") {
      if (pmodem_pool->Enqueue(modemToken)) {
        response = "confirm";
      }
    }

    request.SetAt("response", response);

    myPTRACE(1, "MyH323EndPoint::OnMyCallback request={\n" << request << "}");
  } else {
    myPTRACE(1, "MyH323EndPoint::OnMyCallback unknown class " << from.GetClass() << " extra=" << extra);
  }
}

H323Connection * MyH323EndPoint::CreateConnection(unsigned callReference, void *userData)
{
  MyH323Connection *connection = new MyH323Connection(*this, callReference);
  if (connection && userData)
    connection->SetLocalPartyName((const char *)userData);
  return connection;
}

PseudoModem * MyH323EndPoint::PMAlloc(const PString &number) const
{
  return pmodem_pool->DequeueWithRoute(number);
}

void MyH323EndPoint::PMFree(PseudoModem *pmodem) const
{
  if (pmodem != NULL)
    pmodem_pool->Enqueue(pmodem);
}

void MyH323EndPoint::SetOptions(MyH323Connection &/*conn*/, OpalT38Protocol *t38handler) const
{
  // TODO: make it per host

  if (t38handler != NULL) {
    PAssert(PIsDescendant(t38handler, T38Engine), PInvalidCast);

    ((T38Engine *)t38handler)->SetRedundancy(
        in_redundancy,
        ls_redundancy,
        hs_redundancy,
        re_interval);

    if (old_asn)
      ((T38Engine *)t38handler)->SetOldASN();
  }
}

BOOL MyH323EndPoint::Initialise(const PConfigArgs &args)
{
  DisableFastStart(!args.HasOption('F'));
  DisableH245Tunneling(args.HasOption('T'));
  DisableDetectInBandDTMF(TRUE);
  SetSilenceDetectionMode(H323AudioCodec::NoSilenceDetection);

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

  if (args.HasOption("connectport"))
    connectPort = (WORD)args.GetOptionString("connectport").AsInteger();
  else
    connectPort = H323EndPoint::DefaultTcpPort;

  if (args.HasOption("route")) {
    PString r = args.GetOptionString("route");
    routes = r.Tokenise(",\r\n", FALSE);

    cout << "Route O/G calls:" << endl;

    for( PINDEX i = 0 ; i < routes.GetSize() ; i++ ) {
      r = routes[i];
      PStringArray rs = r.Tokenise("@", FALSE);
      if( rs.GetSize() == 2 ) {
        cout << "  " << rs[0] << " --> " << rs[1] << endl;
        PTRACE(1, "Route " << rs[0] << " --> " << rs[1]);
        if( rs[0] == "all" )
          break;
      }
    }
  }
  
  if (args.HasOption('p')) {
    PString tty = args.GetOptionString('p');
    PStringArray ttys = tty.Tokenise(",\r\n ", FALSE);
    
    for( PINDEX i = 0 ; i < ttys.GetSize() ; i++ ) {
      tty = ttys[i];
      PStringArray atty = tty.Tokenise("@", FALSE);
      PString r = "";
      if (atty.GetSize() == 2) {
        r = atty[0];
        tty = atty[1];
      } else if (atty.GetSize() == 1) {
        tty = atty[0];
      }

      if (!pmodem_pool->CreateModem(tty, r, args, PCREATE_NOTIFIER(OnMyCallback)))
        cout << "Can't create modem for " << tty << endl;
    }
  }
  
  if (args.HasOption("redundancy")) {
    const char *r = args.GetOptionString("redundancy");
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

  if (args.HasOption("repeat"))
    re_interval = (int)args.GetOptionString("repeat").AsInteger();

  if (args.HasOption("old-asn"))
    old_asn = TRUE;

  if (args.HasOption('G')) 
    SetCapability(0, 0, new G7231_Fake_Capability());
  else {
    SetCapability(0, 0, new H323_G711Capability(H323_G711Capability::muLaw, H323_G711Capability::At64k));
    SetCapability(0, 0, new H323_G711Capability(H323_G711Capability::ALaw,  H323_G711Capability::At64k));
  }

  SetCapability(0, 0, new H323_T38Capability(H323_T38Capability::e_UDP));
  //SetCapability(0, 0, new H323_T38NonStandardCapability(181, 0, 18));

  SetCapability(0, 0, new H323_UserInputCapability(H323_UserInputCapability::BasicString));
  //AddAllUserInputCapabilities(0, P_MAX_INDEX);

  capabilities.Remove(args.GetOptionString('D').Lines());
  capabilities.Reorder(args.GetOptionString('P').Lines());

  cout << "Codecs (in preference order):\n" << setprecision(2) << capabilities << endl;

  return TRUE;
}

///////////////////////////////////////////////////////////////

MyH323Connection::MyH323Connection(MyH323EndPoint & _ep, unsigned callReference)
  : H323Connection(_ep, callReference), ep(_ep),
    pmodem(NULL), audioEngine(NULL)
{
}

MyH323Connection::~MyH323Connection()
{
  cout << "Closing connection" << endl;

  if (pmodem != NULL) {
    if (t38handler != NULL) {
      PAssert(PIsDescendant(t38handler, T38Engine), PInvalidCast);
      pmodem->Detach((T38Engine *)t38handler);
    }

    if (audioEngine != NULL)
      pmodem->Detach(audioEngine);

    PStringToString request;
    request.SetAt("command", "clearcall");
    request.SetAt("calltoken", GetCallToken());
    if( !pmodem->Request(request) ) {
      myPTRACE(1, "MyH323Connection::~MyH323Connection error request={\n" << request << "}");
    }

    ep.PMFree(pmodem);
  }

  if (audioEngine != NULL)
    delete audioEngine;
}

void MyH323Connection::OnEstablished()
{
  PAssert(pmodem != NULL, "pmodem is NULL");
  PStringToString request;
  request.SetAt("command", "established");
  request.SetAt("calltoken", GetCallToken());
  if( !pmodem->Request(request) ) {
    myPTRACE(1, "MyH323Connection::OnEstablished error request={\n" << request << "}");
  }
}

BOOL MyH323Connection::Attach(PseudoModem *_pmodem)
{
  PWaitAndSignal mutex(connMutex);

  if (pmodem != NULL)
    return FALSE;

  pmodem = _pmodem;

  if (audioEngine == NULL)
    audioEngine = new AudioEngine(pmodem->ptyName());

  pmodem->Attach(audioEngine);

  return TRUE;
}

OpalT38Protocol * MyH323Connection::CreateT38ProtocolHandler()
{
  PTRACE(2, "MyH323Connection::CreateT38ProtocolHandler");

  PWaitAndSignal mutex(connMutex);

  PAssert(pmodem != NULL, "pmodem is NULL");
  /*
   * we can't have more then one t38handler per connection
   * at the same time and we should delete it on connection clean
   */
  if( t38handler == NULL ) {
    PTRACE(2, "MyH323Connection::CreateT38ProtocolHandler create new one");
    t38handler = new T38Engine(pmodem->ptyName());
    ep.SetOptions(*this, t38handler);
    pmodem->Attach((T38Engine *)t38handler);
  }
  return t38handler;
}

H323Connection::AnswerCallResponse
     MyH323Connection::OnAnswerCall(const PString & /*caller*/,
                                    const H323SignalPDU & setupPDU,
                                    H323SignalPDU & /*connectPDU*/)
{
  PString number;
  cout << "I/C connection";
  PTRACE(1, "I/C connection");

  if (setupPDU.GetSourceE164(number)) {
    cout << " from " << number;
    PTRACE(1, "From: " << number);
  }

  if (setupPDU.GetDestinationE164(number)) {
    cout << " to " << number;
    PTRACE(1, "To: " << number);
  } else {
    number = "";
  }

  cout << endl;

  PseudoModem *_pmodem = ep.PMAlloc(number);

  if (_pmodem == NULL) {
    myPTRACE(1, "... denied (all modems busy)");
    return AnswerCallDenied;
  }

  if (!Attach(_pmodem)) {
    myPTRACE(1, "... denied (internal error)");
    ep.PMFree(_pmodem);
    return AnswerCallDenied;
  }

  RenameCurrentThread(pmodem->ptyName() + "(c)");

  PStringToString request;
  request.SetAt("command", "call");
  request.SetAt("calltoken", GetCallToken());
    
  PString srcNum;
  if( setupPDU.GetSourceE164(srcNum) )
    request.SetAt("srcnum", srcNum);
      
  PString dstNum;
  if( setupPDU.GetDestinationE164(dstNum) )
    request.SetAt("dstnum", dstNum);
      
  unsigned distinctiveRing = setupPDU.GetDistinctiveRing();
  if( distinctiveRing )
    request.SetAt("distinctivering", psprintf("%u", distinctiveRing));
    
  if( !pmodem->Request(request) ) {
    myPTRACE(1, "... denied (modem is not ready)");	// or we can try other modem
    return AnswerCallDenied;
  }

  PString response = request("response");

  if( response == "confirm" ) {
    AnswerCallResponse resp = AnswerCallPending;
    myPTRACE(1, "... Ok " << resp);
    return resp;
  }

  myPTRACE(1, "... denied (no confirm)");
  return AnswerCallDenied;
}

BOOL MyH323Connection::OnStartLogicalChannel(H323Channel & channel)
{
  myPTRACE(1, "MyH323Connection::OnStartLogicalChannel ch=" << channel << " cp=" << channel.GetCapability() << " sid=" << channel.GetSessionID() << " " << channel.GetDirection());

  if (!H323Connection::OnStartLogicalChannel(channel))
    return FALSE;

  cout
    << "Started logical channel: " << channel
    << " " << channel.GetCapability()
    << " " << channel.GetDirection()
    << endl;

  return TRUE;
}

void MyH323Connection::OnClosedLogicalChannel(const H323Channel & channel)
{
  PTRACE(2, "MyH323Connection::OnClosedLogicalChannel beg");
  
  H323Connection::OnClosedLogicalChannel(channel);

  myPTRACE(1, "MyH323Connection::OnClosedLogicalChannel ch=" << channel << " cp=" << channel.GetCapability() << " sid=" << channel.GetSessionID() << " " << channel.GetDirection());
}

BOOL MyH323Connection::OpenAudioChannel(BOOL isEncoding, unsigned /* bufferSize */, H323AudioCodec & codec)
{
  PTRACE(2, "MyH323Connection::OpenAudioChannel " << codec);

  PAssert(audioEngine != NULL, "audioEngine is NULL");

  codec.AttachChannel(audioEngine, FALSE);

  return TRUE;
}

void MyH323Connection::OnUserInputString(const PString & value)
{
  PAssert(audioEngine != NULL, "audioEngine is NULL");

  audioEngine->WriteUserInput(value);
}

// End of File ///////////////////////////////////////////////////////////////

