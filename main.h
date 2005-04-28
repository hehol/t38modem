/*
 * main.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2005 Vyacheslav Frolov
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
 * Contributor(s): Equivalence Pty ltd
 *
 * $Log: main.h,v $
 * Revision 1.16  2005-04-28 09:12:06  vfrolov
 * Made tidy up
 *
 * Revision 1.16  2005/04/28 09:12:06  vfrolov
 * Made tidy up
 *
 * Revision 1.15  2003/12/19 15:25:27  vfrolov
 * Removed class AudioDelay (utilized PAdaptiveDelay)
 * Renamed pmodemQ to pmodem_pool
 * Fixed modem loss on no route
 *
 * Revision 1.14  2003/04/07 06:52:46  vfrolov
 * Fixed --save option
 *
 * Revision 1.13  2002/11/18 22:57:53  craigs
 * Added patches from Vyacheslav Frolov for CORRIGENDUM
 *
 * Revision 1.12  2002/11/05 13:46:51  vfrolov
 * Added missed --username option to help
 * Utilized "localpartyname" option from "dial" request
 *
 * Revision 1.11  2002/05/22 12:01:39  vfrolov
 * Implemented redundancy error protection scheme
 *
 * Revision 1.10  2002/05/16 00:11:02  robertj
 * Changed t38 handler creation function for new API
 *
 * Revision 1.9  2002/05/15 16:17:34  vfrolov
 * Implemented per modem routing for I/C calls
 *
 * Revision 1.8  2002/04/30 11:05:32  vfrolov
 * Implemented T.30 Calling Tone (CNG) generation
 *
 * Revision 1.7  2002/03/22 09:39:47  vfrolov
 * Removed obsoleted option -f
 *
 * Revision 1.6  2002/03/01 09:45:11  vfrolov
 * Added Copyright header
 * Added sending "established" command on connection established
 * Implemented mode change on receiving "requestmode" command
 * Added setting lastReadCount
 * Added some other changes
 *
 * Revision 1.5  2002/02/12 11:25:29  vfrolov
 * Removed obsoleted code
 *
 * Revision 1.4  2002/01/10 06:10:02  craigs
 * Added MPL header
 *
 */

// mostly "stolen" from OpenAM

#ifndef _PM_MAIN_H
#define _PM_MAIN_H

#include <h323.h>
#include <ptclib/delaychan.h>
#include "pmutils.h"
#include "t30tone.h"

class T38Modem : public PProcess
{
  PCLASSINFO(T38Modem, PProcess)

  public:
    T38Modem();
    ~T38Modem();

    void Main();

  protected:
    BOOL Initialise();
};

class PseudoModem;
class PseudoModemQ;
class MyH323Connection;

class MyH323EndPoint : public H323EndPoint
{
  PCLASSINFO(MyH323EndPoint, H323EndPoint);

  public:
    MyH323EndPoint();

    // overrides from H323EndPoint
    virtual H323Connection * CreateConnection(unsigned callReference, void *userData);

    // new functions
    BOOL Initialise(PConfigArgs & args);

    PseudoModem * PMAlloc(const PString &number) const;
    void PMFree(PseudoModem *pmodem) const;
    void SetOptions(MyH323Connection &conn, OpalT38Protocol *t38handler) const;

  protected:
    PseudoModemQ *pmodem_pool;
    PStringArray routes;
    WORD connectPort;

    int in_redundancy;
    int ls_redundancy;
    int hs_redundancy;
    BOOL old_asn;

    PDECLARE_NOTIFIER(PObject, MyH323EndPoint, OnMyCallback);
};

class OpalT38Protocol;

class MyH323Connection : public H323Connection
{
  PCLASSINFO(MyH323Connection, H323Connection);

  public:
    MyH323Connection(MyH323EndPoint &, unsigned);
    ~MyH323Connection();

    BOOL Attach(PseudoModem *_pmodem);

    // overrides from H323Connection
    AnswerCallResponse OnAnswerCall(const PString &, const H323SignalPDU &, H323SignalPDU &);
    void OnEstablished();
    BOOL OnStartLogicalChannel(H323Channel & channel);
    void OnClosedLogicalChannel(const H323Channel & channel);

    OpalT38Protocol * CreateT38ProtocolHandler();
    BOOL OpenAudioChannel(BOOL, unsigned, H323AudioCodec & codec);

  protected:
    const MyH323EndPoint & ep;
    
    PMutex        connMutex;
    PseudoModem * pmodem;
};

///////////////////////////////////////////////////////////////

class AudioRead : public PChannel
{
  PCLASSINFO(AudioRead, PChannel);

  public:
    AudioRead(T30Tone::Type type);
    BOOL Read(void * buffer, PINDEX amount);
    BOOL Close();

  protected:
    T30Tone t30Tone;
    BOOL closed;
    PAdaptiveDelay delay;
    PMutex Mutex;
};

class AudioWrite : public PChannel
{
  PCLASSINFO(AudioWrite, PChannel)

  public:
    AudioWrite();
    BOOL Write(const void * buf, PINDEX len);
    BOOL Close();

  protected:
    BOOL closed;
    PAdaptiveDelay delay;
    PMutex Mutex;
};

#endif  // _PM_MAIN_H


// End of File ///////////////////////////////////////////////////////////////

