/*
 * enginebase.cxx
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
 * $Log: enginebase.cxx,v $
 * Revision 1.5  2009-11-19 11:14:04  vfrolov
 * Added OnUserInput
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
ostream & operator<<(ostream & out, EngineBase::ModemCallbackParam param)
{
  switch (param) {
    case EngineBase::cbpUserDataMask:	break;
    case EngineBase::cbpOutBufNoFull:	return out << "cbpOutBufNoFull";
    case EngineBase::cbpReset:		return out << "cbpReset";
    case EngineBase::cbpOutBufEmpty:	return out << "cbpOutBufEmpty";
    case EngineBase::cbpUserInput:	return out << "cbpUserInput";
  }

  return out << INT(param);
}

ostream & operator<<(ostream & out, EngineBase::ModemClass modemClass)
{
  switch (modemClass) {
    case EngineBase::mcUndefined:       return out << "mcUndefined";
    case EngineBase::mcAudio:           return out << "mcAudio";
    case EngineBase::mcFax:             return out << "mcFax";
  }

  return out << INT(modemClass);
}
#endif
///////////////////////////////////////////////////////////////
EngineBase::EngineBase(const PString &_name)
  : name(_name)
  , recvUserInput(NULL)
  , modemClass(mcUndefined)
{
}

EngineBase::~EngineBase()
{
  if (recvUserInput)
    delete recvUserInput;
}

PBoolean EngineBase::Attach(const PNotifier &callback)
{
  PTRACE(1, name << " Attach");

  PWaitAndSignal mutexWait(Mutex);

  if (!modemCallback.IsNULL()) {
    myPTRACE(1, name << " Attach !modemCallback.IsNULL()");

    return FALSE;
  }

  modemCallback = callback;

  OnAttach();

  return TRUE;
}

void EngineBase::OnAttach()
{
  PTRACE(1, name << " OnAttach Attached");
}

void EngineBase::Detach(const PNotifier &callback)
{
  PTRACE(1, name << " Detach");

  PWaitAndSignal mutexWait(Mutex);

  if (modemCallback.IsNULL()) {
    myPTRACE(1, name << " Detach Already Detached");

    return;
  }

  if (modemCallback != callback) {
    myPTRACE(1, name << " Detach modemCallback != callback");

    return;
  }

  modemCallback = NULL;
  modemClass = mcUndefined;

  OnChangeModemClass();
  OnDetach();
}

void EngineBase::OnDetach()
{
  myPTRACE(1, name << " OnDetach Detached");
}

void EngineBase::ChangeModemClass(ModemClass newModemClass)
{
  PWaitAndSignal mutexWait(Mutex);

  if (modemClass == newModemClass)
    return;

  modemClass = newModemClass;

  myPTRACE(1, name << " ChangeModemClass to " << modemClass);

  OnChangeModemClass();
}

void EngineBase::OnChangeModemClass()
{
  if (modemClass == mcAudio) {
    if (!recvUserInput)
      recvUserInput = new DataStream(64);
  } else {
    if (recvUserInput) {
      delete recvUserInput;
      recvUserInput = NULL;
    }
  }

  myPTRACE(1, name << " OnChangeModemClass to " << modemClass);
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
  myPTRACE(1, name << " WriteUserInput " << value);

  PWaitAndSignal mutexWait(Mutex);

  OnUserInput(value);
}

void EngineBase::OnUserInput(const PString & value)
{
  PTRACE(4, name << " OnUserInput " << value);

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

