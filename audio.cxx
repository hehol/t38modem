/*
 * audio.cxx
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
 * $Log: audio.cxx,v $
 * Revision 1.8  2009-10-05 15:01:08  vfrolov
 * Fixed possible memory leak
 *
 * Revision 1.8  2009/10/05 15:01:08  vfrolov
 * Fixed possible memory leak
 *
 * Revision 1.7  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.6  2007/08/27 10:52:13  vfrolov
 * Fixed typo
 *
 * Revision 1.5  2007/05/28 12:44:24  vfrolov
 * Added adaptive delay restarting
 * Fixed SendOnIdle()
 *
 * Revision 1.4  2007/04/24 16:28:53  vfrolov
 * Added more tracing
 *
 * Revision 1.3  2007/04/04 09:52:57  vfrolov
 * Added missing lastWriteCount setting
 *
 * Revision 1.2  2007/03/23 14:54:19  vfrolov
 * Fixed compiler warnings
 *
 * Revision 1.1  2007/03/23 09:54:45  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>
#include "pmutils.h"
#include "t30tone.h"
#include "audio.h"

#define new PNEW

///////////////////////////////////////////////////////////////
typedef	PInt16			SIMPLE_TYPE;
#define	BYTES_PER_SIMPLE	sizeof(SIMPLE_TYPE)
///////////////////////////////////////////////////////////////
AudioEngine::AudioEngine(const PString &_name)
  : EngineBase(_name),
    sendAudio(NULL),
    recvAudio(NULL),
    recvUserInput(NULL),
    t30Tone(NULL),
    t30ToneDetect(NULL),
    audioClass(FALSE)
{
}

AudioEngine::~AudioEngine()
{
  if (sendAudio)
    delete sendAudio;

  if (recvAudio)
    delete recvAudio;

  if (recvUserInput)
    delete recvUserInput;

  if (t30Tone)
    delete t30Tone;

  if (t30ToneDetect)
    delete t30ToneDetect;
}

PBoolean AudioEngine::Attach(const PNotifier &callback)
{
  PTRACE(1, name << " AudioEngine::Attach");

  if (!modemCallback.IsNULL()) {
    myPTRACE(1, name << " AudioEngine::Attach !modemCallback.IsNULL()");
    return FALSE;
  }

  readDelay.Restart();
  writeDelay.Restart();
  modemCallback = callback;

  return TRUE;
}

void AudioEngine::Detach(const PNotifier &callback)
{
  PTRACE(1, name << " AudioEngine::Detach");

  if (modemCallback == callback) {
    modemCallback = NULL;

    PWaitAndSignal mutexWait(Mutex);

    audioClass = FALSE;

    if (sendAudio) {
      delete sendAudio;
      sendAudio = NULL;
    }

    if (recvAudio) {
      delete recvAudio;
      recvAudio = NULL;
    }

    if (recvUserInput) {
      delete recvUserInput;
      recvUserInput = NULL;
    }

    if (t30Tone) {
      delete t30Tone;
      t30Tone = NULL;
    }

    if (t30ToneDetect) {
      delete t30ToneDetect;
      t30ToneDetect = NULL;
    }

    myPTRACE(1, name << " AudioEngine::Detach Detached");
  } else {
    myPTRACE(1, name << " AudioEngine::Detach "
      << (modemCallback.IsNULL() ? "Already Detached" : "modemCallback != callback"));
  }
}

void AudioEngine::ModemCallbackWithUnlock(INT extra)
{
  Mutex.Signal();
  ModemCallback(extra);
  Mutex.Wait();
}

void AudioEngine::AudioClass(PBoolean _audioClass)
{
  PWaitAndSignal mutexWait(Mutex);

  audioClass = _audioClass;

  myPTRACE(1, name << " AudioClass=" << (audioClass ? "TRUE" : "FALSE"));

  if (audioClass) {
    if (!recvUserInput)
      recvUserInput = new DataStream(64);

    if (!t30ToneDetect)
      t30ToneDetect = new T30ToneDetect;
  } else {
    if (recvUserInput) {
      delete recvUserInput;
      recvUserInput = NULL;
    }

    if (t30ToneDetect) {
      delete t30ToneDetect;
      t30ToneDetect = NULL;
    }
  }
}
///////////////////////////////////////////////////////////////
PBoolean AudioEngine::Read(void * buffer, PINDEX amount)
{
  Mutex.Wait();

  if (sendAudio) {
    PBoolean wasFull = sendAudio->isFull();

    int count = sendAudio->GetData(buffer, amount);

    if (count < 0) {
      delete sendAudio;
      sendAudio = NULL;
      ModemCallbackWithUnlock(callbackParam);
      count = 0;
    } else {
      if (wasFull && !sendAudio->isFull())
        ModemCallbackWithUnlock(cbpOutBufNoFull);
    }

    if (amount > count)
      memset((BYTE *)buffer + count, 0, amount - count);
  } else {
    if (t30Tone)
      t30Tone->Read(buffer, amount);
    else
      memset(buffer, 0, amount);
  }

  Mutex.Signal();

  lastReadCount = amount;

  readDelay.Delay(amount/16);

  return TRUE;
}

void AudioEngine::SendOnIdle(int _dataType)
{
  PTRACE(2, name << " AudioEngine::SendOnIdle " << _dataType);

  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (t30Tone) {
    delete t30Tone;
    t30Tone = NULL;
  }

  switch (_dataType) {
    case dtCng:
      t30Tone = new T30Tone(T30Tone::cng);
      break;
    case dtNone:
      break;
    default:
      PTRACE(1, name << " AudioEngine::SendOnIdle dataType(" << _dataType << ") is not supported");
  }
}

PBoolean AudioEngine::SendStart(int PTRACE_PARAM(_dataType), int PTRACE_PARAM(param))
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (sendAudio)
    delete sendAudio;

  sendAudio = new DataStream(1024*2);

  PTRACE(3, name << " AudioEngine::SendStart _dataType=" << _dataType
                 << " param=" << param);

  return TRUE;
}

int AudioEngine::Send(const void *pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (sendAudio)
    sendAudio->PutData(pBuf, count);

  return count;
}

PBoolean AudioEngine::SendStop(PBoolean PTRACE_PARAM(moreFrames), int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (sendAudio)
    sendAudio->PutEof();

  callbackParam = _callbackParam;

  PTRACE(3, name << " AudioEngine::SendStop moreFrames=" << moreFrames
                 << " callbackParam=" << callbackParam);

  return TRUE;
}

PBoolean AudioEngine::isOutBufFull() const
{
  PWaitAndSignal mutexWait(Mutex);

  return sendAudio && sendAudio->isFull();
}
///////////////////////////////////////////////////////////////
PBoolean AudioEngine::Write(const void * buffer, PINDEX len)
{
  Mutex.Wait();

  if (recvAudio && !recvAudio->isFull()) {
    recvAudio->PutData(buffer, len);
    ModemCallbackWithUnlock(callbackParam);
  }

  PBoolean cng = t30ToneDetect && t30ToneDetect->Write(buffer, len);

  Mutex.Signal();

  if (cng)
    WriteUserInput('c');

  lastWriteCount = len;

  writeDelay.Delay(len/16);

  return TRUE;
}

PBoolean AudioEngine::RecvWait(int /*_dataType*/, int /*param*/, int /*_callbackParam*/, PBoolean &done)
{
  done = TRUE;
  return TRUE;
}

PBoolean AudioEngine::RecvStart(int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (recvAudio)
    delete recvAudio;

  recvAudio = new DataStream(1024*2);

  callbackParam = _callbackParam;

  return TRUE;
}

int AudioEngine::Recv(void * pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (!recvAudio)
    return -1;

  return recvAudio->GetData(pBuf, count);
}

void AudioEngine::RecvStop()
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (recvAudio) {
    delete recvAudio;
    recvAudio = NULL;
  }
}

void AudioEngine::WriteUserInput(const PString & value)
{
  myPTRACE(1, name << " AudioEngine::WriteUserInput " << value);

  PWaitAndSignal mutexWait(Mutex);

  if (recvUserInput && !recvUserInput->isFull()) {
    recvUserInput->PutData((const char *)value, value.GetLength());

    ModemCallbackWithUnlock(cbpUserInput);
  }
}

int AudioEngine::RecvUserInput(void * pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (!recvUserInput)
    return -1;

  return recvUserInput->GetData(pBuf, count);
}
///////////////////////////////////////////////////////////////

