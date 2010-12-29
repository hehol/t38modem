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
 * Revision 1.18  2010-12-29 12:35:41  vfrolov
 * Fixed mutex locking
 *
 * Revision 1.18  2010/12/29 12:35:41  vfrolov
 * Fixed mutex locking
 *
 * Revision 1.17  2010/10/12 16:46:25  vfrolov
 * Implemented fake streams
 *
 * Revision 1.16  2010/10/06 16:54:19  vfrolov
 * Redesigned engine opening/closing
 *
 * Revision 1.15  2010/09/22 15:23:48  vfrolov
 * Added OnResetModemState()
 *
 * Revision 1.14  2010/09/10 18:00:44  vfrolov
 * Cleaned up code
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
#include "tone_gen.h"
#include "audio.h"

#define new PNEW

///////////////////////////////////////////////////////////////
typedef	PInt16                    SIMPLE_TYPE;
#define BYTES_PER_SIMPLE          sizeof(SIMPLE_TYPE)
#define SIMPLES_PER_SEC           8000
#define BYTES_PER_MSEC            ((SIMPLES_PER_SEC*BYTES_PER_SIMPLE)/1000)
///////////////////////////////////////////////////////////////
class FakeReadThread : public PThread
{
    PCLASSINFO(FakeReadThread, PThread);
  public:
    FakeReadThread(AudioEngine &engine)
      : PThread(30000)
      , audioEngine(engine)
    {
      PTRACE(3, audioEngine.Name() << " FakeReadThread");
      audioEngine.AddReference();
    }

    ~FakeReadThread()
    {
      PTRACE(3, audioEngine.Name() << " ~FakeReadThread");
      ReferenceObject::DelPointer(&audioEngine);
    }

  protected:
    virtual void Main();

    AudioEngine &audioEngine;
};

void FakeReadThread::Main()
{
  PTRACE(3, audioEngine.Name() << " FakeReadThread::Main started");

  audioEngine.OpenOut(EngineBase::HOWNEROUT(this), TRUE);

  static BYTE buf[BYTES_PER_MSEC*20];

#if PTRACING
  unsigned long count = 0;
#endif

  for (;;) {
    if (!audioEngine.Read(EngineBase::HOWNEROUT(this), buf, sizeof(buf)))
      break;

#if PTRACING
    count++;
#endif
  }

  audioEngine.CloseOut(EngineBase::HOWNEROUT(this));

  PTRACE(3, audioEngine.Name() << " FakeReadThread::Main stopped, faked out " << count*(sizeof(buf)/BYTES_PER_MSEC) << " ms");
}
///////////////////////////////////////////////////////////////
class FakeWriteThread : public PThread
{
    PCLASSINFO(FakeWriteThread, PThread);
  public:
    FakeWriteThread(AudioEngine &engine)
      : PThread(30000)
      , audioEngine(engine)
    {
      PTRACE(3, audioEngine.Name() << " FakeWriteThread");
      audioEngine.AddReference();
    }

    ~FakeWriteThread()
    {
      PTRACE(3, audioEngine.Name() << " ~FakeWriteThread");
      ReferenceObject::DelPointer(&audioEngine);
    }

  protected:
    virtual void Main();

    AudioEngine &audioEngine;
};

void FakeWriteThread::Main()
{
  PTRACE(3, audioEngine.Name() << " FakeWriteThread::Main started");

  audioEngine.OpenIn(EngineBase::HOWNERIN(this), TRUE);

#if PTRACING
  unsigned long count = 0;
#endif

  for (;;) {
    if (!audioEngine.Write(EngineBase::HOWNERIN(this), NULL, BYTES_PER_MSEC*20))
      break;

#if PTRACING
    count++;
#endif
  }

  audioEngine.CloseIn(EngineBase::HOWNERIN(this));

  PTRACE(3, audioEngine.Name() << " FakeWriteThread::Main stopped, faked out " << count*20 << " ms");
}
///////////////////////////////////////////////////////////////
static ToneGenerator::ToneType dt2tt(EngineBase::DataType dataType)
{
  switch (dataType) {
    case EngineBase::dtCng:     return ToneGenerator::ttCng;
    case EngineBase::dtCed:     return ToneGenerator::ttCed;
    case EngineBase::dtRing:    return ToneGenerator::ttRing;
    case EngineBase::dtBusy:    return ToneGenerator::ttBusy;
    default:                    break;
  }

  return ToneGenerator::ttSilence;
}
///////////////////////////////////////////////////////////////
AudioEngine::AudioEngine(const PString &_name)
  : EngineBase(_name + " AudioEngine")
  , callbackParam(cbpReset)
  , sendAudio(NULL)
  , recvAudio(NULL)
  , pToneIn(NULL)
  , pToneOut(NULL)
  , t30ToneDetect(NULL)
{
  PTRACE(2, name << " AudioEngine");
}

AudioEngine::~AudioEngine()
{
  PTRACE(2, name << " ~AudioEngine");

  delete sendAudio;
  delete recvAudio;
  delete pToneIn;
  delete pToneOut;
  delete t30ToneDetect;
}

void AudioEngine::OnAttach()
{
  EngineBase::OnAttach();
}

void AudioEngine::OnDetach()
{
  EngineBase::OnDetach();
}

void AudioEngine::OnResetModemState()
{
  EngineBase::OnResetModemState();

  if (sendAudio) {
    if (hOwnerOut != NULL) {
      sendAudio->PutEof();
    } else {
      delete sendAudio;
      sendAudio = NULL;
    }
  }

  if (recvAudio) {
    delete recvAudio;
    recvAudio = NULL;
  }

  if (pToneIn) {
    delete pToneIn;
    pToneIn = NULL;
  }

  if (pToneOut) {
    delete pToneOut;
    pToneOut = NULL;
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
}

void AudioEngine::OnCloseOut()
{
  EngineBase::OnCloseOut();
}

void AudioEngine::OnChangeEnableFakeOut()
{
  EngineBase::OnChangeEnableFakeOut();

  if (IsOpenOut() || !isEnableFakeOut)
    return;

  if (!sendAudio)
    return;

  (new FakeReadThread(*this))->Resume();
}

PBoolean AudioEngine::Read(HOWNEROUT hOwner, void * buffer, PINDEX amount)
{
  if (hOwnerOut != hOwner || !IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexOutWait(MutexOut);

  if (hOwnerOut != hOwner || !IsModemOpen())
    return FALSE;

  {
    PWaitAndSignal mutexWait(Mutex);

    if (hOwnerOut != hOwner || !IsModemOpen())
      return FALSE;

    if (firstOut) {
      firstOut = FALSE;
      ModemCallbackWithUnlock(cbpUpdateState);

      if (hOwnerOut != hOwner || !IsModemOpen())
        return FALSE;

      readDelay.Restart();
    }
  }

  readDelay.Delay(amount/BYTES_PER_MSEC);

  if (hOwnerOut != hOwner || !IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexWait(Mutex);

  if (hOwnerOut != hOwner || !IsModemOpen())
    return FALSE;

  if (sendAudio) {
    PBoolean wasFull = sendAudio->isFull();

    int count = sendAudio->GetData(buffer, amount);

    if (count < 0) {
      count = 0;
      delete sendAudio;
      sendAudio = NULL;
      ModemCallbackWithUnlock(callbackParam);

      if (hOwnerOut != hOwner || !IsModemOpen())
        return FALSE;
    } else {
      if (wasFull && !sendAudio->isFull()) {
        ModemCallbackWithUnlock(cbpOutBufNoFull);

        if (hOwnerOut != hOwner || !IsModemOpen())
          return FALSE;
      }
    }

    if (amount > count)
      memset((BYTE *)buffer + count, 0, amount - count);
  } else {
    if (pToneOut)
      pToneOut->Read(buffer, amount);
    else
      memset(buffer, 0, amount);
  }

  return TRUE;
}

void AudioEngine::SendOnIdle(DataType _dataType)
{
  PTRACE(2, name << " SendOnIdle " << _dataType);

  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  ToneGenerator::ToneType toneType = dt2tt(_dataType);

  if (pToneOut && pToneOut->Type() != toneType) {
    delete pToneOut;
    pToneOut = NULL;
  }

  if (pToneOut == NULL && toneType != ToneGenerator::ttSilence)
    pToneOut = new ToneGenerator(toneType);
}

PBoolean AudioEngine::SendStart(DataType PTRACE_PARAM(_dataType), int PTRACE_PARAM(param))
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexWait(Mutex);

  if (sendAudio)
    delete sendAudio;

  sendAudio = new DataStream(1024 * BYTES_PER_SIMPLE);

  PTRACE(3, name << " SendStart _dataType=" << _dataType
                 << " param=" << param);

  return TRUE;
}

int AudioEngine::Send(const void *pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return -1;

  PWaitAndSignal mutexWait(Mutex);

  if (sendAudio)
    sendAudio->PutData(pBuf, count);

  return count;
}

PBoolean AudioEngine::SendStop(PBoolean PTRACE_PARAM(moreFrames), int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexWait(Mutex);

  callbackParam = _callbackParam;

  if (sendAudio)
    sendAudio->PutEof();

  PTRACE(3, name << " SendStop moreFrames=" << moreFrames
                 << " callbackParam=" << callbackParam);

  return TRUE;
}

PBoolean AudioEngine::isOutBufFull() const
{
  PWaitAndSignal mutexWait(Mutex);

  if (!sendAudio)
    return FALSE;

  return sendAudio->isFull();
}
///////////////////////////////////////////////////////////////
void AudioEngine::OnOpenIn()
{
  EngineBase::OnOpenIn();
}

void AudioEngine::OnCloseIn()
{
  EngineBase::OnCloseIn();
}

void AudioEngine::OnChangeEnableFakeIn()
{
  EngineBase::OnChangeEnableFakeIn();

  if (IsOpenIn() || !isEnableFakeIn)
    return;

  if (!recvAudio)
    return;

  (new FakeWriteThread(*this))->Resume();
}

PBoolean AudioEngine::Write(HOWNERIN hOwner, const void * buffer, PINDEX len)
{
  if (hOwnerIn != hOwner || !IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexInWait(MutexIn);

  if (hOwnerIn != hOwner || !IsModemOpen())
    return FALSE;

  {
    PWaitAndSignal mutexWait(Mutex);

    if (hOwnerIn != hOwner || !IsModemOpen())
      return FALSE;

    if (firstIn) {
      firstIn = FALSE;
      ModemCallbackWithUnlock(cbpUpdateState);

      if (hOwnerIn != hOwner || !IsModemOpen())
        return FALSE;

      writeDelay.Restart();
    }

    if (buffer) {
      if (recvAudio && !recvAudio->isFull()) {
        recvAudio->PutData(buffer, len);
        ModemCallbackWithUnlock(callbackParam);

        if (hOwnerIn != hOwner || !IsModemOpen())
          return FALSE;
      }

      if (t30ToneDetect && t30ToneDetect->Write(buffer, len)) {
        OnUserInput('c');

        if (hOwnerIn != hOwner || !IsModemOpen())
          return FALSE;
      }
    } else {
      if (recvAudio && !recvAudio->isFull()) {
        for (PINDEX rest = len ; rest > 0 ;) {
          BYTE buf[64*BYTES_PER_SIMPLE];
          PINDEX lenChank = rest;

          if (lenChank > (PINDEX)sizeof(buf))
            lenChank = (PINDEX)sizeof(buf);

          if (pToneIn)
            pToneIn->Read(buf, lenChank);
          else
            memset(buf, 0, lenChank);

          recvAudio->PutData(buf, lenChank);
          rest -= lenChank;
        }

        ModemCallbackWithUnlock(callbackParam);

        if (hOwnerIn != hOwner || !IsModemOpen())
          return FALSE;
      }
    }
  }

  writeDelay.Delay(len/BYTES_PER_MSEC);

  if (hOwnerIn != hOwner || !IsModemOpen())
    return FALSE;

  return TRUE;
}

void AudioEngine::RecvOnIdle(DataType _dataType)
{
  PTRACE(2, name << " RecvOnIdle " << _dataType);

  PWaitAndSignal mutexWaitModem(MutexModem);
  PWaitAndSignal mutexWait(Mutex);

  ToneGenerator::ToneType toneType = dt2tt(_dataType);

  if (pToneIn && pToneIn->Type() != toneType) {
    delete pToneIn;
    pToneIn = NULL;
  }

  if (pToneIn == NULL && toneType != ToneGenerator::ttSilence)
    pToneIn = new ToneGenerator(toneType);
}

PBoolean AudioEngine::RecvWait(DataType /*_dataType*/, int /*param*/, int /*_callbackParam*/, PBoolean &done)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexWait(Mutex);

  if (recvAudio)
    delete recvAudio;

  recvAudio = new DataStream(1024 * BYTES_PER_SIMPLE);

  done = TRUE;

  return TRUE;
}

PBoolean AudioEngine::RecvStart(int _callbackParam)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!IsModemOpen())
    return FALSE;

  PWaitAndSignal mutexWait(Mutex);

  if (!recvAudio)
    return FALSE;

  callbackParam = _callbackParam;

  return TRUE;
}

int AudioEngine::Recv(void * pBuf, PINDEX count)
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if (!recvAudio)
    return -1;

  PWaitAndSignal mutexWait(Mutex);

  return recvAudio->GetData(pBuf, count);
}

void AudioEngine::RecvStop()
{
  PWaitAndSignal mutexWaitModem(MutexModem);

  if(!IsModemOpen())
    return;

  PWaitAndSignal mutexWait(Mutex);

  if (recvAudio) {
    delete recvAudio;
    recvAudio = NULL;
  }
}
///////////////////////////////////////////////////////////////

