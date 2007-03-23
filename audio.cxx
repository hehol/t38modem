/*
 * audio.cxx
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
 * $Log: audio.cxx,v $
 * Revision 1.1  2007-03-23 09:54:45  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/03/23 09:54:45  vfrolov
 * Initial revision
 *
 *
 */

#include <ptlib.h>
#include "pmutils.h"
#include "t30tone.h"
#include "audio.h"

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

BOOL AudioEngine::Attach(const PNotifier &callback)
{
  PTRACE(1, name << " AudioEngine::Attach");

  if (!modemCallback.IsNULL()) {
    myPTRACE(1, name << " AudioEngine::Attach !modemCallback.IsNULL()");
    return FALSE;
  }

  modemCallback = callback;

  PWaitAndSignal mutexWait(Mutex);

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

void AudioEngine::AudioClass(BOOL _audioClass)
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
BOOL AudioEngine::Read(void * buffer, PINDEX amount)
{
  Mutex.Wait();

  if (sendAudio) {
    BOOL wasFull = sendAudio->isFull();

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
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  t30Tone = new T30Tone(T30Tone::cng);
}

BOOL AudioEngine::SendStart(int /*_dataType*/, int /*param*/)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  sendAudio = new DataStream(1024*2);

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

BOOL AudioEngine::SendStop(BOOL /*moreFrames*/, int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (sendAudio)
    sendAudio->PutEof();

  callbackParam = _callbackParam;

  return TRUE;
}

BOOL AudioEngine::isOutBufFull() const
{
  PWaitAndSignal mutexWait(Mutex);

  return sendAudio && sendAudio->isFull();
}
///////////////////////////////////////////////////////////////
BOOL AudioEngine::Write(const void * buffer, PINDEX len)
{
  Mutex.Wait();

  if (recvAudio && !recvAudio->isFull()) {
    recvAudio->PutData(buffer, len);
    ModemCallbackWithUnlock(callbackParam);
  }

  BOOL cng = t30ToneDetect && t30ToneDetect->Write(buffer, len);

  Mutex.Signal();

  if (cng)
    WriteUserInput('c');

  writeDelay.Delay(len/16);

  return TRUE;
}

BOOL AudioEngine::RecvWait(int /*_dataType*/, int /*param*/, int /*_callbackParam*/, BOOL &done)
{
  done = TRUE;
  return TRUE;
}

BOOL AudioEngine::RecvStart(int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

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

