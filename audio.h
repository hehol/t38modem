/*
 * audio.h
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
 * $Log: audio.h,v $
 * Revision 1.5  2010-03-18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.5  2010/03/18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.4  2009/11/20 16:37:27  vfrolov
 * Fixed audio class application blocking by forced T.38 mode
 *
 * Revision 1.3  2009/11/18 19:08:47  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/03/23 09:54:45  vfrolov
 * Initial revision
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

    PBoolean Read(void * buffer, PINDEX amount);
    void SendOnIdle(DataType _dataType);
    PBoolean SendStart(DataType _dataType, int param);
    int Send(const void *pBuf, PINDEX count);
    PBoolean SendStop(PBoolean moreFrames, int _callbackParam);
    PBoolean isOutBufFull() const;

    PBoolean Write(const void * buffer, PINDEX len);
    PBoolean RecvWait(DataType _dataType, int param, int _callbackParam, PBoolean &done);
    PBoolean RecvStart(int _callbackParam);
    int Recv(void *pBuf, PINDEX count);
    void RecvStop();

  protected:

    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnChangeModemClass();
    virtual void OnOpenIn();
    virtual void OnOpenOut();
    virtual void OnCloseIn();
    virtual void OnCloseOut();

    PAdaptiveDelay readDelay;
    PAdaptiveDelay writeDelay;

    PTime  targetTimeFakeOut;
    mutable PTimer timerFakeOut;
    PDECLARE_NOTIFIER(PTimer, AudioEngine, OnTimerFakeOutCallback);
    const PNotifier timerFakeOutCallback;

    int callbackParam;

    DataStream *volatile sendAudio;
    DataStream *volatile recvAudio;

    T30Tone *volatile t30Tone;
    T30ToneDetect *volatile t30ToneDetect;
};
///////////////////////////////////////////////////////////////

#endif  // _PM_AUDIO_H

