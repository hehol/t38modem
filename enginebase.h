/*
 * enginebase.h
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
 * $Log: enginebase.h,v $
 * Revision 1.2  2007-04-09 08:07:12  vfrolov
 * Added symbolic logging ModemCallbackParam
 *
 * Revision 1.2  2007/04/09 08:07:12  vfrolov
 * Added symbolic logging ModemCallbackParam
 *
 * Revision 1.1  2007/03/23 09:54:45  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _ENGINEBASE_H
#define _ENGINEBASE_H

///////////////////////////////////////////////////////////////

class EngineBase : public PObject
{
  PCLASSINFO(EngineBase, PObject);

  public:

    EngineBase(const PString &_name = "") : name(_name) {}

    enum {
      diagOutOfOrder	= 0x01,
      diagDiffSig	= 0x04,	// a different signal is detected
      diagBadFcs	= 0x08,
      diagNoCarrier	= 0x10,
      diagError		= 0x80,	// bad usage
    };

    enum {
      dtNone,
      dtCed,
      dtCng,
      dtSilence,
      dtHdlc,
      dtRaw,
    };

    enum ModemCallbackParam {
      cbpUserDataMask	= 0xFF,
      cbpOutBufNoFull	= 256,
      cbpReset		= -1,
      cbpOutBufEmpty	= -2,
      cbpUserInput	= -3,
    };

#if PTRACING
    friend ostream & operator<<(ostream & o, ModemCallbackParam param);
#endif

  /**@name Modem API */
  //@{
    BOOL TryLockModemCallback();
    void UnlockModemCallback();
  //@}

  protected:

    void ModemCallback(INT extra);

    PNotifier modemCallback;
    PTimedMutex MutexModemCallback;

    PMutex MutexModem;

    const PString name;
};
///////////////////////////////////////////////////////////////

#endif  // _ENGINEBASE_H

