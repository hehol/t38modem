/*
 * enginebase.h
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
 * $Log: enginebase.h,v $
 * Revision 1.10  2010-09-22 15:07:45  vfrolov
 * Added ResetModemState() and OnResetModemState()
 *
 * Revision 1.10  2010/09/22 15:07:45  vfrolov
 * Added ResetModemState() and OnResetModemState()
 *
 * Revision 1.9  2010/09/08 17:22:23  vfrolov
 * Redesigned modem engine (continue)
 *
 * Revision 1.8  2010/07/07 08:09:47  vfrolov
 * Added IsAttached()
 *
 * Revision 1.7  2010/03/18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.6  2009/11/19 14:48:28  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.5  2009/11/19 11:14:04  vfrolov
 * Added OnUserInput
 *
 * Revision 1.4  2009/11/18 19:08:47  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.3  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.2  2007/04/09 08:07:12  vfrolov
 * Added symbolic logging ModemCallbackParam
 *
 * Revision 1.1  2007/03/23 09:54:45  vfrolov
 * Initial revision
 *
 */

#ifndef _ENGINEBASE_H
#define _ENGINEBASE_H

///////////////////////////////////////////////////////////////
class DataStream;

class EngineBase : public PObject
{
  PCLASSINFO(EngineBase, PObject);

  public:

  /**@name Construction */
  //@{
    EngineBase(const PString &_name = "");
    virtual ~EngineBase();
  //@}

    enum {
      diagOutOfOrder	= 0x01,
      diagDiffSig	= 0x04,	// a different signal is detected
      diagBadFcs	= 0x08,
      diagNoCarrier	= 0x10,
      diagError		= 0x80,	// bad usage
    };

    enum DataType {
      dtNone,
      dtCed,
      dtCng,
      dtSilence,
      dtHdlc,
      dtRaw,
    };

    enum ModemCallbackParam {
      cbpUserDataMask  = 0xFF,
      cbpOutBufNoFull  = 256,
      cbpUpdateState   = 257,
      cbpReset         = -1,
      cbpOutBufEmpty   = -2,
      cbpUserInput     = -3,
    };

    enum ModemClass {
      mcUndefined,
      mcAudio,
      mcFax,
    };

  /**@name Modem API */
  //@{
    PBoolean IsAttached() const;
    PBoolean Attach(const PNotifier &callback);
    void Detach(const PNotifier &callback);
    void ResetModemState();

    void OpenIn();
    void OpenOut();
    void CloseIn();
    void CloseOut();

    PBoolean IsOpenIn() const { return isOpenIn && IsModemOpen(); }
    PBoolean IsOpenOut() const { return isOpenOut && IsModemOpen(); }

    PBoolean TryLockModemCallback();
    void UnlockModemCallback();

    void ChangeModemClass(ModemClass newModemClass);

    void WriteUserInput(const PString & value);
    int RecvUserInput(void * pBuf, PINDEX count);

    virtual void SendOnIdle(DataType _dataType) = 0;
    virtual PBoolean SendStart(DataType _dataType, int param) = 0;
    virtual int Send(const void *pBuf, PINDEX count) = 0;
    virtual PBoolean SendStop(PBoolean moreFrames, int _callbackParam) = 0;
    virtual PBoolean isOutBufFull() const = 0;

    virtual PBoolean RecvWait(DataType _dataType, int param, int _callbackParam, PBoolean &done) = 0;
    virtual PBoolean RecvStart(int _callbackParam) = 0;
    virtual int Recv(void *pBuf, PINDEX count) = 0;
    virtual void RecvStop() = 0;
    virtual int RecvDiag() const { return 0; };
  //@}

  protected:
    PBoolean IsModemOpen() const { return !modemCallback.IsNULL(); }

    virtual void OnAttach();
    virtual void OnDetach();
    virtual void OnResetModemState();
    virtual void OnChangeModemClass();
    virtual void OnUserInput(const PString & value);

    virtual void OnOpenIn() {}
    virtual void OnOpenOut() {}
    virtual void OnCloseIn() {}
    virtual void OnCloseOut() {}

    const PString name;
    DataStream *volatile recvUserInput;
    ModemClass modemClass;
    volatile PBoolean isOpenIn;
    volatile PBoolean isOpenOut;

    void ModemCallbackWithUnlock(INT extra);

    PNotifier modemCallback;
    PTimedMutex MutexModemCallback;

    PMutex MutexModem;
    PMutex Mutex;
};

#if PTRACING
ostream & operator<<(ostream & out, EngineBase::DataType dataType);
ostream & operator<<(ostream & out, EngineBase::ModemCallbackParam param);
ostream & operator<<(ostream & out, EngineBase::ModemClass modemClass);
#endif
///////////////////////////////////////////////////////////////

#endif  // _ENGINEBASE_H

