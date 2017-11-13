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
#if 0
    virtual void AdjustMediaFormats(bool local, const OpalConnection * otherConnection, OpalMediaFormatList & mediaFormats) const;
    virtual void OnEstablished();
    virtual void OnReleased();
    virtual void OnClosedMediaStream(const OpalMediaStream & stream);
    virtual void OnStopMediaPatch(OpalMediaPatch & patch);
    virtual PBoolean SendUserInputTone(char tone, unsigned duration);
    virtual void OnUserInputTone(char tone, unsigned duration);
    virtual bool SwitchFaxMediaStreams(bool toT38);
    virtual bool OnSwitchingFaxMediaStreams(bool toT38);
    virtual void OnApplyStringOptions();
  /**@name New operations */
  //@{
    /**Fax transmission/receipt completed.
       Default behaviour calls equivalent function on OpalFaxEndPoint.
      */
    virtual void OnFaxCompleted(
      bool failed   ///< Fax ended with failure
    );

    /**Get fax transmission/receipt statistics.
      */
    virtual void GetStatistics(
      OpalMediaStatistics & statistics  ///< Statistics for call
    ) const;

    /**Get receive fax flag.
      */
    bool IsReceive() const { return m_receiving; }
  //@}




    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, /// Media format for stream
      unsigned sessionID,                  /// Session number for stream
      PBoolean isSource                    /// Is a source stream
    );

    virtual bool IsNetworkConnection() const { return true; }

    virtual void OnReleased();

#endif

    virtual PBoolean SetUpConnection();

    virtual PBoolean SetAlerting(
      const PString & calleeName,   /// Name of endpoint being alerted.
      PBoolean withMedia            /// Open media with alerting
    );
#if 0

    virtual PBoolean SetConnected();

    virtual OpalMediaFormatList GetMediaFormats() const;

    virtual void OnConnected();
    virtual void OnEstablished();
    virtual void AcceptIncoming();

    virtual PBoolean SendUserInputTone(
      char tone,                    ///<  DTMF tone code
      unsigned duration = 0         ///<  Duration of tone in milliseconds
    );

    enum PseudoModemMode {
      pmmUnknown,
      pmmAny,
      pmmFax,
      pmmFaxNoForce,
    };
    bool RequestMode(
      PseudoModemMode mode
    );

    bool OnSwitchingFaxMediaStreams(bool toT38);

  protected:
    bool UpdateMediaStreams(OpalConnection &other);

    PDECLARE_NOTIFIER(PThread, ModemConnection, RequestMode);
    const PNotifier requestMode;
#endif
    PseudoModem *pmodem;
    bool isPartyA;
#if 0
    EngineBase *userInputEngine;
    PseudoModemMode requestedMode;

    PDECLARE_NOTIFIER(PTimer,  ModemConnection, OnPhaseTimeout);
    PTimer phaseTimer;
    Phases phaseTimerPhase;
    bool phaseWasTimeout;
#endif
};
/////////////////////////////////////////////////////////////////////////////
//
//  Implementation
//
/////////////////////////////////////////////////////////////////////////////
#if 0
#if PTRACING
static ostream & operator<<(ostream & out, ModemConnection::PseudoModemMode mode)
{
  switch (mode) {
    case ModemConnection::pmmAny:         return out << "any";
    case ModemConnection::pmmFax:         return out << "fax";
    case ModemConnection::pmmFaxNoForce:  return out << "fax-no-force";
    default:                              return out << "unknown" << INT(mode);
  }
}
#endif
#endif
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

#if 0
PStringArray ModemEndPoint::Descriptions()
{
  PStringArray descriptions = PString(
      "Modem options:\n"
      "  --no-modem                : Disable MODEM protocol.\n"
      "  -p --ptty [num@]tty[,...] : Pseudo ttys. Can be used multiple times.\n"
      "                              If tty prefixed by num@ then tty will\n"
      "                              accept incoming calls only\n"
      "                              for numbers with prefix num.\n"
      "                              Use none@tty to disable incoming calls.\n"
      "                              See Modem drivers section for tty format.\n"
      "  --force-fax-mode          : Use OPAL-Force-Fax-Mode=true route option by\n"
      "                              default.\n"
      "  --force-fax-mode-delay secs\n"
      "                            : Use OPAL-Force-Fax-Mode-Delay=secs route option by\n"
      "                              default.\n"
      "  --no-force-t38-mode       : Use OPAL-No-Force-T38-Mode=true route option by\n"
      "                              default.\n"
      "Modem route options:\n"
      "  OPAL-Set-Up-Phase-Timeout=secs\n"
      "    Set timeout for outgoing call Set-Up phase to secs seconds.\n"
      "  OPAL-Try-Next=dst\n"
      "    Set alternate incoming destination address for outgoing calls to dst. This\n"
      "    address will be used to re-route if outgoing call Set-Up phase fails.\n"
      "  OPAL-Force-Fax-Mode={true|false}\n"
      "    Enable or disable forcing fax mode (T.38 or G.711 pass-trough).\n"
      "  OPAL-Force-Fax-Mode-Delay=secs\n"
      "    Set Force-Fax-Mode to delay secs seconds.\n"
      "  OPAL-No-Force-T38-Mode={true|false}\n"
      "    Not enable or not disable forcing T.38 mode.\n"
      "Modem drivers:\n"
  ).Lines();

  PStringArray ds;

  ds = PseudoModemDrivers::Descriptions();

  for (PINDEX i = 0 ; i < ds.GetSize() ; i++)
    descriptions.Append(new PString(PString("  ") + ds[i]));

  return descriptions;
}

PStringArray ModemEndPoint::Descriptions(const PConfigArgs & /*args*/)
{
  PStringArray descriptions;

  return descriptions;
}
#endif

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
#if 0
        PString partyA = PString("t38:") + request("localpartyname");
#else
        PString partyA = m_prefix + request("localpartyname");
#endif
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
#if 0
    } else if (command == "requestmode") {
      PSafePtr<ModemConnection> pConn =
          PSafePtrCast<OpalConnection, ModemConnection>(GetConnectionWithLock(request("calltoken"), PSafeReference));

      if (pConn != NULL) {
        const PString &newModeString = request("mode");
        ModemConnection::PseudoModemMode mode;

        if (newModeString == "fax")           { mode = ModemConnection::pmmFax;         } else
        if (newModeString == "fax-no-force")  { mode = ModemConnection::pmmFaxNoForce;  } else
                                              { mode = ModemConnection::pmmUnknown;     }

        if (mode != ModemConnection::pmmUnknown) {
          if (pConn->RequestMode(mode))
            response = "confirm";
        } else {
          myPTRACE(1, "ModemEndPoint::OnMyCallback: unknown mode " << newModeString);
        }
      }
#endif
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
  //formats += OPAL_FAX_TIFF_FILE;
  return formats;
}
/////////////////////////////////////////////////////////////////////////////

#if 0
ModemConnection::ModemConnection(
    OpalCall & call,
    ModemEndPoint & ep,
    const PString & remoteParty,
    void          * userData,
    bool            receiving,
    bool            disableT38,
    StringOptions * stringOptions)
  : OpalFaxConnection(call, ep, "", receiving, disableT38, stringOptions)
#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif
  , requestMode(PCREATE_NOTIFIER(RequestMode))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
  , pmodem((PseudoModem *)userData)
  , userInputEngine(NULL)
  , requestedMode(pmmAny)
  , isPartyA(userData != NULL)
  , phaseTimerPhase(NumPhases)
  , phaseWasTimeout(false)
{
  m_remotePartyNumber = GetPartyName(remoteParty);
  PString remotePartyAddress = remoteParty;

  myPTRACE(4, "ModemConnection::ModemConnection " << *this);

  phaseTimer.SetNotifier(PCREATE_NOTIFIER(OnPhaseTimeout));
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
          if (!phaseWasTimeout)
            break;
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

  if (userInputEngine != NULL)
    ReferenceObject::DelPointer(userInputEngine);

  phaseTimer.Stop();
}

void ModemConnection::OnPhaseTimeout(PTimer &, INT)
{
  PTRACE(4, "ModemConnection::OnPhaseTimeout: for " << phaseTimerPhase << " on " << GetPhase());

  if (phaseTimerPhase == GetPhase()) {
    PTRACE(4, "ModemConnection::OnPhaseTimeout: clearing call");
    phaseWasTimeout = true;
    ClearCall();
  }
}

OpalMediaStream * ModemConnection::CreateMediaStream(
    const OpalMediaFormat & mediaFormat,
    unsigned sessionID,
    PBoolean isSource)
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
#if 1
  return new OpalNullMediaStream(*this, mediaFormat, sessionID, isSource, isSource, true);
#else
  else
  if (mediaFormat == OpalPCM16) {
    if (pmodem != NULL) {
      AudioEngine *audioEngine = pmodem->NewPtrAudioEngine();

      if (audioEngine != NULL)
        return new AudioModemMediaStream(*this, sessionID, isSource, audioEngine);
    }
  }
  else
  if (mediaFormat == OpalG711_ULAW_64K) {
    if (pmodem != NULL) {
      AudioEngine *audioEngine = pmodem->NewPtrAudioEngine();

      if (audioEngine != NULL)
        return new AudioModemMediaStream(*this, sessionID, isSource, audioEngine);
    }
  }
  else
  if (mediaFormat == OpalG711_ALAW_64K) {
    if (pmodem != NULL) {
      AudioEngine *audioEngine = pmodem->NewPtrAudioEngine();

      if (audioEngine != NULL)
        return new AudioModemMediaStream(*this, sessionID, isSource, audioEngine);
    }
  }

  return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);
#endif
}

void ModemConnection::OnReleased()
{
  myPTRACE(1, "ModemConnection::OnReleased " << *this);

  OpalConnection::OnReleased();
}

#endif

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

#if 0
    if (GetStringOptions().Contains("Set-Up-Phase-Timeout")) {
      long secs = GetStringOptions()("Set-Up-Phase-Timeout").AsInteger();

      PTRACE(4, "ModemConnection::SetUpConnection: Set-Up-Phase-Timeout=" << secs);

      if (secs > 0) {
        phaseTimerPhase = SetUpPhase;
        phaseTimer.SetInterval(0, secs);
      }
    }
#endif

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

#if 0

PBoolean ModemConnection::SetConnected()
{
  myPTRACE(1, "ModemConnection::SetConnected " << *this);

  return OpalConnection::SetConnected();
}

OpalMediaFormatList ModemConnection::GetMediaFormats() const
{
  PTRACE(4, "ModemConnection::GetMediaFormats " << *this);

  OpalMediaFormatList mediaFormats = m_endpoint.GetMediaFormats();

  switch (requestedMode) {
    case pmmFax:
      for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
        if (mediaFormats[i].GetMediaType() != OpalMediaType::Fax()) {
          PTRACE(3, "ModemConnection::GetMediaFormats Remove " << mediaFormats[i]);
          mediaFormats -= mediaFormats[i];
          i--;
        }
      }
      break;
    default:
      break;
  }

  PTRACE(3, "ModemConnection::GetMediaFormats mediaFormats " << mediaFormats);

  return mediaFormats;
}

void ModemConnection::OnConnected()
{
  myPTRACE(1, "ModemConnection::OnConnected " << *this);

  OpalConnection::OnConnected();
}

void ModemConnection::OnEstablished()
{
  myPTRACE(1, "ModemConnection::OnEstablished " << *this);

  PAssert(pmodem != NULL, "pmodem is NULL");
  PStringToString request;
  request.SetAt("command", "established");
  request.SetAt("calltoken", GetToken());
  if (!pmodem->Request(request)) {
    myPTRACE(1, "ModemConnection::OnEstablished error request={\n" << request << "}");
  }

  OpalConnection::OnEstablished();
}

void ModemConnection::AcceptIncoming()
{
  myPTRACE(1, "ModemConnection::AcceptIncoming " << *this);

  SetPhase(ConnectedPhase);

  OnConnected();
}

PBoolean ModemConnection::SendUserInputTone(char tone, unsigned PTRACE_PARAM(duration))
{
  PTRACE(4, "ModemConnection::SendUserInputTone " << tone << " " << duration);

  if (tone == ' ')
    return false;

  if (userInputEngine == NULL) {
    if (pmodem != NULL)
      userInputEngine = pmodem->NewPtrUserInputEngine();

    if (userInputEngine == NULL)
      return false;
  }

  userInputEngine->WriteUserInput(tone);

  return true;
}

void ModemConnection::RequestMode(PThread &, INT faxMode)
{
  PSafePtr<OpalConnection> other = GetOtherPartyConnection();

  if (other != NULL) {
    if (!LockReadWrite()) {
      myPTRACE(1, "ModemConnection::RequestMode " << *this << " Can't lock");
      return;
    }

    bool done = false;

    if (faxMode) {
      OpalMediaFormatList otherMediaFormats = other->GetMediaFormats();
      other->AdjustMediaFormats(false, NULL, otherMediaFormats);

      PTRACE(4, "ModemConnection::RequestMode: other connection formats: \n" <<
                setfill('\n') << otherMediaFormats << setfill(' '));

      if (!otherMediaFormats.HasType(OpalMediaType::Fax())) {
        PTRACE(3, "ModemConnection::RequestMode: other connection has not fax type");

        faxMode = false;
        done = UpdateMediaStreams(*other);
      }
      else
      if (GetStringOptions().GetBoolean("No-Force-T38-Mode")) {
        PTRACE(3, "ModemConnection::RequestMode: No-Force-T38-Mode=true");

        faxMode = false;
        done = UpdateMediaStreams(*other);
      }
    }

    if (!done && !other->SwitchFaxMediaStreams(faxMode)) {
      myPTRACE(1, "ModemConnection::RequestMode " << *this << " Change to mode " <<
                  (faxMode ? "fax" : "audio") << " failed");
    }

    UnlockReadWrite();
  }
}

bool ModemConnection::RequestMode(PseudoModemMode mode)
{
  int ForceFaxModeDelay = 0; 

  myPTRACE(1, "ModemConnection::RequestMode: " << *this << " " << mode);

  PTRACE(4, "ModemConnection::RequestMode: options: \n" <<
            setfill('\n') << GetStringOptions() << setfill(' '));

  PseudoModemMode oldMode = requestedMode;
  requestedMode = mode;

  for (;;) {
    switch (requestedMode) {
      case pmmAny:
        break;
      case pmmFax:
        if (!GetStringOptions().GetBoolean("Force-Fax-Mode", true)) {
          PTRACE(3, "ModemConnection::RequestMode: Force-Fax-Mode=false");
          requestedMode = mode;
          break;
        }

        PTRACE(3, "ModemConnection::RequestMode: force fax mode for other connection");

        if (isPartyA) {                                // only delay if we are the caller
          ForceFaxModeDelay = GetStringOptions()("Force-Fax-Mode-Delay").AsInteger();
          PTRACE(3, "ModemConnection::RequestMode: wait " << ForceFaxModeDelay << " seconds for force fax mode");
          if (ForceFaxModeDelay) {
            PThread::Sleep(ForceFaxModeDelay * 1000);  // Sleep some seconds to let other side hear our CNG
          }
        }

        PThread::Create(requestMode, (INT)true);
        break;
      case pmmFaxNoForce: {
        PSafePtr<OpalConnection> other = GetOtherPartyConnection();

        if (other != NULL) {
          OpalMediaFormatList otherMediaFormats = other->GetMediaFormats();
          other->AdjustMediaFormats(false, NULL, otherMediaFormats);

          PTRACE(4, "ModemConnection::RequestMode: other connection formats: \n" <<
                    setfill('\n') << otherMediaFormats << setfill(' '));

          if (!otherMediaFormats.HasType(OpalMediaType::Fax())) {
            PTRACE(3, "ModemConnection::RequestMode: other connection has not fax type");
            requestedMode = pmmFax;
            continue;
          }

          if (GetStringOptions().GetBoolean("Force-Fax-Mode")) {
            PTRACE(3, "ModemConnection::RequestMode: Force-Fax-Mode=true");
            requestedMode = pmmFax;
            continue;
          }

          bool force = false;

          OPAL_DEFINE_MEDIA_COMMAND(SearchForFakeTranscoder, "search_for_fake_transcoder", OpalMediaType::Audio());
          static const SearchForFakeTranscoder cmdSearchForFakeTranscoder;

          for (OpalMediaStreamPtr stream = GetMediaStream(OpalMediaType::Audio(), true) ;
               stream != NULL ;
               stream = GetMediaStream(OpalMediaType::Audio(), true, stream))
          {
            if (!stream->IsOpen())
              continue;

            OpalMediaPatch *patch = stream->GetPatch();

            if (patch == NULL)
              continue;

            OpalMediaStreamPtr otherStream = patch->GetSink();

            if (otherStream == NULL)
              continue;

            if (!otherStream->IsOpen())
              continue;

            if (!stream->ExecuteCommand(cmdSearchForFakeTranscoder)) {
              myPTRACE(4, "ModemConnection::RequestMode: found non-fake patch " << *patch);
              force = false;
              break;
            }

            myPTRACE(4, "ModemConnection::RequestMode: found fake patch " << *patch);
            force = true;
          }

          if (force) {
            myPTRACE(3, "ModemConnection::RequestMode: non-fake audio source not found");
            requestedMode = pmmFax;
            continue;
          }
        }
        break;
      }
      default:
        myPTRACE(1, "ModemConnection::RequestMode: " << *this << " unknown mode " << requestedMode);
        requestedMode = oldMode;
        return false;
    }

    break;
  }

  return true;
}

static OpalMediaStreamPtr FindMediaStream(
    const OpalConnection &connection,
    const OpalMediaFormatList &allowedFormats,
    bool source)
{
  for (OpalMediaStreamPtr stream = connection.GetMediaStream(OpalMediaType(), source) ;
       stream != NULL ;
       stream = connection.GetMediaStream(OpalMediaType(), source, stream))
  {
    if (!stream->IsOpen())
      continue;

    if (allowedFormats.HasFormat(stream->GetMediaFormat()))
      return stream;
  }

  return NULL;
}

static OpalMediaStreamPtr GetAllowedOtherStream(
    const OpalMediaStream &stream,
    const OpalMediaFormatList &allowedFormats)
{
  OpalMediaPatch *patch = stream.GetPatch();

  if (patch == NULL)
    return NULL;

  OpalMediaStreamPtr otherStream = (stream.IsSource()) ? patch->GetSink() : (OpalMediaStreamPtr)&patch->GetSource();

  if (otherStream == NULL)
    return NULL;

  if (!otherStream->IsOpen())
    return NULL;

  if (!allowedFormats.HasFormat(otherStream->GetMediaFormat()))
    return NULL;

  return otherStream;
}

static OpalMediaFormatList ReorderMediaFormats(
    const OpalMediaFormatList &mediaFormats,
    const OpalMediaFormat &mediaFormat)
{
  OpalMediaFormatList formats = mediaFormats;
  PStringArray order;

  order += mediaFormat.GetName();
  order += '@' + mediaFormat.GetMediaType();
  formats.Reorder(order);

  return formats;
}

bool ModemConnection::UpdateMediaStreams(OpalConnection &other)
{
  OpalMediaFormatList otherMediaFormats = other.GetMediaFormats();
  other.AdjustMediaFormats(true, NULL, otherMediaFormats);

  OpalMediaFormatList thisMediaFormats = GetMediaFormats();
  AdjustMediaFormats(true, NULL, thisMediaFormats);
  other.AdjustMediaFormats(true, this, thisMediaFormats);

  PTRACE(3, "ModemConnection::UpdateMediaStreams:\n"
            "patching " << setfill(',') << thisMediaFormats << setfill(' ') << "\n"
            "<------> " << setfill(',') << otherMediaFormats << setfill(' '));

  if (!thisMediaFormats.HasType(OpalMediaType::Audio())) {
    for (PINDEX i = 0 ; i < otherMediaFormats.GetSize() ; i++) {
      if (otherMediaFormats[i] != OpalG711uLaw &&
          otherMediaFormats[i] != OpalG711ALaw &&
          otherMediaFormats[i].GetMediaType() != OpalMediaType::Fax())
      {
        otherMediaFormats -= otherMediaFormats[i];
        i--;
      }
    }
  } else {
    for (PINDEX i = 0 ; i < otherMediaFormats.GetSize() ; i++) {
      if (otherMediaFormats[i].GetMediaType() != OpalMediaType::Audio() &&
          otherMediaFormats[i].GetMediaType() != OpalMediaType::Fax())
      {
        otherMediaFormats -= otherMediaFormats[i];
        i--;
      }
    }
  }

  PTRACE(4, "ModemConnection::UpdateMediaStreams:\n"
            "patching " << setfill(',') << thisMediaFormats << setfill(' ') << "\n"
            "<------> " << setfill(',') << otherMediaFormats << setfill(' '));

  if (otherMediaFormats.GetSize() < 1) {
    PTRACE(2, "ModemConnection::UpdateMediaStreams: other connection has no capable media formats");
    return false;
  }

  OpalMediaStreamPtr otherSink = FindMediaStream(other, otherMediaFormats, false);

  if (otherSink == NULL) {
    PTRACE(2, "ModemConnection::UpdateMediaStreams: other connection has no capable sink media streams");
    return false;
  }

  unsigned otherSinkSessionID = otherSink->GetSessionID();

  OpalMediaFormat otherSinkFormat = otherSink->GetMediaFormat();

  PTRACE(3, "ModemConnection::UpdateMediaStreams: "
            "otherSink=" << *otherSink << " "
            "otherSinkFormat=" << otherSinkFormat << " "
            "otherSinkSessionID=" << otherSinkSessionID);

  OpalMediaStreamPtr otherSource = FindMediaStream(other, otherMediaFormats, true);

  if (otherSource == NULL) {
    PTRACE(2, "ModemConnection::UpdateMediaStreams: other connection has no capable source media streams");
    return false;
  }

  OpalMediaFormat otherSourceFormat = otherSource->GetMediaFormat();

  PTRACE(3, "ModemConnection::UpdateMediaStreams: "
            "otherSource=" << *otherSource << " "
            "otherSourceFormat=" << otherSourceFormat);

  OpalMediaStreamPtr thisSource = GetAllowedOtherStream(*otherSink, thisMediaFormats);
  OpalMediaFormat thisSourceFormat;

  if (thisSource != NULL) {
    PTRACE(4, "ModemConnection::UpdateMediaStreams: no need to replace " << *thisSource << " --> " << *otherSink);
  } else {
    if (!GetCall().SelectMediaFormats(
                                       otherSinkFormat.GetMediaType(),
                                       ReorderMediaFormats(thisMediaFormats, otherSinkFormat),
                                       otherSinkFormat,
                                       GetLocalMediaFormats(),
                                       thisSourceFormat,
                                       otherSinkFormat))
    {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't select source format for sink " << otherSink);
      return false;
    }

    PTRACE(3, "ModemConnection::UpdateMediaStreams: selected source format "
           << thisSourceFormat << " for sink " << otherSinkFormat);
  }

  OpalMediaStreamPtr thisSink = GetAllowedOtherStream(*otherSource, thisMediaFormats);
  OpalMediaFormat thisSinkFormat;

  if (thisSink != NULL) {
    PTRACE(4, "ModemConnection::UpdateMediaStreams: no need to replace " << *thisSink << " <-- " << *otherSource);
  } else {
    if (!GetCall().SelectMediaFormats(
                                       otherSourceFormat.GetMediaType(),
                                       otherSourceFormat,
                                       ReorderMediaFormats(thisMediaFormats, otherSourceFormat),
                                       GetLocalMediaFormats(),
                                       otherSourceFormat,
                                       thisSinkFormat))
    {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't select sink format for source " << otherSource);
      return false;
    }

    PTRACE(3, "ModemConnection::UpdateMediaStreams: selected sink format "
           << thisSinkFormat << " for source " << otherSourceFormat);
  }

  if (thisSource == NULL) {
    OpalMediaPatch *patch = otherSink->GetPatch();

    if (patch != NULL) {
      otherSink->SetPatch(NULL);
    }

    PTRACE(4, "ModemConnection::UpdateMediaStreams: opening source for sink " << *otherSink);
    thisSource = OpenMediaStream(thisSourceFormat, otherSinkSessionID, true);

    if (thisSource == NULL) {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't open source stream");
      return false;
    }

    PTRACE(4, "ModemConnection::UpdateMediaStreams: creating patch for source " << *thisSource);
    patch = GetEndPoint().GetManager().CreateMediaPatch(*thisSource,
                otherSink->RequiresPatchThread(thisSource) && thisSource->RequiresPatchThread(otherSink));

    if (patch == NULL) {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't create patch for " << *thisSource);
      return false;
    }

    PTRACE(4, "ModemConnection::UpdateMediaStreams: adding otherSink to patch " << *patch);
    patch->AddSink(otherSink);

    other.OnPatchMediaStream(false, *patch);
    OnPatchMediaStream(true, *patch);

    PTRACE(3, "ModemConnection::UpdateMediaStreams: created patch " << *patch);
  }

  if (thisSink == NULL) {
    OpalMediaPatch *patch = otherSource->GetPatch();

    if (patch != NULL)
      otherSource->SetPatch(NULL);

    // NOTE: Both sinks must have the same session ID for T.38 <-> PCM transcoding !!!
    PTRACE(4, "ModemConnection::UpdateMediaStreams: opening sink for source " << *otherSource);
    thisSink = OpenMediaStream(thisSinkFormat, otherSinkSessionID, false);

    if (thisSink == NULL) {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't open sink stream");
      return false;
    }

    PTRACE(4, "ModemConnection::UpdateMediaStreams: creating patch for source " << *otherSource);
    patch = GetEndPoint().GetManager().CreateMediaPatch(*otherSource,
                thisSink->RequiresPatchThread(otherSource) && otherSource->RequiresPatchThread(thisSink));

    if (patch == NULL) {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't create patch for " << *otherSource);
      return false;
    }

    PTRACE(4, "ModemConnection::UpdateMediaStreams: adding thisSink to patch " << *patch);
    patch->AddSink(thisSink);

    other.OnPatchMediaStream(true, *patch);
    OnPatchMediaStream(false, *patch);

    PTRACE(3, "ModemConnection::UpdateMediaStreams: created patch " << *patch);
  }

  StartMediaStreams();

  PTRACE(3, "ModemConnection::UpdateMediaStreams: OK");

  return true;
}

bool ModemConnection::OnSwitchingFaxMediaStreams(bool toT38)
{
  bool faxMode = true;

    PTRACE(3, "ModemConnection::OnSwitchingFaxMediaStreams: Remote switch of media streams to " << (toT38 ? "T.38" : "audio") << " on " << *this);
    if (GetStringOptions().GetBoolean("No-Force-T38-Mode")) {
      PTRACE(3, "ModemConnection::OnSwitchingFaxMediaStreams: No-Force-T38-Mode=true");
      faxMode = false;
    }
    PTRACE(3, "ModemConnection::OnSwitchingFaxMediaStreams: return: " << !(toT38 && !faxMode));
    return !(toT38 && !faxMode);
}
#else
ModemConnection::ModemConnection(OpalCall        & call,
                                 ModemEndPoint & ep,
                                 const PString & remoteParty,
                                 void          * userData,
                                 bool            receiving,
                                 bool            disableT38,
                                 StringOptions * stringOptions)
  : OpalFaxConnection(call, ep, PString::Empty(), receiving, disableT38, stringOptions)
  //: OpalFaxConnection(call, ep, "faxfile", receiving, disableT38, stringOptions)
#if 0
#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif
  , requestMode(PCREATE_NOTIFIER(RequestMode))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
#endif
  , pmodem((PseudoModem *)userData)
  //, userInputEngine(NULL)
  //, requestedMode(pmmAny)
  , isPartyA(userData != NULL)
  //, phaseTimerPhase(NumPhases)
  //, phaseWasTimeout(false)
{
  m_remotePartyNumber = GetPartyName(remoteParty);
  PString remotePartyAddress = remoteParty;

  myPTRACE(4, "ModemConnection::ModemConnection " << *this);

  //phaseTimer.SetNotifier(PCREATE_NOTIFIER(OnPhaseTimeout));
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

  //if (userInputEngine != NULL)
  //  ReferenceObject::DelPointer(userInputEngine);

  //phaseTimer.Stop();
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


#if 0
void ModemConnection::AdjustMediaFormats(bool   local,
                           const OpalConnection * otherConnection,
                            OpalMediaFormatList & mediaFormats) const
{
  OpalFaxConnection::AdjustMediaFormats(local, otherConnection, mediaFormats);
}

void ModemConnection::OnApplyStringOptions()
{
  OpalFaxConnection::OnApplyStringOptions();
}

void ModemConnection::OnEstablished()
{
  OpalLocalConnection::OnEstablished();

  // If switched and we don't need to do CNG/CED any more, or T.38 is disabled
  // in which case the SpanDSP will deal with CNG/CED stuff.
  if (m_disableT38 || m_switchTime == 0)
    return;

  m_switchTimer.SetInterval(0, m_switchTime);
  PTRACE(3, "FAX\tStarting " << m_switchTime << " second timer for auto-switch to T.38");
}


void ModemConnection::OnReleased()
{
  m_switchTimer.Stop(false);

  OpalLocalConnection::OnReleased();

  // Probably not be necessary, but just in case of race conditions
  PTRACE_IF(4, !m_mediaStreams.IsEmpty(), "FAX", "Waiting for media streams to close.");
  while (!m_mediaStreams.IsEmpty())
    PThread::Sleep(20);

  InternalOnFaxCompleted();
}


void ModemConnection::OnClosedMediaStream(const OpalMediaStream & stream)
{
  OpalFaxConnection::OnClosedMediaStream(stream);
}


void ModemConnection::OnStopMediaPatch(OpalMediaPatch & patch)
{
  OpalMediaStreamPtr src = &patch.GetSource();
  if (&src->GetConnection() == this)
    GetEndPoint().GetManager().QueueDecoupledEvent(
              new PSafeWorkArg1<OpalConnection, OpalMediaStreamPtr, bool>(this, src, &OpalConnection::CloseMediaStream));
  OpalLocalConnection::OnStopMediaPatch(patch);
}


PBoolean ModemConnection::SendUserInputTone(char tone, unsigned duration)
{
  OnUserInputTone(tone, duration);
  return true;
}

void ModemConnection::OnUserInputTone(char tone, unsigned /*duration*/)
{
  // Not yet switched and got a CNG/CED from the remote system, start switch
  if (m_disableT38 || IsReleased())
    return;

  if (m_receiving ? (tone == 'X')
                  : (tone == 'Y' && m_stringOptions.GetBoolean(OPAL_SWITCH_ON_CED))) {
    PTRACE(3, "FAX\tRequesting mode change in response to " << (tone == 'X' ? "CNG" : "CED"));
    SwitchFaxMediaStreams(true);
  }
}


void ModemConnection::GetStatistics(OpalMediaStatistics & statistics) const
{
  if (m_finalStatistics.m_fax.m_result >= 0) {
    statistics = m_finalStatistics;
    return;
  }

  OpalMediaStreamPtr stream;
  if ((stream = GetMediaStream(OpalMediaType::Fax(), false)) == NULL &&
      (stream = GetMediaStream(OpalMediaType::Fax(), true )) == NULL) {

    PSafePtr<OpalConnection> other = GetOtherPartyConnection();
    if (other == NULL) {
      PTRACE(2, "FAX\tNo connection to get statistics.");
      return;
    }

    if ((stream = other->GetMediaStream(OpalMediaType::Fax(), false)) == NULL &&
        (stream = other->GetMediaStream(OpalMediaType::Fax(), true )) == NULL) {
      PTRACE(2, "FAX\tNo stream to get statistics.");
      return;
    }
  }

  stream->GetStatistics(statistics);
}


bool ModemConnection::SwitchFaxMediaStreams(bool enable)
{
  if (IsReleased())
    return false;

  m_switchTime = 0;
  m_switchTimer.Stop(false);
  m_completed = false;
  PSafePtr<OpalConnection> other = GetOtherPartyConnection();
  return other != NULL && other->SwitchFaxMediaStreams(enable);
}

#endif

void ModemConnection::OnSwitchedFaxMediaStreams(bool toT38, bool success)
{
  myPTRACE(1, "ModemConnection::OnSwitchedFaxMediaStreams toT38:" << toT38 << "success:" << success);
  if (success) {
    m_switchTimer.Stop(false);
    m_finalStatistics.m_fax.m_result = OpalMediaStatistics::FaxNotStarted;
  }
  else {
    if (toT38 && m_stringOptions.GetBoolean(OPAL_NO_G111_FAX)) {
      PTRACE(4, "FAX\tSwitch request to fax failed, checking for fall back to G.711");
      InternalOnFaxCompleted();
    }

    m_disableT38 = true;
  }
}

#if 0

bool ModemConnection::OnSwitchingFaxMediaStreams(bool toT38)
{
  if (toT38 && m_disableT38) {
    PTRACE(3, "FAX\tRequest to switch to T.38 disabled");
    return false;
  }

  PTRACE(4, "FAX\tRequest to switch to T.38 allowed");
  return true;
}

void ModemConnection::OnFaxCompleted(bool failed)
{
  m_endpoint.OnFaxCompleted(*this, failed);

  // Prevent to reuse filename
  m_filename.MakeEmpty();
}

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

#if 0
    if (GetStringOptions().Contains("Set-Up-Phase-Timeout")) {
      long secs = GetStringOptions()("Set-Up-Phase-Timeout").AsInteger();

      PTRACE(4, "ModemConnection::SetUpConnection: Set-Up-Phase-Timeout=" << secs);

      if (secs > 0) {
        phaseTimerPhase = SetUpPhase;
        phaseTimer.SetInterval(0, secs);
      }
    }
#endif

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
    myPTRACE(1, "... denied (modem is not ready)");     // or we can try other modem
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

#endif

#endif


/////////////////////////////////////////////////////////////////////////////

