/*
 * tone_gen.h
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
 * $Log: tone_gen.h,v $
 * Revision 1.1  2010-10-12 16:43:00  vfrolov
 * Initial revision
 *
 * Revision 1.1  2010/10/12 16:43:00  vfrolov
 * Initial revision
 *
 */

#ifndef _TONE_GEN_H
#define _TONE_GEN_H

///////////////////////////////////////////////////////////////
class ToneGenerator : public PObject
{
  PCLASSINFO(ToneGenerator, PObject);

  public:

    enum ToneType {
      ttSilence,
      ttCng,
      ttCed,
      ttRing,
      ttBusy,
    };

    ToneGenerator(ToneType tt = ttSilence);
    void Read(void * buffer, PINDEX amount);
    ToneType Type() const { return type; }

  protected:
    ToneType type;
    PINDEX bytesOn;
    PINDEX bytesOff;
    const BYTE *pToneOn;
    PINDEX bytesToneOn;
    PINDEX index;
};
///////////////////////////////////////////////////////////////

#endif  // _TONE_GEN_H

