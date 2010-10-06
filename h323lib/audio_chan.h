/*
 * audio_chan.h
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
 * $Log: audio_chan.h,v $
 * Revision 1.1  2010-10-06 16:34:10  vfrolov
 * Initial revision
 *
 * Revision 1.1  2010/10/06 16:34:10  vfrolov
 * Initial revision
 *
 */

#ifndef _AUDIO_CHAN_H
#define _AUDIO_CHAN_H

///////////////////////////////////////////////////////////////
class AudioEngine;
class PseudoModem;
///////////////////////////////////////////////////////////////
class ModemAudioChannel : public PChannel
{
  PCLASSINFO(ModemAudioChannel, PChannel);

  public:

  /**@name Construction */
  //@{
    ModemAudioChannel(PBoolean out, PseudoModem * pmodem);
    ~ModemAudioChannel();
  //@}

  /**@name Operations */
  //@{
    PBoolean IsOK() const { return audioEngine != NULL; }
    PBoolean Read(void * buffer, PINDEX amount);
    PBoolean Write(const void * buffer, PINDEX len);
  //@}

  private:
    AudioEngine * audioEngine;
    PBoolean outDirection;
};
///////////////////////////////////////////////////////////////

#endif  // _AUDIO_CHAN_H

