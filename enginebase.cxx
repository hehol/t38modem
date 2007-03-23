/*
 * enginebase.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007 Vyacheslav Frolov
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
 * Revision 1.1  2007-03-23 09:54:45  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/03/23 09:54:45  vfrolov
 * Initial revision
 *
 *
 */

#include <ptlib.h>
#include "enginebase.h"

///////////////////////////////////////////////////////////////
BOOL EngineBase::TryLockModemCallback()
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

void EngineBase::ModemCallback(INT extra)
{
  MutexModemCallback.Wait();

  if (!modemCallback.IsNULL())
    modemCallback(*this, extra);

  MutexModemCallback.Signal();
}
///////////////////////////////////////////////////////////////

