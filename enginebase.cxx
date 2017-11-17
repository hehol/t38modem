/*
 * enginebase.cxx
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
 * $Log: enginebase.cxx,v $
 * Revision 1.13  2010-10-12 16:46:25  vfrolov
 * Implemented fake streams
 *
 * Revision 1.13  2010/10/12 16:46:25  vfrolov
 * Implemented fake streams
 *
 * Revision 1.12  2010/10/06 16:54:19  vfrolov
 * Redesigned engine opening/closing
 *
 * Revision 1.11  2010/09/29 11:52:59  vfrolov
 * Redesigned engine attaching/detaching
 *
 * Revision 1.10  2010/09/22 15:07:45  vfrolov
 * Added ResetModemState() and OnResetModemState()
 *
 * Revision 1.9  2010/09/08 17:22:23  vfrolov
 * Redesigned modem engine (continue)
 *
 * Revision 1.8  2010/07/07 08:09:47  vfrolov
 * Added IsAttached()
 *
 * Revision 1.7  2010/03/18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.6  2009/11/19 14:48:28  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.5  2009/11/19 11:14:04  vfrolov
 * Added OnUserInput
 *
 * Revision 1.4  2009/11/18 19:08:47  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.3  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.2  2007/04/09 08:07:12  vfrolov
 * Added symbolic logging ModemCallbackParam
 *
 * Revision 1.1  2007/03/23 09:54:45  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>
#include "pmutils.h"
#include "enginebase.h"

#define new PNEW

///////////////////////////////////////////////////////////////
#if PTRACING
ostream & operator<<(ostream & out, EngineBase::DataType dataType)
{
  switch (dataType) {
    case EngineBase::dtNone:      return out << "dtNone";
    case EngineBase::dtRing:      return out << "dtRing";
    case EngineBase::dtBusy:      return out << "dtBusy";
    case EngineBase::dtCed:       return out << "dtCed";
    case EngineBase::dtCng:       return out << "dtCng";
    case EngineBase::dtSilence:   return out << "dtSilence";
    case EngineBase::dtHdlc:      return out << "dtHdlc";
    case EngineBase::dtRaw:       return out << "dtRaw";
  }

  return out << "dt" << INT(dataType);
}

ostream & operator<<(ostream & out, EngineBase::ModemCallbackParam param)
{
  switch (param) {
    case EngineBase::cbpUserDataMask:   break;
    case EngineBase::cbpOutBufNoFull:   return out << "cbpOutBufNoFull";
    case EngineBase::cbpUpdateState:    return out << "cbpUpdateState";
    case EngineBase::cbpReset:          return out << "cbpReset";
    case EngineBase::cbpOutBufEmpty:    return out << "cbpOutBufEmpty";
    case EngineBase::cbpUserInput:      return out << "cbpUserInput";
  }

  return out << "cbp" << INT(param);
}

ostream & operator<<(ostream & out, EngineBase::ModemClass modemClass)
{
  switch (modemClass) {
    case EngineBase::mcUndefined:       return out << "mcUndefined";
    case EngineBase::mcFax:             return out << "mcFax";
  }

  return out << "mc" << INT(modemClass);
}
#endif
///////////////////////////////////////////////////////////////
EngineBase::EngineBase(const PString &_name)
  : name(_name)
  , recvUserInput(NULL)
  , modemClass(mcUndefined)
  , hOwnerIn(NULL)
  , hOwnerOut(NULL)
  , firstIn(TRUE)
  , firstOut(TRUE)
  , isFakeOwnerIn(FALSE)
  , isFakeOwnerOut(FALSE)
  , isEnableFakeIn(FALSE)
  , isEnableFakeOut(FALSE)
{
}

EngineBase::~EngineBase()
{
  if (recvUserInput)
    delete recvUserInput;

  if (hOwnerIn != NULL)
    myPTRACE(1, "T38Modem\t" << name << " ~EngineBase WARNING: (In) still open by " << hOwnerIn);

  if (hOwnerOut != NULL)
    myPTRACE(1, "T38Modem\t" << name << " ~EngineBase WARNING: (Out) still open by " << hOwnerOut);

  if (!modemCallback.IsNULL())
    myPTRACE(1, "T38Modem\t" << name << " ~EngineBase WARNING: !modemCallback.IsNULL()");
}

PBoolean EngineBase::Attach(const PNotifier &callback)
{
  myPTRACE(1, "T38Modem\t" << name << " Attach");

  PWaitAndSignal mutexWait(Mutex);

  if (!modemCallback.IsNULL()) {
    myPTRACE(1, "T38Modem\t" << name << " Attach !modemCallback.IsNULL()");

    return FALSE;
  }

  modemCallback = callback;

  OnAttach();

  return TRUE;
}

void EngineBase::OnAttach()
{
  myPTRACE(1, "T38Modem\t" << name << " OnAttach Attached");

  OnResetModemState();
}

void EngineBase::Detach(const PNotifier &callback)
{
  myPTRACE(1, "T38Modem\t" << name << " Detach");

  PWaitAndSignal mutexWait(Mutex);

  if (modemCallback.IsNULL()) {
    myPTRACE(1, "T38Modem\t" << name << " Detach Already Detached");

    return;
  }

  if (modemCallback != callback) {
    myPTRACE(1, "T38Modem\t" << name << " Detach modemCallback != callback");

    return;
  }

  modemCallback = NULL;
  modemClass = mcUndefined;

  OnChangeModemClass();
  OnDetach();
}

void EngineBase::OnDetach()
{
  myPTRACE(1, "T38Modem\t" << name << " OnDetach Detached");

  OnResetModemState();
}

void EngineBase::ResetModemState() {
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  OnResetModemState();
}

void EngineBase::OnResetModemState()
{
  myPTRACE(1, "T38Modem\t" << name << " OnResetModemState");
}

void EngineBase::OpenIn(HOWNERIN hOwner, PBoolean fake)
{
  PWaitAndSignal mutexWait(Mutex);

  while (hOwnerIn != NULL) {
    if (hOwnerIn == hOwner) {
      myPTRACE(1, "T38Modem\t" << name << " OpenIn: re-open " << hOwner);
      return;
    }

    if (fake) {
      myPTRACE(1, "T38Modem\t" << name << " OpenIn: disabled close " << hOwnerIn << " by fake " << hOwner);
      return;
    }

    myPTRACE(1, "T38Modem\t" << name << " OpenIn " << (isFakeOwnerIn ? ": close fake " : "WARNING: close ") << hOwnerIn << " by " << hOwner);

    hOwnerIn = NULL;
    OnCloseIn();
  }

  myPTRACE(1, "T38Modem\t" << name << " OpenIn: open " << hOwner);

  hOwnerIn = hOwner;
  isFakeOwnerIn = fake;
  OnOpenIn();
}

void EngineBase::OpenOut(HOWNEROUT hOwner, PBoolean fake)
{
  PWaitAndSignal mutexWait(Mutex);

  while (hOwnerOut != NULL) {
    if (hOwnerOut == hOwner) {
      myPTRACE(1, "T38Modem\t" << name << " OpenOut: re-open " << hOwner);
      return;
    }

    if (fake) {
      myPTRACE(1, "T38Modem\t" << name << " OpenOut: disabled close " << hOwnerOut << " by fake " << hOwner);
      return;
    }

    myPTRACE(1, "T38Modem\t" << name << " OpenOut " << (isFakeOwnerOut ? ": close fake " : "WARNING: close ") << hOwnerOut << " by " << hOwner);

    hOwnerOut = NULL;
    OnCloseOut();
  }

  myPTRACE(1, "T38Modem\t" << name << " OpenOut: open " << hOwner);

  hOwnerOut = hOwner;
  isFakeOwnerOut = fake;
  OnOpenOut();
}

void EngineBase::OnOpenIn()
{
  firstIn = TRUE;
  ModemCallbackWithUnlock(cbpUpdateState);
}

void EngineBase::OnOpenOut()
{
  firstOut = TRUE;
  ModemCallbackWithUnlock(cbpUpdateState);
}

void EngineBase::CloseIn(HOWNERIN hOwner)
{
  PWaitAndSignal mutexWait(Mutex);

  if (hOwnerIn == hOwner) {
    myPTRACE(1, "T38Modem\t" << name << " CloseIn: close " << (isFakeOwnerIn ? "fake " : "") << hOwner);

    if (!isFakeOwnerIn)
      isEnableFakeIn = FALSE;  // allow re-enable fake stream

    hOwnerIn = NULL;
    OnCloseIn();
  } else {
    myPTRACE(1, "T38Modem\t" << name << " CloseIn: re-close " << hOwner);
  }
}

void EngineBase::CloseOut(HOWNEROUT hOwner)
{
  PWaitAndSignal mutexWait(Mutex);

  if (hOwnerOut == hOwner) {
    myPTRACE(1, "T38Modem\t" << name << " CloseOut: close " << (isFakeOwnerOut ? "fake " : "") << hOwner);

    if (!isFakeOwnerOut)
      isEnableFakeOut = FALSE;  // allow re-enable fake stream

    hOwnerOut = NULL;
    OnCloseOut();
  } else {
    myPTRACE(1, "T38Modem\t" << name << " CloseOut: re-close " << hOwner);
  }
}

void EngineBase::OnCloseIn()
{
  ModemCallbackWithUnlock(cbpUpdateState);
}

void EngineBase::OnCloseOut()
{
  ModemCallbackWithUnlock(cbpUpdateState);
}

void EngineBase::EnableFakeIn(PBoolean enable)
{
  PWaitAndSignal mutexWait(Mutex);

  if (isEnableFakeIn == enable)
    return;

  myPTRACE(3, "T38Modem\t" << name << " EnableFakeIn: " << (enable ? "enable" : "disable"));

  isEnableFakeIn = enable;
  OnChangeEnableFakeIn();
}

void EngineBase::OnChangeEnableFakeIn()
{
  if (!isEnableFakeIn && hOwnerIn != NULL && isFakeOwnerIn) {
    myPTRACE(1, "T38Modem\t" << name << " OnChangeEnableFakeIn: close fake " << hOwnerIn);

    hOwnerIn = NULL;
    OnCloseIn();
  }
}

void EngineBase::EnableFakeOut(PBoolean enable)
{
  PWaitAndSignal mutexWait(Mutex);

  if (isEnableFakeOut == enable)
    return;

  myPTRACE(3, "T38Modem\t" << name << " EnableFakeOut: " << (enable ? "enable" : "disable"));

  isEnableFakeOut = enable;
  OnChangeEnableFakeOut();
}

void EngineBase::OnChangeEnableFakeOut()
{
  if (!isEnableFakeOut && hOwnerOut != NULL && isFakeOwnerOut) {
    myPTRACE(1, "T38Modem\t" << name << " OnChangeEnableFakeOut: close fake " << hOwnerOut);

    hOwnerOut = NULL;
    OnCloseOut();
  }
}

void EngineBase::ChangeModemClass(ModemClass newModemClass)
{
  PWaitAndSignal mutexWait(Mutex);

  if (modemClass == newModemClass)
    return;

  modemClass = newModemClass;

  myPTRACE(1, "T38Modem\t" << name << " ChangeModemClass to " << modemClass);

  OnChangeModemClass();
}

void EngineBase::OnChangeModemClass()
{
  if (recvUserInput) {
    delete recvUserInput;
    recvUserInput = NULL;
  }

  myPTRACE(1, "T38Modem\t" << name << " OnChangeModemClass to " << modemClass);
}

PBoolean EngineBase::TryLockModemCallback()
{
  MutexModem.Wait();

  if (!MutexModemCallback.Wait(0)) {
    MutexModem.Signal();
    return FALSE;
  }

  return TRUE;
}

void EngineBase::UnlockModemCallback()
{
  MutexModemCallback.Signal();
  MutexModem.Signal();
}

void EngineBase::ModemCallbackWithUnlock(INT extra)
{
  Mutex.Signal();
  MutexModemCallback.Wait();

  if (!modemCallback.IsNULL())
    modemCallback(*this, extra);

  MutexModemCallback.Signal();
  Mutex.Wait();
}

void EngineBase::WriteUserInput(const PString & value)
{
  myPTRACE(1, "T38Modem\t" << name << " WriteUserInput " << value);

  PWaitAndSignal mutexWait(Mutex);

  OnUserInput(value);
}

void EngineBase::OnUserInput(const PString & value)
{
  myPTRACE(4, "T38Modem\t" << name << " OnUserInput " << value);

  if (recvUserInput && !recvUserInput->isFull()) {
    recvUserInput->PutData((const char *)value, value.GetLength());

    ModemCallbackWithUnlock(cbpUserInput);
  }
}

int EngineBase::RecvUserInput(void * pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (!recvUserInput)
    return -1;

  return recvUserInput->GetData(pBuf, count);
}
///////////////////////////////////////////////////////////////

