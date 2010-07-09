/*
 * audio.cxx
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
 * $Log: audio.cxx,v $
 * Revision 1.13  2010-07-09 04:44:27  vfrolov
 * Added tracing targetTimeFakeOut
 *
 * Revision 1.13  2010/07/09 04:44:27  vfrolov
 * Added tracing targetTimeFakeOut
 *
 * Revision 1.12  2010/03/18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.11  2009/11/20 16:37:27  vfrolov
 * Fixed audio class application blocking by forced T.38 mode
 *
 * Revision 1.10  2009/11/19 11:14:04  vfrolov
 * Added OnUserInput
 *
 * Revision 1.9  2009/11/18 19:08:47  vfrolov
 * Moved common code to class EngineBase
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
  : EngineBase(_name + " AudioEngine")
#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif
  , timerFakeOutCallback(PCREATE_NOTIFIER(OnTimerFakeOutCallback))
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
  , callbackParam(cbpReset)
  , sendAudio(NULL)
  , recvAudio(NULL)
  , t30Tone(NULL)
  , t30ToneDetect(NULL)
{
  timerFakeOut.SetNotifier(timerFakeOutCallback);
}

AudioEngine::~AudioEngine()
{
  timerFakeOut = 0;

  if (sendAudio)
    delete sendAudio;

  if (recvAudio)
    delete recvAudio;

  if (t30Tone)
    delete t30Tone;

  if (t30ToneDetect)
    delete t30ToneDetect;
}

void AudioEngine::OnAttach()
{
  EngineBase::OnAttach();

  readDelay.Restart();
  writeDelay.Restart();
}

void AudioEngine::OnDetach()
{
  EngineBase::OnDetach();

  if (sendAudio) {
    delete sendAudio;
    sendAudio = NULL;
  }

  if (recvAudio) {
    delete recvAudio;
    recvAudio = NULL;
  }

  if (t30Tone) {
    delete t30Tone;
    t30Tone = NULL;
  }
}

void AudioEngine::OnChangeModemClass()
{
  EngineBase::OnChangeModemClass();

  if (modemClass == mcAudio) {
    if (!t30ToneDetect)
      t30ToneDetect = new T30ToneDetect;
  } else {
    if (t30ToneDetect) {
      delete t30ToneDetect;
      t30ToneDetect = NULL;
    }
  }
}
///////////////////////////////////////////////////////////////
void AudioEngine::OnOpenOut()
{
  EngineBase::OnOpenOut();
  readDelay.Restart();
  timerFakeOut = 0;

  if (sendAudio && !sendAudio->isFull())
    ModemCallbackWithUnlock(cbpOutBufNoFull);
}

void AudioEngine::OnCloseOut()
{
  EngineBase::OnCloseOut();

  if (sendAudio) {
    int countTotal = 0;

    for (;;) {
      static BYTE buffer[64];

      PBoolean wasFull = sendAudio->isFull();

      int count = sendAudio->GetData(buffer, sizeof(buffer));

      if (count < 0) {
        delete sendAudio;
        sendAudio = NULL;
        ModemCallbackWithUnlock(callbackParam);
        break;
      }
      else {
        if (wasFull && !sendAudio->isFull())
          ModemCallbackWithUnlock(cbpOutBufNoFull);

        if (count == 0)
          break;

        countTotal += count;
      }
    }

    if (sendAudio)
      targetTimeFakeOut = PTime() + countTotal/16;
  }
}

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

void AudioEngine::SendOnIdle(DataType _dataType)
{
  PTRACE(2, name << " SendOnIdle " << _dataType);

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
      PTRACE(1, name << " SendOnIdle dataType(" << _dataType << ") is not supported");
  }
}

PBoolean AudioEngine::SendStart(DataType PTRACE_PARAM(_dataType), int PTRACE_PARAM(param))
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (sendAudio)
    delete sendAudio;

  sendAudio = new DataStream(1024*2);

  PTRACE(3, name << " SendStart _dataType=" << _dataType
                 << " param=" << param);

  if (!isOpenOut)
    targetTimeFakeOut = PTime();

  return TRUE;
}

int AudioEngine::Send(const void *pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  if (sendAudio) {
    if (isOpenOut)
      sendAudio->PutData(pBuf, count);
    else
      targetTimeFakeOut += count/16;
  }

  return count;
}

PBoolean AudioEngine::SendStop(PBoolean PTRACE_PARAM(moreFrames), int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  callbackParam = _callbackParam;

  if (sendAudio) {
    if (isOpenOut) {
      sendAudio->PutEof();
    } else {
      delete sendAudio;
      sendAudio = NULL;

      PTimeInterval delay = targetTimeFakeOut - PTime();
      int sleep_time = (int)delay.GetMilliSeconds();

      timerFakeOut = (sleep_time > 0 ? sleep_time : 1);
    }
  }

  PTRACE(3, name << " SendStop moreFrames=" << moreFrames
                 << " callbackParam=" << callbackParam);

  return TRUE;
}

PBoolean AudioEngine::isOutBufFull() const
{
  PWaitAndSignal mutexWait(Mutex);

  if (!sendAudio)
    return FALSE;

  if (!isOpenOut) {
    PTimeInterval delay = targetTimeFakeOut - PTime();
    int sleep_time = (int)delay.GetMilliSeconds();

    if (sleep_time <= 0)
      return FALSE;

    PTRACE(5, "AudioEngine::isOutBufFull targetTimeFakeOut=" << targetTimeFakeOut.AsString("yyyy/MM/dd hh:mm:ss.uuu", PTime::Local));

    timerFakeOut = sleep_time;

    return TRUE;
  }

  return sendAudio->isFull();
}

void AudioEngine::OnTimerFakeOutCallback(PTimer & PTRACE_PARAM(from), INT PTRACE_PARAM(extra))
{
  PTRACE(4, "AudioEngine::OnTimerFakeOutCallback"
            " " << ModemCallbackParam(sendAudio ? cbpOutBufNoFull : callbackParam) <<
            " " <<from.GetClass() << " " << extra);

  PWaitAndSignal mutexWait(Mutex);

  ModemCallbackWithUnlock(sendAudio ? cbpOutBufNoFull : callbackParam);
}
///////////////////////////////////////////////////////////////
void AudioEngine::OnOpenIn()
{
  EngineBase::OnOpenIn();
  writeDelay.Restart();
}

void AudioEngine::OnCloseIn()
{
  EngineBase::OnCloseIn();
}

PBoolean AudioEngine::Write(const void * buffer, PINDEX len)
{
  Mutex.Wait();

  if (recvAudio && !recvAudio->isFull()) {
    recvAudio->PutData(buffer, len);
    ModemCallbackWithUnlock(callbackParam);
  }

  PBoolean cng = t30ToneDetect && t30ToneDetect->Write(buffer, len);

  if (cng)
    OnUserInput('c');

  Mutex.Signal();

  lastWriteCount = len;

  writeDelay.Delay(len/16);

  return TRUE;
}

PBoolean AudioEngine::RecvWait(DataType /*_dataType*/, int /*param*/, int /*_callbackParam*/, PBoolean &done)
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
///////////////////////////////////////////////////////////////

