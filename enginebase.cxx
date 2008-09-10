/*
 * enginebase.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2008 Vyacheslav Frolov
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
 * Revision 1.3  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
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
#include "enginebase.h"

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
#endif
///////////////////////////////////////////////////////////////
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

void EngineBase::ModemCallback(INT extra)
{
  MutexModemCallback.Wait();

  if (!modemCallback.IsNULL())
    modemCallback(*this, extra);

  MutexModemCallback.Signal();
}
///////////////////////////////////////////////////////////////

