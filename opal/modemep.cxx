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
 * Revision 1.4  2009-01-14 16:35:55  vfrolov
 * Added Calling-Party-Number for SIP
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

#include "../t38engine.h"
#include "../audio.h"
#include "../pmodem.h"
#include "../drivers.h"
#include "manager.h"
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
      const PString & remoteParty
    );
    ~ModemConnection();

    PBoolean Attach(PseudoModem *_pmodem);

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

    virtual PBoolean SendUserInputString(
      const PString & value                ///<  String value of indication
    );

  protected:
    PseudoModem *pmodem;
    AudioEngine * audioEngine;
    T38Engine * t38engine;
    int preparePacketTimeout;
};
/////////////////////////////////////////////////////////////////////////////
//
//  Implementation
//
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
        PString num = request("number");

        myPTRACE(1, "MyManager::OnMyCallback SetUpCall(" << num << ")");

        PString LocalPartyName = request("localpartyname");
        PString callToken;

        GetManager().SetUpCall(
            PString("modem:") + (LocalPartyName.IsEmpty() ? "" : LocalPartyName),
            num, callToken);

        PSafePtr<OpalCall> call = GetManager().FindCallWithLock(callToken);

        if (call != NULL) {
          PSafePtr<ModemConnection> pConn =
              PSafePtrCast<OpalConnection, ModemConnection>(call->GetConnection(0));

          if (pConn != NULL && pConn->Attach(modem)) {
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
      PSafePtr<OpalConnection> pConn = GetConnectionWithLock(request("calltoken"), PSafeReference);

      if (pConn != NULL) {
        if (request("mode") == "fax") {
          if (((MyManager &)GetManager()).OnRequestModeChange(*pConn, OpalMediaType::Fax()))
            response = "confirm";
        } else {
          myPTRACE(1, "ModemEndPoint::OnMyCallback unknown mode");
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

  return ((ModemConnection &)connection).SetReadTimeout(timeout);
}

PBoolean ModemEndPoint::MakeConnection(
    OpalCall & call,
    const PString & remoteParty,
    void * /*userData*/,
    unsigned int /*options*/,
    OpalConnection::StringOptions * /*stringOptions*/
    )
{
  myPTRACE(1, "ModemEndPoint::MakeConnection " << remoteParty);

  PWaitAndSignal wait(inUseFlag);
  PString token;

  for (int i = 0 ; i < 10000 ; i++) {
    token = remoteParty + "/" + call.GetToken() + "/" + PString(i);
    if (!connectionsActive.Contains(token)) {
      PSafePtr<ModemConnection> connection = new ModemConnection(call, *this, token, remoteParty);

      if (connection == NULL)
        return FALSE;

      connectionsActive.SetAt(connection->GetToken(), connection);

      return TRUE;
    }
  }

  return FALSE;
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
    const PString & remoteParty)
  : OpalConnection(call, ep, token),
    pmodem(NULL),
    audioEngine(NULL),
    t38engine(NULL),
    preparePacketTimeout(-1)
{
  remotePartyNumber = GetPartyName(remoteParty);
  remotePartyAddress = remoteParty;

  myPTRACE(1, "ModemConnection::ModemConnection " << *this);
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
      stringOptions.SetAt("Calling-Party-Number", remotePartyNumber);

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

  return endpoint.GetMediaFormats();
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

PBoolean ModemConnection::SendUserInputString(const PString & value)
{
  if (audioEngine == NULL)
    return FALSE;

  audioEngine->WriteUserInput(value);

  return TRUE;
}
/////////////////////////////////////////////////////////////////////////////

