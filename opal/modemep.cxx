/*
 * modemep.cxx
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
 * $Log: modemep.cxx,v $
 * Revision 1.18  2010-01-29 07:15:52  vfrolov
 * Fixed a "glare" condition with switcing to the fax passthrough
 * mode and receiving a T.38 mode request at the same time
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

#include <opal/buildopts.h>
#include <opal/patch.h>

#include "../t38engine.h"
#include "../audio.h"
#include "../pmodem.h"
#include "../drivers.h"
#include "modemstrm.h"
#include "modemep.h"
#include "opalutils.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
class ModemConnection : public OpalConnection
{
    PCLASSINFO(ModemConnection, OpalConnection);
  public:

    ModemConnection(
      OpalCall & call,
      ModemEndPoint & ep,
      const PString & token,
      const PString & remoteParty,
      void *userData
    );
    ~ModemConnection();

    virtual OpalMediaStream * CreateMediaStream(
      const OpalMediaFormat & mediaFormat, /// Media format for stream
      unsigned sessionID,                  /// Session number for stream
      PBoolean isSource                    /// Is a source stream
    );

    void SetReadTimeout(const PTimeInterval &timeout);

    virtual bool IsNetworkConnection() const { return true; }

    virtual void OnReleased();

    virtual PBoolean SetUpConnection();

    virtual PBoolean SetAlerting(
      const PString & calleeName,   /// Name of endpoint being alerted.
      PBoolean withMedia            /// Open media with alerting
    );

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

  protected:
    bool UpdateMediaStreams(OpalConnection &other);
    PBoolean Attach(PseudoModem *_pmodem);

    PDECLARE_NOTIFIER(PThread, ModemConnection, RequestMode);
    const PNotifier requestMode;

    PseudoModem *pmodem;
    AudioEngine * audioEngine;
    T38Engine * t38engine;
    int preparePacketTimeout;
    PseudoModemMode requestedMode;
};
/////////////////////////////////////////////////////////////////////////////
//
//  Implementation
//
/////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////////////////////////////
ModemEndPoint::ModemEndPoint(OpalManager & mgr, const char * prefix)
  : OpalEndPoint(mgr, prefix, CanTerminateCall)
{
  myPTRACE(1, "ModemEndPoint::ModemEndPoint");

  pmodem_pool = new PseudoModemQ();
}

PString ModemEndPoint::ArgSpec()
{
  return
    PseudoModemDrivers::ArgSpec() +
    "-no-modem."
    "p-ptty:"
  ;
}

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

PBoolean ModemEndPoint::Create(OpalManager & mgr, const PConfigArgs & args)
{
  if (args.HasOption("no-modem")) {
    cout << "Disabled MODEM protocol" << endl;
    return TRUE;
  }

  if ((new ModemEndPoint(mgr))->Initialise(args))
    return TRUE;

  return FALSE;
}

PBoolean ModemEndPoint::Initialise(const PConfigArgs & args)
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
        PString partyA = PString("modem:") + request("localpartyname");
        PString partyB = request("number") + "@" + modem->ttyName();

        myPTRACE(1, "MyManager::OnMyCallback SetUpCall(" << partyA << ", " << partyB << ")");

        PSafePtr<OpalCall> call = GetManager().SetUpCall(partyA, partyB, modem);

        if (call != NULL) {
          PSafePtr<OpalConnection> pConn = call->GetConnection(0);

          if (pConn != NULL) {
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

void ModemEndPoint::SetReadTimeout(
    OpalConnection &connection,
    const PTimeInterval &timeout)
{
  PAssert(PIsDescendant(&connection, ModemConnection), PInvalidCast);

  ((ModemConnection &)connection).SetReadTimeout(timeout);
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

  PINDEX iParams = remoteParty.Find(';');

  PString remotePartyAddress = remoteParty.Left(iParams);

  if (stringOptions != NULL && iParams != P_MAX_INDEX) {
    PStringToString params;

    PURL::SplitVars(remoteParty(iParams + 1, P_MAX_INDEX), params, ';', '=');

    for (PINDEX i = 0; i < params.GetSize(); i++) {
      PCaselessString key = params.GetKeyAt(i);

      if (key.NumCompare("OPAL-") == EqualTo) {
        stringOptions->SetAt(key.Mid(5), params.GetDataAt(i));
      }
    }
  }

  PWaitAndSignal wait(inUseFlag);
  PString token;

  for (int i = 0 ; i < 10000 ; i++) {
    token = remotePartyAddress + "/" + call.GetToken() + "/" + PString(i);
    if (!connectionsActive.Contains(token))
      return AddConnection(new ModemConnection(call, *this, token, remotePartyAddress, userData));
  }

  return AddConnection(NULL);
}

OpalMediaFormatList ModemEndPoint::GetMediaFormats() const
{
  myPTRACE(1, "ModemEndPoint::GetMediaFormats");

  OpalMediaFormatList formats;

  formats += OpalPCM16;
  formats += OpalT38;
  formats += OpalRFC2833;

  return formats;
}
/////////////////////////////////////////////////////////////////////////////
ModemConnection::ModemConnection(
    OpalCall & call,
    ModemEndPoint & ep,
    const PString & token,
    const PString & remoteParty,
    void *userData)
  : OpalConnection(call, ep, token)
#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif
  , requestMode(PCREATE_NOTIFIER(RequestMode))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
  , pmodem(NULL)
  , audioEngine(NULL)
  , t38engine(NULL)
  , preparePacketTimeout(-1)
  , requestedMode(pmmAny)
{
  remotePartyNumber = GetPartyName(remoteParty);
  remotePartyAddress = remoteParty;

  myPTRACE(1, "ModemConnection::ModemConnection " << *this);

  if (userData)
    Attach((PseudoModem *)userData);
}

ModemConnection::~ModemConnection()
{
  myPTRACE(1, "ModemConnection::~ModemConnection " << *this);

  if (pmodem != NULL) {
    if (t38engine != NULL)
      pmodem->Detach(t38engine);

    if (audioEngine != NULL)
      pmodem->Detach(audioEngine);

    PStringToString request;
    request.SetAt("command", "clearcall");
    request.SetAt("calltoken", GetToken());
    if( !pmodem->Request(request) ) {
      myPTRACE(1, "ModemConnection::~ModemConnection error request={\n" << request << "}");
    }

    ModemEndPoint &ep = (ModemEndPoint &)GetEndPoint();

    ep.PMFree(pmodem);
    pmodem = NULL;
  }

  if (t38engine != NULL)
    delete t38engine;

  if (audioEngine != NULL)
    delete audioEngine;
}

OpalMediaStream * ModemConnection::CreateMediaStream(
    const OpalMediaFormat & mediaFormat,
    unsigned sessionID,
    PBoolean isSource)
{
  myPTRACE(2, "ModemConnection::CreateMediaStream " << *this <<
      " mediaFormat=" << mediaFormat << " sessionID=" << sessionID << " isSource=" << isSource);

  if (mediaFormat == OpalT38) {
    PAssert(t38engine != NULL, "t38engine is NULL");

    if (!isSource) {
      PAssert(pmodem != NULL, "pmodem is NULL");

      pmodem->Attach(t38engine);
    }

    return new T38ModemMediaStream(*this, sessionID, isSource, t38engine);
  }

  if (mediaFormat == OpalPCM16) {
    PAssert(audioEngine != NULL, "audioEngine is NULL");

    if (!isSource) {
      PAssert(pmodem != NULL, "pmodem is NULL");
      pmodem->Attach(audioEngine);
    }

    return new AudioModemMediaStream(*this, mediaFormat, sessionID, isSource, audioEngine);
  }

  return OpalConnection::CreateMediaStream(mediaFormat, sessionID, isSource);
}

void ModemConnection::SetReadTimeout(const PTimeInterval &timeout)
{
  if (timeout > INT_MAX)
    preparePacketTimeout = -1;
  else
    preparePacketTimeout = (int)timeout.GetMilliSeconds();

  if (t38engine)
    t38engine->SetPreparePacketTimeout(preparePacketTimeout);
}

void ModemConnection::OnReleased()
{
  myPTRACE(1, "ModemConnection::OnReleased " << *this);

  OpalConnection::OnReleased();
}

PBoolean ModemConnection::Attach(PseudoModem *_pmodem)
{
  if (pmodem != NULL)
    return FALSE;

  pmodem = _pmodem;

  PAssert(pmodem != NULL, "pmodem is NULL");

  if (audioEngine == NULL)
    audioEngine = new AudioEngine(pmodem->ptyName());

  if (t38engine == NULL) {
    t38engine = new T38Engine(pmodem->ptyName());
    t38engine->SetPreparePacketTimeout(preparePacketTimeout);
  }

  return TRUE;
}

PBoolean ModemConnection::SetUpConnection()
{
  myPTRACE(1, "ModemConnection::SetUpConnection " << *this);

  SetPhase(SetUpPhase);

  if (GetCall().GetConnection(0) == this) {
    OpalConnection::StringOptions stringOptions;

    if (!remotePartyNumber.IsEmpty())
      stringOptions.SetAt(OPAL_OPT_CALLING_PARTY_NUMBER, remotePartyNumber);

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

  {
    PSafePtr<OpalConnection> other = GetOtherPartyConnection();

    if (other == NULL)
      return FALSE;

    srcNum = other->GetRemotePartyNumber();
  }

  PString dstNum = GetRemotePartyNumber();

  myPTRACE(1, "ModemConnection::SetUpConnection"
           << " dstNum=" << dstNum
           << " srcNum=" << srcNum
           << " ...");

  ModemEndPoint &ep = (ModemEndPoint &)GetEndPoint();
  PseudoModem *_pmodem = ep.PMAlloc(dstNum);

  if (_pmodem == NULL) {
    myPTRACE(1, "... denied (all modems busy)");
    Release(EndedByLocalUser);
    return FALSE;
  }

  if (!Attach(_pmodem)) {
    myPTRACE(1, "... denied (internal error)");
    ep.PMFree(_pmodem);
    Release(EndedByLocalUser);
    return FALSE;
  }

  PStringToString request;
  request.SetAt("command", "call");
  request.SetAt("calltoken", GetToken());
  request.SetAt("srcnum", srcNum);
  request.SetAt("dstnum", dstNum);

  if (!pmodem->Request(request)) {
    myPTRACE(1, "... denied (modem is not ready)");	// or we can try other modem
    Release(EndedByLocalUser);
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

  return TRUE;
}

PBoolean ModemConnection::SetConnected()
{
  myPTRACE(1, "ModemConnection::SetConnected " << *this);

  return OpalConnection::SetConnected();
}

OpalMediaFormatList ModemConnection::GetMediaFormats() const
{
  PTRACE(4, "ModemConnection::GetMediaFormats " << *this);

  OpalMediaFormatList mediaFormats = endpoint.GetMediaFormats();

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

  if (audioEngine == NULL || tone == ' ')
    return false;

  audioEngine->WriteUserInput(tone);

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
      other->AdjustMediaFormats(false, otherMediaFormats, NULL);

      PTRACE(4, "ModemConnection::RequestMode: other connection formats: \n" <<
                setfill('\n') << otherMediaFormats << setfill(' '));

      if (!otherMediaFormats.HasType(OpalMediaType::Fax())) {
        PTRACE(3, "ModemConnection::RequestMode: other connection has not fax type");

        faxMode = false;
        done = UpdateMediaStreams(*other);
      }
      else
      if (other->GetStringOptions().Contains("No-Force-T38-Mode")) {
        PTRACE(3, "ModemConnection::RequestMode: other connection has option No-Force-T38-Mode");

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
  myPTRACE(1, "ModemConnection::RequestMode: " << *this << " " << mode);

  PseudoModemMode oldMode = requestedMode;
  requestedMode = mode;

  for (;;) {
    switch (requestedMode) {
      case pmmAny:
        break;
      case pmmFax:
        PTRACE(3, "ModemConnection::RequestMode: force fax mode for other connection");

        PThread::Create(requestMode, (INT)true);
        break;
      case pmmFaxNoForce: {
        PSafePtr<OpalConnection> other = GetOtherPartyConnection();

        if (other != NULL) {
          OpalMediaFormatList otherMediaFormats = other->GetMediaFormats();
          other->AdjustMediaFormats(false, otherMediaFormats, NULL);

          PTRACE(4, "ModemConnection::RequestMode: other connection formats: \n" <<
                    setfill('\n') << otherMediaFormats << setfill(' '));

          if (!otherMediaFormats.HasType(OpalMediaType::Fax())) {
            PTRACE(3, "ModemConnection::RequestMode: other connection has not fax type");
            requestedMode = pmmFax;
            continue;
          }

          PTRACE(4, "ModemConnection::RequestMode: other connection options: \n" <<
                    setfill('\n') << other->GetStringOptions() << setfill(' '));

          if (other->GetStringOptions().Contains("Force-Fax-Mode")) {
            PTRACE(3, "ModemConnection::RequestMode: other connection has option Force-Fax-Mode");
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
  other.AdjustMediaFormats(true, otherMediaFormats, NULL);

  OpalMediaFormatList thisMediaFormats = GetMediaFormats();
  AdjustMediaFormats(true, thisMediaFormats, NULL);
  other.AdjustMediaFormats(true, thisMediaFormats, this);

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
      otherSink->RemovePatch(patch);
      patch->GetSource().Close();
    }

    thisSource = OpenMediaStream(thisSourceFormat, otherSinkSessionID, true);

    if (thisSource == NULL) {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't open source stream");
      return false;
    }

    patch = GetEndPoint().GetManager().CreateMediaPatch(*thisSource,
                otherSink->RequiresPatchThread(thisSource) && thisSource->RequiresPatchThread(otherSink));

    if (patch == NULL) {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't create patch for " << *thisSource);
      return false;
    }

    patch->AddSink(otherSink);

    other.OnPatchMediaStream(false, *patch);
    OnPatchMediaStream(true, *patch);

    PTRACE(3, "ModemConnection::UpdateMediaStreams: created patch " << *patch);
  }

  if (thisSink == NULL) {
    OpalMediaPatch *patch = otherSource->GetPatch();

    if (patch != NULL)
      otherSource->RemovePatch(patch);

    // NOTE: Both sinks must have the same session ID for T.38 <-> PCM transcoding !!!
    thisSink = OpenMediaStream(thisSinkFormat, otherSinkSessionID, false);

    if (thisSink == NULL) {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't open sink stream");
      return false;
    }

    patch = GetEndPoint().GetManager().CreateMediaPatch(*otherSource,
                thisSink->RequiresPatchThread(otherSource) && otherSource->RequiresPatchThread(thisSink));

    if (patch == NULL) {
      PTRACE(3, "ModemConnection::UpdateMediaStreams: can't create patch for " << *otherSource);
      return false;
    }

    patch->AddSink(thisSink);

    other.OnPatchMediaStream(true, *patch);
    OnPatchMediaStream(false, *patch);

    PTRACE(3, "ModemConnection::UpdateMediaStreams: created patch " << *patch);
  }

  StartMediaStreams();

  PTRACE(3, "ModemConnection::UpdateMediaStreams: OK");

  return true;
}
/////////////////////////////////////////////////////////////////////////////

