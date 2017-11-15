/*
 * modemep.cxx
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
 * $Log: modemep.cxx,v $
 * Revision 1.32  2011-02-11 09:41:07  vfrolov
 * Added more tracing
 *
 * Revision 1.32  2011/02/11 09:41:07  vfrolov
 * Added more tracing
 *
 * Revision 1.31  2011/01/17 17:17:38  vfrolov
 * Disabled compiling with OPAL version != 3.9.0 (use SVN TRUNK 24174)
 *
 * Revision 1.30  2011/01/14 20:42:36  vfrolov
 * Added "srcname" to incoming call request
 * Thanks Dmitry (gorod225)
 *
 * Revision 1.29  2010/10/06 16:54:19  vfrolov
 * Redesigned engine opening/closing
 *
 * Revision 1.28  2010/09/29 11:52:59  vfrolov
 * Redesigned engine attaching/detaching
 *
 * Revision 1.27  2010/07/09 13:18:13  vfrolov
 * Fixed help message
 *
 * Revision 1.26  2010/07/09 07:12:36  vfrolov
 * Added "Trying alternate route ..." message
 *
 * Revision 1.25  2010/07/09 04:51:55  vfrolov
 * Implemented alternate route option (OPAL-Try-Next)
 * Added timeout option for SetUpPhase (OPAL-Set-Up-Phase-Timeout)
 *
 * Revision 1.24  2010/07/07 08:11:44  vfrolov
 * Fixed race condition with engine attaching
 *
 * Revision 1.23  2010/03/23 07:51:28  vfrolov
 * Added ability to disable auto fax mode forcing with OPAL-Force-Fax-Mode=false
 *
 * Revision 1.22  2010/03/19 08:36:05  vfrolov
 * Added forcing fax mode (to send CNG) if used fake audio encoder
 *
 * Revision 1.21  2010/03/15 13:40:28  vfrolov
 * Removed unused code
 *
 * Revision 1.20  2010/02/16 16:21:25  vfrolov
 * Added --force-fax-mode and --no-force-t38-mode options
 *
 * Revision 1.19  2010/02/02 08:41:56  vfrolov
 * Implemented ringing indication for voice class dialing
 *
 * Revision 1.18  2010/01/29 07:15:52  vfrolov
 * Fixed a "glare" condition with switcing to the fax passthrough
 * mode and receiving a T.38 mode request at the same time
 *
 * Revision 1.17  2010/01/13 09:59:19  vfrolov
 * Fixed incompatibility with OPAL trunk
 * Fixed incorrect codec selection for the incoming offer
 *
 * Revision 1.16  2009/12/23 17:50:57  vfrolov
 * Added missing calls to OnPatchMediaStream()
 *
 * Revision 1.15  2009/12/08 15:06:22  vfrolov
 * Fixed incompatibility with OPAL trunk
 *
 * Revision 1.14  2009/11/15 18:21:18  vfrolov
 * Replaced AutoStartMediaStreams() by more adequate code
 *
 * Revision 1.13  2009/11/11 18:05:00  vfrolov
 * Added ability to apply options for incoming connections
 *
 * Revision 1.12  2009/11/10 11:30:57  vfrolov
 * Implemented G.711 fallback to fax pass-through mode
 *
 * Revision 1.11  2009/11/02 18:02:19  vfrolov
 * Removed pre v3.7 compatibility code
 *
 * Revision 1.10  2009/10/16 19:22:42  vfrolov
 * Fixed race condition
 *
 * Revision 1.9  2009/07/22 14:42:49  vfrolov
 * Added Descriptions(args) to endpoints
 *
 * Revision 1.8  2009/07/13 15:08:17  vfrolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.7  2009/07/08 18:50:52  vfrolov
 * Added ability to use route pattern "modem:.*@tty"
 *
 * Revision 1.6  2009/04/09 10:02:19  vfrolov
 * Fixed ignoring tone for H.323 because duration always is zero
 *
 * Revision 1.5  2009/04/08 14:34:28  vfrolov
 * Replaced SendUserInputString() by SendUserInputTone() for compatibility
 * with OPAL SVN trunk
 *
 * Revision 1.4  2009/01/14 16:35:55  vfrolov
 * Added Calling-Party-Number for SIP
 *
 * Revision 1.3  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.2  2007/07/20 14:30:25  vfrolov
 * Moved GetPartyName() to opalutils.cxx
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>

#include <opal_config.h>
/////////////////////////////////////////////////////////////////////////////
#define PACK_VERSION(major, minor, build) (((((major) << 8) + (minor)) << 8) + (build))

#if !(PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= PACK_VERSION(3, 16, 1))
  #error *** Incompatible OPAL version (required >= 3.16.1) ***
#endif

#undef PACK_VERSION
/////////////////////////////////////////////////////////////////////////////

#include <opal/patch.h>

#include "../enginebase.h"
#include "../pmodem.h"
#include "../drivers.h"
#include "modemstrm.h"
#include "modemep.h"
#include "opalutils.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
class ModemConnection : public OpalFaxConnection
{
    PCLASSINFO(ModemConnection, OpalFaxConnection);
  public:

    ModemConnection(
      OpalCall      & call,
      ModemEndPoint & ep,
      const PString & remoteParty,
      void          * userData,
      bool            receiving,
      bool            disableT38,
      StringOptions * stringOptions
    );
    ~ModemConnection();

    virtual PString GetPrefixName() const;

    virtual OpalMediaStream * CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, PBoolean isSource);
    virtual OpalMediaFormatList GetMediaFormats() const;
    virtual void OnSwitchedFaxMediaStreams(bool toT38, bool success);
    virtual PBoolean SetUpConnection();
    virtual PBoolean SetAlerting(
      const PString & calleeName,   /// Name of endpoint being alerted.
      PBoolean withMedia            /// Open media with alerting
    );

  protected:
    PseudoModem *pmodem;
    bool isPartyA;
};
/////////////////////////////////////////////////////////////////////////////
//
//  Implementation
//
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
PStringToString ModemEndPoint::defaultStringOptions;

ModemEndPoint::ModemEndPoint(MyManager & mgr, const char * g711Prefix, const char * t38Prefix)
  : OpalFaxEndPoint(mgr, g711Prefix, t38Prefix)
  , MyManagerEndPoint(mgr)
{
  myPTRACE(1, "ModemEndPoint::ModemEndPoint");

  pmodem_pool = new PseudoModemQ();
}

PString ModemEndPoint::GetArgumentSpec()
{
  return 
    PseudoModemDrivers::ArgSpec() +
    "[Modem Options:]"
    "-no-modem.              Disable MODEM protocol.\n"
    "-ptty:                  Pseudo ttys. Can be used multiple times.\n"
    "-force-fax-mode.        Use OPAL-Force-Fax-Mode=true route option by defalt\n"
    "-force-fax-mode-delay:  Number of seconds to delay Force Fax Mode.\n"
    "-no-force-t38-mode.     Use OPAL-No-Force-T38-Mode=true route option by default\n";
}

bool ModemEndPoint::Initialise(PArgList & args, bool verbose, const PString & defaultRoute)
{
  if (args.HasOption("ptty")) {
    PString tty = args.GetOptionString("ptty");
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
        cerr << "Can't create modem for " << tty << endl;
    }
  }

  if (args.HasOption("force-fax-mode"))
    defaultStringOptions.SetAt("Force-Fax-Mode", "true");

  defaultStringOptions.SetAt("Force-Fax-Mode-Delay",
                               args.HasOption("force-fax-mode-delay")
                               ? args.GetOptionString("force-fax-mode-delay")
                               : "7");   // Default is 7 seconds

  if (args.HasOption("no-force-t38-mode"))
    defaultStringOptions.SetAt("No-Force-T38-Mode", "true");

  if (args.HasOption("audio"))
    m_prefix = "fax:";
  else
    m_prefix = "t38:";

  return TRUE;
}

void ModemEndPoint::OnMyCallback(PObject &from, INT myPTRACE_PARAM(extra))
{
  if (PIsDescendant(&from, PStringToString) ) {
    PStringToString &request = (PStringToString &)from;
    PString command = request("command");

    myPTRACE(1, "ModemEndPoint::OnMyCallback command=" << command << " extra=" << extra);

    PString modemToken = request("modemtoken");
    PString response = "reject";

    if (command == "dial") {
      PseudoModem *modem = pmodem_pool->Dequeue(modemToken);
      if (modem != NULL) {
        PString partyA = m_prefix + request("localpartyname");
        PString partyB = request("number") + "@" + modem->ttyName();

        PString originalPartyB;
        long tries = request("trynextcount").AsInteger();

        if (tries++ > 0) {
          originalPartyB = request("originalpartyb");
          cout << "Trying alternate route " << tries << " to " << partyB << " instead " << originalPartyB << endl;
          PTRACE(1, "Trying alternate route " << tries << " to " << partyB << " instead " << originalPartyB);
        } else {
          originalPartyB = partyB;
        }

        myPTRACE(1, "MyManager::OnMyCallback SetUpCall(" << partyA << ", " << partyB << ")");

        PSafePtr<OpalCall> call = GetManager().SetUpCall(partyA, partyB, modem);

        if (call != NULL) {
          PSafePtr<OpalConnection> pConn = call->GetConnection(0);

          if (pConn != NULL) {
            OpalConnection::StringOptions newOptions;
            newOptions.SetAt("Try-Next-Count", tries);
            newOptions.SetAt("Original-Party-B", originalPartyB);
            pConn->SetStringOptions(newOptions, false);

            request.SetAt("calltoken", pConn->GetToken());
            response = "confirm";
            modem = NULL;
          }
        }

        if (modem != NULL)
          pmodem_pool->Enqueue(modem);
      }
    } else if (command == "answer") {
      PSafePtr<ModemConnection> pConn =
          PSafePtrCast<OpalConnection, ModemConnection>(GetConnectionWithLock(request("calltoken"), PSafeReference));

      if (pConn != NULL) {
        pConn->AcceptIncoming();
        response = "confirm";
      }
    } else if (command == "clearcall") {
      PSafePtr<ModemConnection> pConn =
          PSafePtrCast<OpalConnection, ModemConnection>(GetConnectionWithLock(request("calltoken"), PSafeReference));

      if (pConn != NULL) {
        pConn->ClearCall();
        response = "confirm";
      }
    } else if (command == "addmodem") {
      if (pmodem_pool->Enqueue(modemToken))
        response = "confirm";
    }

    request.SetAt("response", response);

    myPTRACE(1, "ModemEndPoint::OnMyCallback request={\n" << request << "}");
  } else {
    myPTRACE(1, "ModemEndPoint::OnMyCallback unknown class " << from.GetClass() << " extra=" << extra);
  }
}

PseudoModem * ModemEndPoint::PMAlloc(const PString &number) const
{
  return pmodem_pool->DequeueWithRoute(number);
}

void ModemEndPoint::PMFree(PseudoModem *pmodem) const
{
  if (pmodem != NULL)
    pmodem_pool->Enqueue(pmodem);
}

PSafePtr<OpalConnection> ModemEndPoint::MakeConnection(
    OpalCall & call,
    const PString & remoteParty,
    void *userData,
    unsigned int /*options*/,
    OpalConnection::StringOptions * stringOptions
    )
{
  myPTRACE(1, "ModemEndPoint::MakeConnection " << remoteParty);

  OpalConnection::StringOptions localOptions;

  if (stringOptions == NULL)
    stringOptions = &localOptions;

  PINDEX iParams = remoteParty.Find(';');

  PString remotePartyAddress = remoteParty.Left(iParams);

  if (iParams != P_MAX_INDEX) {
    PStringToString params;

    PURL::SplitVars(remoteParty(iParams + 1, P_MAX_INDEX), params, ';', '=');

    for (PINDEX i = 0; i < params.GetSize(); i++) {
      PCaselessString key = params.GetKeyAt(i);

      if (key.NumCompare("OPAL-") == EqualTo) {
        stringOptions->SetAt(key.Mid(5), params.GetDataAt(i));
      }
    }
  }

  PINDEX prefixLength = remoteParty.Find(':');
  PStringArray tokens = remoteParty.Mid(prefixLength+1).Tokenise(";", true);

  bool receiving = false;
  PString stationId = GetDefaultDisplayName();

  for (PINDEX i = 1; i < tokens.GetSize(); ++i) {
    if (tokens[i] *= "receive")
      receiving = true;
    else if (tokens[i].Left(10) *= "stationid=")
      stationId = tokens[i].Mid(10);
  }

  PWaitAndSignal wait(PMutex inUseFlag);
  PString token;

  for (int i = 0 ; i < 10000 ; i++) {
    token = remotePartyAddress + "/" + call.GetToken() + "/" + PString(i);
    if (!m_connectionsActive.Contains(token)) {
      ModemConnection * connection =
          new ModemConnection(call,
                              *this,
                              remotePartyAddress,
                              userData,
                              receiving,
                              remoteParty.Left(prefixLength) *= GetPrefixName(),
                              stringOptions);

      PTRACE(5, "ModemEndPoint::MakeConnection new " << connection->GetClass() << ' ' << (void *)connection);

      OpalConnection::StringOptions newOptions;

      for (PINDEX i = 0 ; i < defaultStringOptions.GetSize() ; i++) {
        if (!connection->GetStringOptions().Contains(defaultStringOptions.GetKeyAt(i)))
          newOptions.SetAt(defaultStringOptions.GetKeyAt(i), defaultStringOptions.GetDataAt(i));
      }

      connection->SetStringOptions(newOptions, false);

      return AddConnection(connection);
    }
  }

  return AddConnection(NULL);
}

OpalMediaFormatList ModemEndPoint::GetMediaFormats() const
{
  myPTRACE(1, "ModemEndPoint::GetMediaFormats");

  OpalMediaFormatList formats;
  formats += OpalT38;
  return formats;
}

/////////////////////////////////////////////////////////////////////////////

PBoolean ModemConnection::SetUpConnection()
{
  myPTRACE(1, "ModemConnection::SetUpConnection " << *this);

  SetPhase(SetUpPhase);

  if (GetCall().GetConnection(0) == this) {
    OpalConnection::StringOptions stringOptions;

    if (!m_remotePartyNumber.IsEmpty())
      stringOptions.SetAt(OPAL_OPT_CALLING_PARTY_NUMBER, m_remotePartyNumber);

    if (!OnIncomingConnection(0, &stringOptions)) {
      Release(EndedByCallerAbort);
      return FALSE;
    }

    PTRACE(2, "Outgoing call routed to " << GetCall().GetPartyB() << " for " << *this);

    if (!GetCall().OnSetUp(*this)) {
      Release(EndedByNoAccept);
      return FALSE;
    }

    return TRUE;
  }

  PString srcNum;
  PString srcName;

  {
    PSafePtr<OpalConnection> other = GetOtherPartyConnection();

    if (other == NULL)
      return FALSE;

    srcNum = other->GetRemotePartyNumber();
    srcName = other->GetRemotePartyName();
  }

  PString dstNum = GetRemotePartyNumber();

  myPTRACE(1, "ModemConnection::SetUpConnection"
           << " dstNum=" << dstNum
           << " srcNum=" << srcNum
           << " srcName=" << srcName
           << " ...");

  ModemEndPoint &ep = (ModemEndPoint &)GetEndPoint();
  PseudoModem *_pmodem = ep.PMAlloc(dstNum);

  if (_pmodem == NULL) {
    myPTRACE(1, "... denied (all modems busy)");
    // LXK change -- Use EndedByLocalBusy instead of EndedByLocalUser
    // so that caller receives a normal busy signal instead of a
    // fast busy (congestion) or recorded message.
    Release(EndedByLocalBusy);
    return FALSE;
  }

  if (pmodem != NULL) {
    myPTRACE(1, "... denied (internal error)");
    ep.PMFree(_pmodem);
    Release(EndedByLocalBusy);  // LXK change -- see above
    return FALSE;
  }

  pmodem = _pmodem;

  PStringToString request;
  request.SetAt("command", "call");
  request.SetAt("calltoken", GetToken());
  request.SetAt("srcnum", srcNum);
  request.SetAt("srcname", srcName);
  request.SetAt("dstnum", dstNum);

  if (!pmodem->Request(request)) {
    myPTRACE(1, "... denied (modem is not ready)");	// or we can try other modem
    Release(EndedByLocalBusy);  // LXK change -- see above
    return FALSE;
  }

  if (request("response") != "confirm") {
    myPTRACE(1, "... denied (no confirm)");
    Release(EndedByLocalUser);
    return FALSE;
  }

  myPTRACE(1, "... Ok");

  SetPhase(AlertingPhase);
  OnAlerting();

  return TRUE;
}

PBoolean ModemConnection::SetAlerting(
    const PString & myPTRACE_PARAM(calleeName),
    PBoolean myPTRACE_PARAM(withMedia))
{
  myPTRACE(1, "ModemConnection::SetAlerting " << *this << " " << calleeName << " " << withMedia);

  SetPhase(AlertingPhase);

  PAssert(pmodem != NULL, "pmodem is NULL");

  PStringToString request;
  request.SetAt("command", "alerting");
  request.SetAt("calltoken", GetToken());
  if (!pmodem->Request(request)) {
    myPTRACE(1, "ModemConnection::SetAlerting error request={\n" << request << "}");
  }

  return TRUE;
}

ModemConnection::ModemConnection(OpalCall        & call,
                                 ModemEndPoint & ep,
                                 const PString & remoteParty,
                                 void          * userData,
                                 bool            receiving,
                                 bool            disableT38,
                                 StringOptions * stringOptions)
  : OpalFaxConnection(call, ep, PString::Empty(), receiving, disableT38, stringOptions)
  , pmodem((PseudoModem *)userData)
  , isPartyA(userData != NULL)
{
  m_remotePartyNumber = GetPartyName(remoteParty);
  PString remotePartyAddress = remoteParty;

  myPTRACE(4, "ModemConnection::ModemConnection " << *this);
}

ModemConnection::~ModemConnection()
{
  myPTRACE(4, "ModemConnection::~ModemConnection " << *this << " " << GetCallEndReason());

  if (pmodem != NULL) {
    PseudoModem *pmodemTmp = pmodem;

    ((ModemEndPoint &)GetEndPoint()).PMFree(pmodem);
    pmodem = NULL;

    PStringToString request;
    request.SetAt("command", "clearcall");
    request.SetAt("calltoken", GetToken());

    if (isPartyA) {
      switch (GetCallEndReason()) {
        case EndedByLocalUser:
        //  if (!phaseWasTimeout)
        //    break;
        case EndedByQ931Cause:
        case EndedByConnectFail:
        case EndedByGatekeeper:
        case EndedByNoBandwidth:
        case EndedByCapabilityExchange:
        case EndedByCallForwarded:
        case EndedByNoEndPoint:
        case EndedByHostOffline:
        case EndedByUnreachable:
        case EndedByTransportFail:
          if (GetStringOptions().Contains("Try-Next")) {
            PString num = GetStringOptions()("Try-Next");
            myPTRACE(1, "ModemConnection::~ModemConnection: Try-Next=" << num);
            request.SetAt("trynextcommand", "dial");
            request.SetAt("number", num);
            request.SetAt("localpartyname", m_remotePartyNumber);
            request.SetAt("trynextcount", GetStringOptions().Contains("Try-Next-Count") ? GetStringOptions()("Try-Next-Count") : "1");
            request.SetAt("originalpartyb", GetStringOptions().Contains("Original-Party-B") ? GetStringOptions()("Original-Party-B") : "unknown");
          }
          break;
        default:
          break;
      }
    }

    if (!pmodemTmp->Request(request)) {
      myPTRACE(1, "ModemConnection::~ModemConnection error request={\n" << request << "}");
    }
  }
}

OpalMediaStream * ModemConnection::CreateMediaStream(const OpalMediaFormat & mediaFormat, unsigned sessionID, bool isSource)
{
  myPTRACE(2, "ModemConnection::CreateMediaStream " << *this <<
      " mediaFormat=" << mediaFormat << " sessionID=" << sessionID << " isSource=" << isSource);

  if (mediaFormat == OpalT38) {
    if (pmodem != NULL) {
      T38Engine *t38engine = pmodem->NewPtrT38Engine();

      if (t38engine != NULL)
        return new T38ModemMediaStream(*this, sessionID, isSource, t38engine);
    }
  }
  return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource, isSource, true);
}

PString ModemConnection::GetPrefixName() const
{
  return m_disableT38 ? m_endpoint.GetPrefixName() : m_endpoint.GetT38Prefix();
}


OpalMediaFormatList ModemConnection::GetMediaFormats() const
{
  OpalMediaFormatList formats;

  formats += OpalT38;

  if (!m_disableT38) {
    formats += OpalPCM16;
    formats += OpalRFC2833;
    formats += OpalCiscoNSE;
  }

  PTRACE(2, "ModemConnection::GetMediaFormats " << formats);
  return formats;
}

void ModemConnection::OnSwitchedFaxMediaStreams(bool toT38, bool success)
{
  myPTRACE(1, "ModemConnection::OnSwitchedFaxMediaStreams toT38:" << toT38 << " success:" << success);
  if (success) {
    m_switchTimer.Stop(false);
    m_finalStatistics.m_fax.m_result = OpalMediaStatistics::FaxNotStarted;
  }
  else {
    if (toT38 && m_stringOptions.GetBoolean(OPAL_NO_G711_FAX)) {
      PTRACE(4, "FAX\tSwitch request to fax failed, checking for fall back to G.711");
      InternalOnFaxCompleted();
    }

    m_disableT38 = true;
  }
}

/////////////////////////////////////////////////////////////////////////////

