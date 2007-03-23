/*
 * audio.h
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
 * $Log: audio.h,v $
 * Revision 1.1  2007-03-23 09:54:45  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/03/23 09:54:45  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _PM_AUDIO_H
#define _PM_AUDIO_H

#include <ptclib/delaychan.h>
#include "enginebase.h"

///////////////////////////////////////////////////////////////
class DataStream;
class T30Tone;
class T30ToneDetect;
///////////////////////////////////////////////////////////////
class AudioEngine : public PChannel, public EngineBase
{
  PCLASSINFO(AudioEngine, PChannel);

  public:

    AudioEngine(const PString &_name = "");
    ~AudioEngine();

    BOOL Attach(const PNotifier &callback);
    void Detach(const PNotifier &callback);
    void ModemCallbackWithUnlock(INT extra);
    void AudioClass(BOOL _audioClass);

    BOOL Read(void * buffer, PINDEX amount);
    void SendOnIdle(int _dataType);
    BOOL SendStart(int _dataType, int param);
    int Send(const void *pBuf, PINDEX count);
    BOOL SendStop(BOOL moreFrames, int _callbackParam);
    BOOL isOutBufFull() const;

    BOOL Write(const void * buffer, PINDEX len);
    BOOL RecvWait(int _dataType, int param, int _callbackParam, BOOL &done);
    BOOL RecvStart(int _callbackParam);
    int Recv(void *pBuf, PINDEX count);
    void RecvStop();
    void WriteUserInput(const PString & value);
    int RecvUserInput(void * pBuf, PINDEX count);

  protected:

    PAdaptiveDelay readDelay;
    PAdaptiveDelay writeDelay;

    int callbackParam;

    DataStream *volatile sendAudio;
    DataStream *volatile recvAudio;

    DataStream *volatile recvUserInput;

    T30Tone *volatile t30Tone;
    T30ToneDetect *volatile t30ToneDetect;

    BOOL audioClass;

    PMutex Mutex;
};
///////////////////////////////////////////////////////////////

#endif  // _PM_AUDIO_H

