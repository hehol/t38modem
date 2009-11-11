/*
 * modemep.cxx
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
 * $Log: modemep.cxx,v $
 * Revision 1.13  2009-11-11 18:05:00  vfrolov
 * Added ability to apply options for incoming connections
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
    bool SwitchToFaxPassthrough(OpalConnection &connection);
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
ostream & operator<<(ostream & out, ModemConnection::PseudoModemMode mode)
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
  formats += T38ModemMediaStream::GetT38MediaFormat();

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

  if (mediaFormat == T38ModemMediaStream::GetT38MediaFormat()) {
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

  PString response = request("response");

  if (response != "confirm") {
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
  PTRACE(1, "ModemConnection::GetMediaFormats " << *this);

  OpalMediaFormatList mediaFormats = endpoint.GetMediaFormats();

  switch (requestedMode) {
    case pmmFax:
      for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
        if (mediaFormats[i].GetMediaType() == OpalMediaType::Audio()) {
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
      other->AdjustMediaFormats(otherMediaFormats, NULL);

      PTRACE(4, "ModemConnection::RequestMode: other connection formats: \n" <<
                setfill('\n') << otherMediaFormats << setfill(' '));

      if (!otherMediaFormats.HasType(OpalMediaType::Fax())) {
        PTRACE(3, "ModemConnection::RequestMode: other connection has not fax type");

        faxMode = false;
        done = SwitchToFaxPassthrough(*other);
      }
      else
      if (other->GetStringOptions().Contains("No-Force-T38-Mode")) {
        PTRACE(3, "ModemConnection::RequestMode: other connection has option No-Force-T38-Mode");

        faxMode = false;
        done = SwitchToFaxPassthrough(*other);
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

        PThread::Create(requestMode, (INT)TRUE);
        break;
      case pmmFaxNoForce: {
        PSafePtr<OpalConnection> other = GetOtherPartyConnection();

        if (other != NULL) {
          OpalMediaFormatList otherMediaFormats = other->GetMediaFormats();
          other->AdjustMediaFormats(otherMediaFormats, NULL);

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
        myPTRACE(1, "ModemConnection::RequestMode: " << *this << " unknown mode " << requestMode);
        requestedMode = oldMode;
        return FALSE;
    }

    break;
  }

  return TRUE;
}

bool ModemConnection::SwitchToFaxPassthrough(OpalConnection &connection)
{
  OpalMediaFormatList mediaFormats = connection.GetMediaFormats();
  connection.AdjustMediaFormats(mediaFormats, NULL);

  PTRACE(3, "ModemConnection::SwitchToFaxPassthrough:\n"
         << setfill('\n') << mediaFormats << setfill(' '));

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    if (mediaFormats[i] != OpalG711uLaw && mediaFormats[i] != OpalG711ALaw) {
      mediaFormats -= mediaFormats[i];
      i--;
    }
  }

  if (mediaFormats.GetSize() < 1) {
    PTRACE(2, "ModemConnection::SwitchToFaxPassthrough: G.711 is not supported");
    return false;
  }

  for (PINDEX i = 0 ; i < mediaFormats.GetSize() ; i++) {
    OpalMediaStreamPtr sink = connection.GetMediaStream(mediaFormats[i].GetMediaType(), false);

    if (sink == NULL)
      continue;

    if (!sink->IsOpen())
      continue;

    OpalMediaFormat sinkFormat = sink->GetMediaFormat();

    if (sinkFormat != mediaFormats[i])
      continue;

    PTRACE(3, "ModemConnection::SwitchToFaxPassthrough: sink=" << *sink << " sinkFormat=" << sinkFormat);

    PStringArray order;
    order += sinkFormat.GetName();
    order += '@' + OpalMediaType::Fax();

    OpalMediaFormatList sourceMediaFormats = GetMediaFormats();
    AdjustMediaFormats(sourceMediaFormats, NULL);
    connection.AdjustMediaFormats(sourceMediaFormats, this);
    sourceMediaFormats.Reorder(order);

    OpalMediaFormat sourceFormat;

    if (GetCall().SelectMediaFormats(
                                       OpalMediaType::Fax(),
                                       sourceMediaFormats,
                                       sinkFormat,
                                       GetLocalMediaFormats(),
                                       sourceFormat,
                                       sinkFormat))
    {
      PTRACE(3, "ModemConnection::SwitchToFaxPassthrough: selected "
             << sourceFormat << " --> " << sinkFormat);

      OpalMediaPatch *patch = sink->GetPatch();

      if (patch != NULL) {
        sink->RemovePatch(patch);
        patch->GetSource().Close();
      }

      unsigned sessionID = connection.GetNextSessionID(OpalMediaType::Fax(), false);

      PTRACE(3, "ModemConnection::SwitchToFaxPassthrough: using session " << sessionID << " on " << connection);

      OpalMediaStreamPtr source = OpenMediaStream(sourceFormat, sessionID, true);

      if (source != NULL) {
        patch = GetEndPoint().GetManager().CreateMediaPatch(*source,
                                       sink->RequiresPatchThread(source) && source->RequiresPatchThread(sink));

        if (patch != NULL) {
          patch->AddSink(sink);
          connection.AutoStartMediaStreams(true);
          PTRACE(3, "ModemConnection::SwitchToFaxPassthrough: fallback to " << sinkFormat << " - OK");
          return true;
        }
      }
    }

    break;
  }

  PTRACE(3, "ModemConnection::SwitchToFaxPassthrough: no sink for\n"
         << setfill('\n') << mediaFormats << setfill(' '));

  return false;
}
/////////////////////////////////////////////////////////////////////////////

