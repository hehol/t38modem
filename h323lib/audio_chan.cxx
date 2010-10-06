/*
 * audio_chan.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2010 Vyacheslav Frolov
 *
 * t38modem Project
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
 * The Original Code is t38modem.
 *
 * The Initial Developer of the Original Code is Vyacheslav Frolov
 *
 * Contributor(s):
 *
 * $Log: audio_chan.cxx,v $
 * Revision 1.1  2010-10-06 16:34:10  vfrolov
 * Initial revision
 *
 * Revision 1.1  2010/10/06 16:34:10  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>

#include "audio_chan.h"
#include "../audio.h"
#include "../pmodem.h"

#define new PNEW

///////////////////////////////////////////////////////////////
ModemAudioChannel::ModemAudioChannel(PBoolean out, PseudoModem * pmodem)
  : audioEngine(pmodem->NewPtrAudioEngine())
  , outDirection(out)
{
  if (audioEngine != NULL) {
    if (outDirection)
      audioEngine->OpenOut(EngineBase::HOWNEROUT(this));
    else
      audioEngine->OpenIn(EngineBase::HOWNERIN(this));
  }
}

ModemAudioChannel::~ModemAudioChannel()
{
  if (audioEngine != NULL) {
    if (outDirection)
      audioEngine->CloseOut(EngineBase::HOWNEROUT(this));
    else
      audioEngine->CloseIn(EngineBase::HOWNERIN(this));

    ReferenceObject::DelPointer(audioEngine);
  }
}

PBoolean ModemAudioChannel::Read(void * buffer, PINDEX amount)
{
  if (!audioEngine->Read(EngineBase::HOWNEROUT(this), buffer, amount))
    return FALSE;

  lastReadCount = amount;

  return TRUE;
}

PBoolean ModemAudioChannel::Write(const void * buffer, PINDEX len)
{
  if (!audioEngine->Write(EngineBase::HOWNERIN(this), buffer, len))
    return FALSE;

  lastWriteCount = len;

  return TRUE;
}
///////////////////////////////////////////////////////////////

