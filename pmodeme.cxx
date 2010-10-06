/*
 * pmodeme.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2010 Vyacheslav Frolov
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
 * $Log: pmodeme.cxx,v $
 * Revision 1.98  2010-10-06 09:06:48  vfrolov
 * Fixed crash at dialing reset (reported by gorod225)
 *
 * Revision 1.98  2010/10/06 09:06:48  vfrolov
 * Fixed crash at dialing reset (reported by gorod225)
 *
 * Revision 1.97  2010/09/29 11:52:59  vfrolov
 * Redesigned engine attaching/detaching
 *
 * Revision 1.96  2010/09/22 15:51:13  vfrolov
 * Moved ResetModemState() to EngineBase
 *
 * Revision 1.95  2010/09/14 06:35:10  vfrolov
 * Implemented dial string terminated by a semicolon ("ATD<dialstring>;[...]")
 *
 * Revision 1.94  2010/09/10 18:08:07  vfrolov
 * Implemented +VTD command
 * Cleaned up code
 *
 * Revision 1.93  2010/09/10 05:34:12  vfrolov
 * Allowed "+ A B C D" dialing digits
 * Added ignoring unrecognized dialing digits (V.250)
 * Added missing on-hook's
 *
 * Revision 1.92  2010/09/08 17:22:23  vfrolov
 * Redesigned modem engine (continue)
 *
 * Revision 1.91  2010/07/09 04:46:55  vfrolov
 * Implemented alternate route
 *
 * Revision 1.90  2010/07/08 05:11:34  vfrolov
 * Redesigned modem engine (continue)
 *
 * Revision 1.89  2010/07/07 13:40:56  vfrolov
 * Fixed ussue with call clearing in stSend state
 * Added missing responce tracing
 *
 * Revision 1.88  2010/07/07 08:22:47  vfrolov
 * Redesigned modem engine
 *
 * Revision 1.87  2010/03/23 08:58:14  vfrolov
 * Fixed issues with +FTS and +FRS
 *
 * Revision 1.86  2010/03/18 08:42:17  vfrolov
 * Added named tracing of data types
 *
 * Revision 1.85  2010/02/05 14:55:33  vfrolov
 * Used S7 timeout
 *
 * Revision 1.84  2010/02/02 09:51:11  vfrolov
 * Added missing timerRing.Stop()
 *
 * Revision 1.83  2010/02/02 08:41:56  vfrolov
 * Implemented ringing indication for voice class dialing
 *
 * Revision 1.82  2010/01/28 10:30:37  vfrolov
 * Added cleaning user input buffers for non-audio classes
 *
 * Revision 1.81  2010/01/27 14:03:38  vfrolov
 * Added missing mutexes
 *
 * Revision 1.80  2010/01/22 14:11:40  vfrolov
 * Added missing characters # and * for extension numbers
 * Thanks to Dmitry
 *
 * Revision 1.79  2009/12/02 09:06:42  vfrolov
 * Added a short delay after transmitting of signal before call clearing
 *
 * Revision 1.78  2009/11/20 16:37:27  vfrolov
 * Fixed audio class application blocking by forced T.38 mode
 *
 * Revision 1.77  2009/11/19 11:18:16  vfrolov
 * Added handling T.38 CED indication
 *
 * Revision 1.76  2009/11/18 19:08:47  vfrolov
 * Moved common code to class EngineBase
 *
 * Revision 1.75  2009/11/17 11:25:48  vfrolov
 * Added missing delay before sending fax-no-force requestmode
 * Redesigned handling stConnectHandle state
 *
 * Revision 1.74  2009/11/06 10:02:29  vfrolov
 * Fixed typo in fax-no-force
 *
 * Revision 1.73  2009/11/02 18:06:33  vfrolov
 * Added fax-no-forse requestmode
 *
 * Revision 1.72  2009/10/27 18:25:22  vfrolov
 * Added reseting send state on detaching engines
 *
 * Revision 1.71  2009/10/01 13:31:12  vfrolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.70  2009/07/29 17:12:35  vfrolov
 * Wait audioEngine on stConnectHandle
 *
 * Revision 1.69  2009/07/10 15:23:31  vfrolov
 * Implicitly dial modifier '@' will continue of called number
 *
 * Revision 1.68  2009/07/10 14:04:13  vfrolov
 * Changed usage multiple dial modifiers '@'
 *   each next '@' overrides previous '@'
 *   ("ATD4444@123@456" eq "ATD4444@456", "ATD4444@123@" eq "ATD4444")
 * Dial modifiers (except 'D') can be used after '@'
 * Dial modifiers 'T' and 'P' can be used instead 'D'
 *
 * Revision 1.67  2009/07/06 08:28:44  vfrolov
 * Added DTMF shielding
 *
 * Revision 1.66  2009/07/03 16:38:17  vfrolov
 * Added more state tracing
 *
 * Revision 1.65  2009/07/03 16:22:56  vfrolov
 * Added missing pPlayTone deletings
 * Added more state tracing
 *
 * Revision 1.64  2009/07/02 15:09:48  vfrolov
 * Improved state tracing
 *
 * Revision 1.63  2009/07/02 06:55:30  vfrolov
 * Fixed +VSM=? and +VLS=? for modem type autodetecting by VentaFax
 * Thanks Dmitry (gorod225)
 *
 * Revision 1.62  2009/07/02 05:41:37  vfrolov
 * Enabled +VIT > 0 (for compatibility with some voice applications)
 *
 * Revision 1.61  2009/07/01 15:11:58  vfrolov
 * Fixed codec 128,"8-BIT LINEAR"
 *
 * Revision 1.60  2009/07/01 10:52:06  vfrolov
 * Enabled +VSM=<cml> w/o <vsr>
 *
 * Revision 1.59  2009/07/01 08:20:39  vfrolov
 * Implemented +VIP command
 *
 * Revision 1.58  2009/06/30 13:55:27  vfrolov
 * Added +VSM codecs
 *   128,"8-BIT LINEAR",8,0,(8000),(0),(0)
 *   130,"UNSIGNED PCM",8,0,(8000),(0),(0)
 *   131,"G.711 ULAW",8,0,(8000),(0),(0)
 * Added +VLS
 *   5,"ST",00000000,00000000,00000000
 *
 * Revision 1.57  2009/06/30 10:50:33  vfrolov
 * Added +VSM codecs
 *   0,"SIGNED PCM",8,0,(8000),(0),(0)
 *   1,"UNSIGNED PCM",8,0,(8000),(0),(0)
 *
 * Revision 1.56  2009/06/29 15:36:38  vfrolov
 * Added ability to dial in connection establised state
 *
 * Revision 1.55  2009/06/29 13:28:42  vfrolov
 * Added +VSM codecs
 *   4,"G711U",8,0,(8000),(0),(0)
 *   5,"G711A",8,0,(8000),(0),(0)
 *
 * Revision 1.54  2009/06/25 16:48:52  vfrolov
 * Added stub for +VSD command
 *
 * Revision 1.53  2009/06/25 12:46:38  vfrolov
 * Implemented dialing followed answering ("ATD<dialstring>;A")
 *
 * Revision 1.52  2009/06/24 13:12:58  vfrolov
 * Implemented +VEM and +VIT commands
 *
 * Revision 1.51  2009/06/24 12:48:37  vfrolov
 * Added stubs for +VGR and +VGT commands
 *
 * Revision 1.50  2009/06/24 12:19:01  vfrolov
 * Added stubs for +VRA and +VRN
 *
 * Revision 1.49  2009/06/24 08:04:46  vfrolov
 * Added semicolon concatenating of commands
 *
 * Revision 1.48  2009/06/22 16:05:48  vfrolov
 * Added ability to dial extension numbers
 *
 * Revision 1.47  2009/05/06 09:17:23  vfrolov
 * Enabled dialing characters # and *
 *
 * Revision 1.46  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.45  2008/09/10 07:05:06  frolov
 * Fixed doubled mutex lock
 *
 * Revision 1.44  2007/08/27 10:55:21  vfrolov
 * Added missing moreFrames = FALSE
 *
 * Revision 1.43  2007/08/24 16:12:14  vfrolov
 * Disabled CNG sending in voice class mode
 *
 * Revision 1.42  2007/05/04 09:58:57  vfrolov
 * Fixed Attach(audioEngine)
 *
 * Revision 1.41  2007/04/24 16:26:02  vfrolov
 * Fixed unexpected state change
 *
 * Revision 1.40  2007/04/09 08:07:12  vfrolov
 * Added symbolic logging ModemCallbackParam
 *
 * Revision 1.39  2007/03/30 07:56:36  vfrolov
 * Included g711.c
 *
 * Revision 1.38  2007/03/23 14:54:20  vfrolov
 * Fixed compiler warnings
 *
 * Revision 1.37  2007/03/23 10:14:35  vfrolov
 * Implemented voice mode functionality
 *
 * Revision 1.36  2007/03/22 16:26:04  vfrolov
 * Fixed compiler warnings
 *
 * Revision 1.35  2007/02/22 16:00:33  vfrolov
 * Implemented AT#HCLR command
 *
 * Revision 1.34  2006/12/11 11:19:48  vfrolov
 * Fixed race condition with modem Callback
 *
 * Revision 1.33  2006/12/07 10:53:24  vfrolov
 * Added OnParentStop()
 *
 * Revision 1.32  2006/12/01 13:35:25  vfrolov
 * Fixed modem locking after unexpected dial while connection
 *
 * Revision 1.31  2005/11/22 16:39:36  vfrolov
 * Fixed MSVC compile warning
 *
 * Revision 1.30  2005/03/05 15:42:39  vfrolov
 * Added missing check for PTRACING
 * Fixed typo in T38DLE trace
 *
 * Revision 1.29  2005/03/04 16:35:38  vfrolov
 * Implemented AT#DFRMC command
 * Redisigned class Profile
 *
 * Revision 1.28  2005/02/16 12:14:47  vfrolov
 * Send CONNECT just before data for AT+FRM command
 *
 * Revision 1.27  2005/02/10 10:35:18  vfrolov
 * Fixed AT prefix searching
 *
 * Revision 1.26  2005/02/03 11:32:11  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.25  2005/02/01 11:43:46  vfrolov
 * Implemented ATV0 command (numeric format for result codes)
 * Implemented AT+FMI?, AT+FMM? and AT+FMR? commands
 * Added stubs for ATBn, ATX3 and AT+FCLASS=0 commands
 * Added stub for AT+FLO command
 *
 * Revision 1.24  2004/10/27 13:36:26  vfrolov
 * Decreased binary, DLE, and callback tracing
 *
 * Revision 1.23  2004/07/06 16:07:24  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.22  2004/06/24 17:20:22  vfrolov
 * Added stub for ATXn command
 *
 * Revision 1.21  2004/05/09 07:46:11  csoutheren
 * Updated to compile with new PIsDescendant function
 *
 * Revision 1.20  2004/03/01 17:14:34  vfrolov
 * Fixed binary log in command mode
 *
 * Revision 1.19  2003/12/04 16:09:51  vfrolov
 * Implemented FCS generation
 * Implemented ECM support
 *
 * Revision 1.18  2003/01/08 16:58:58  vfrolov
 * Added cbpOutBufNoFull and isOutBufFull()
 *
 * Revision 1.17  2002/12/30 12:49:29  vfrolov
 * Added tracing thread's CPU usage (Linux only)
 *
 * Revision 1.16  2002/12/20 10:12:50  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.15  2002/12/19 10:31:33  vfrolov
 * Changed usage multiple dial modifiers 'L' (for secure reasons)
 *   each next 'L' overrides previous 'L'
 *   ("ATD4444L123L456" eq "ATD4444L456", "ATD4444L123L" eq "ATD4444")
 * Added dial modifier 'D' - continue dial number
 *   ("ATD000L123D4444" eq "ATD0004444L123")
 * Added mising spaces into "NMBR = " and "NDID = "
 *
 * Revision 1.14  2002/11/05 13:59:11  vfrolov
 * Implemented Local Party Number dial modifier 'L'
 * (put dial string 1234L5678 to dial 1234 from 5678)
 *
 * Revision 1.13  2002/05/15 16:10:52  vfrolov
 * Reimplemented AT+FTS and AT+FRS
 * Added workaround "Reset state stSendAckWait"
 *
 * Revision 1.12  2002/05/07 10:36:16  vfrolov
 * Added more code for case stResetHandle
 * Changed duration of CED (T.30 requires 2.6...4.0 secs)
 *
 * Revision 1.11  2002/04/19 14:06:04  vfrolov
 * Implemented T.38 mode request dial modifiers
 *   F - enable
 *   V - disable
 *
 * Revision 1.10  2002/04/03 02:45:36  vfrolov
 * Implemented AT#CID=10 - ANI/DNIS reporting between RINGs
 *
 * Revision 1.9  2002/03/01 14:59:48  vfrolov
 * Get data for Revision string from version.h
 *
 * Revision 1.8  2002/03/01 10:00:26  vfrolov
 * Added Copyright header
 * Implemented connection established handling and mode change request
 * Implemented ATI8 command
 * Fixed some deadlocks
 * Added some other changes
 *
 * Revision 1.7  2002/02/11 08:40:15  vfrolov
 * More clear call implementation
 *
 * Revision 1.6  2002/01/10 06:10:02  craigs
 * Added MPL header
 *
 * Revision 1.5  2002/01/06 03:48:45  craigs
 * Added changes to support efax 0.9
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.4  2002/01/03 21:36:00  craigs
 * Added change to use S1 register for number of rings on answer
 * Thanks to Vyacheslav Frolov
 *
 * Revision 1.3  2002/01/02 04:49:37  craigs
 * Added support for ATS0 register
 *
 * Revision 1.2  2002/01/01 23:59:52  craigs
 * Lots of additional implementation thanks to Vyacheslav Frolov
 *
 */

#include <ptlib.h>
#include <ptclib/dtmf.h>
#include "pmodemi.h"
#include "pmodeme.h"
#include "dle.h"
#include "fcs.h"
#include "t38engine.h"
#include "audio.h"
#include "version.h"

///////////////////////////////////////////////////////////////
#include "g711.c"
///////////////////////////////////////////////////////////////
static const char Manufacturer[] = "Vyacheslav Frolov";
static const char Model[] = "T38FAX";
#define _TOSTR(s) #s
#define TOSTR(s) _TOSTR(s)
static const char Revision[] = TOSTR(MAJOR_VERSION) "." TOSTR(MINOR_VERSION) "." TOSTR(BUILD_NUMBER);
///////////////////////////////////////////////////////////////
#define DeclareStringParam(name)	\
  public:			\
    void name(const PString &_s##name) { s##name = _s##name; }	\
    const PString &name() const { return s##name; }		\
  protected:			\
    PString s##name;
///////////////////////////////////////////////////////////////
class Profile
{
  enum {
    MaxReg = 59,
    MaxRegVoice = MaxReg - 10,
    MinRegVoice = MaxRegVoice - 9,
    MaxBit = 7
  };

  public:
    Profile();

    #define DeclareRegisterBit(name, byte, bit)	\
      void name(PBoolean val) { SetBit(byte, bit, val); } \
      PBoolean name() const { PBoolean val; GetBit(byte, bit, val); return val; }

    DeclareRegisterBit(Echo, 23, 0);
    DeclareRegisterBit(asciiResultCodes, 23, 6);
    DeclareRegisterBit(noResultCodes, 23, 7);

    #define DeclareRegisterByte(name, byte)	\
      void name(BYTE val) { SetReg(byte, val); } \
      BYTE name() const { BYTE val; GetReg(byte, val); return val; }

    DeclareRegisterByte(AutoAnswer,    0);
    DeclareRegisterByte(RingCount,     1);
    DeclareRegisterByte(S7,            7);
    DeclareRegisterByte(DialTimeComma, 8);
    DeclareRegisterByte(DialTimeDTMF, 11);

    DeclareRegisterByte(Vtd,               MaxRegVoice - 7);
    DeclareRegisterByte(Vcml,              MaxRegVoice - 6);
    DeclareRegisterByte(Vsds,              MaxRegVoice - 5);
    DeclareRegisterByte(Vsdi,              MaxRegVoice - 4);
    DeclareRegisterByte(VgrInterval,       MaxRegVoice - 3);
    DeclareRegisterByte(VgtInterval,       MaxRegVoice - 2);
    DeclareRegisterByte(VraInterval,       MaxRegVoice - 1);
    DeclareRegisterByte(VrnInterval,       MaxRegVoice - 0);

    DeclareRegisterByte(IfcByDCE,          MaxReg - 5);
    DeclareRegisterByte(IfcByDTE,          MaxReg - 4);
    DeclareRegisterByte(ClearMode,         MaxReg - 3);
    DeclareRegisterByte(DelayFrmConnect,   MaxReg - 2);
    DeclareRegisterByte(DidMode,           MaxReg - 1);
    DeclareRegisterByte(CidMode,           MaxReg - 0);

    void Flo(BYTE val) { IfcByDTE(val); IfcByDCE(val); }
    BYTE Flo() const {
      if (IfcByDTE() == IfcByDCE())
        return IfcByDTE();
      return 255;
    }

    PBoolean SetBit(PINDEX r, PINDEX b, PBoolean val) {
      if( !ChkRB(r, b) ) return FALSE;
      BYTE msk = MaskB(b);
      if( val ) S[r] |= msk;
      else      S[r] &= ~msk;
      return TRUE;
    }
    PBoolean GetBit(PINDEX r, PINDEX b, PBoolean &val) const {
      if (!ChkRB(r, b)) {
        val = 0;
        return FALSE;
      }
      BYTE msk = MaskB(b);
      val = (S[r] & msk) ? TRUE : FALSE;
      return TRUE;
    }
    PBoolean SetReg(PINDEX r, BYTE val) {
      if( !ChkR(r) ) return FALSE;
      S[r] = val;
      return TRUE;
    }
    PBoolean GetReg(PINDEX r, BYTE &val) const {
      if (!ChkR(r)) {
        val = 0;
        return FALSE;
      }
      val = S[r];
      return TRUE;
    }
    PBoolean SetBits(PINDEX r, PINDEX bl, PINDEX bh, BYTE val) {
      if( !ChkRBB(r, bl, bh) ) return FALSE;
      BYTE msk = MaskBB(bl, bh);
      S[r] &= ~msk;
      S[r] &= (val << bl) & msk;
      return TRUE;
    }
    PBoolean GetBits(PINDEX r, PINDEX bl, PINDEX bh, BYTE &val) const {
      if (!ChkRBB(r, bl, bh)) {
        val = 0;
        return FALSE;
      }
      BYTE msk = MaskBB(bl, bh);
      val = BYTE((S[r] & msk) >> bl);
      return TRUE;
    }

    Profile &operator=(const Profile &p);
    Profile &SetVoiceProfile(const Profile &p);

    void ModemClass(const PString &_modemClass) {
      modemClass = _modemClass;

      if (modemClass == "1") {
        modemClassId = EngineBase::mcFax;
      }
      else
      if (modemClass == "8") {
        modemClassId = EngineBase::mcAudio;
      }
      else {
        modemClassId = EngineBase::mcUndefined;
      }
    }

    const PString &ModemClass() const { return modemClass; }
    EngineBase::ModemClass ModemClassId() const { return modemClassId; }

  protected:
    static PBoolean ChkR(PINDEX r) {
      return r <= MaxReg;
    }
    static PBoolean ChkB(PINDEX b) {
      return b <= MaxBit;
    }
    static PBoolean ChkRB(PINDEX r, PINDEX b) {
      return ChkR(r) && ChkB(b);
    }
    static PBoolean ChkRBB(PINDEX r, PINDEX bl, PINDEX bh) {
      return ChkR(r) && ChkB(bl) && ChkB(bh) && bl <= bh;
    }
    static BYTE MaskB(PINDEX b) {
      return "\x01\x02\x04\x08\x10\x20\x40\x80"[b];
    }
    static BYTE MaskBB(PINDEX bl, PINDEX bh) { // bl <= bh
      return BYTE(("\x01\x03\x07\x0F\x1F\x3F\x7F\xFF"[bh - bl]) << bl);
    }

    BYTE S[MaxReg + 1];	// S-registers
    PString modemClass;
    EngineBase::ModemClass modemClassId;
};

static const Profile Profiles[1];
///////////////////////////////////////////////////////////////
class Timeout : public PTimer
{
    PCLASSINFO(Timeout, PTimer);
  public:
    Timeout(const PNotifier &callback, PBoolean _continuous = FALSE)
        : state(0), continuous(_continuous) {
      SetNotifier(callback);
    }

    void Start(unsigned period) {
      PWaitAndSignal mutexWait(Mutex);
      state = 1;
      if( continuous ) {
        RunContinuous(period);
        OnTimeout();
      } else {
        PTimer::operator=(period);
      }
    }

    void Stop() {
      PWaitAndSignal mutexWait(Mutex);
      state = 0;
      PTimer::operator=(0);
    }

    PBoolean Get() {
      PWaitAndSignal mutexWait(Mutex);
      if (state == 2) {
        state = continuous ? 1 : 0;
        return TRUE;
      }
      return FALSE;
    }
  protected:
    void OnTimeout() {
      PWaitAndSignal mutexWait(Mutex);
      if( state == 1 )
        state = 2;
      PTimer::OnTimeout();
    }

    int state;
    PBoolean continuous;
    PMutex Mutex;
};
///////////////////////////////////////////////////////////////
enum CallState {  // ---------+---------------------------+--------+------------------------------------------------+
                  //          |     on sending command    |   on   |               on received command              |
                  //          +---------------------------+clearing+------------------------------------------------+
                  //          |"dial"|"answer"|"clearcall"|off_hook|"call"|"alerting"|"established"|  "clearcall"   |
                  // ---------+------+--------+-----+-----+--------+------+----------+-------------+----------+-----+
                  // off_hook | true |  true  |true |false|  true  |false |   true   |     true    |   true   |false|
                  // ---------+------+--------+-----+-----+--------+------+----------+-------------+----------+-----+
  cstCleared,     //            )--+                 <--+    <--+    )--+                                <--+  <--+ |
                  //               |                    |       |       |                                   |     | |
                  //               |                    |       |       |                                   |     | |
  cstDialing,     //            <--+           )--+  )--+    )--+       |     )--+         )--+     )--+ )--+  )--+ |
                  //                              |     |       |       |        |            |        |          | |
  cstAlerted,     //                           )--+  )--+    )--+       |     <--+         )--+     )--+       )--+ |
                  //                              |     |       |       |                     |        |          | |
                  //                              |     |       |       |                     |        |          | |
  cstCalled,      //                    )--+   )--+  )--+       |    <--+                     |        |       )--+ |
                  //                       |      |     |       |                             |        |          | |
  cstAnswering,   //                    <--+   )--+  )--+    )--+                          )--+     )--+       )--+ |
                  //                              |     |       |                             |        |          | |
                  //                              |     |       |                             |        |          | |
  cstEstablished, //                           )--+  )--+    )--+                          <--+     )--+       )--+ |
                  //                              |     |       |                                      |          | |
  cstReleasing,   //                           <--+  )--+    )--+                                   <--+       )--+ |
};                // ---------+------+--------+-----+-----+--------+------+----------+-------------+----------+-----+

#if PTRACING
static ostream & operator<<(ostream & out, CallState state)
{
  switch (state) {
    case cstCleared:            return out << "cstCleared";
    case cstDialing:            return out << "cstDialing";
    case cstAlerted:            return out << "cstAlerted";
    case cstCalled:             return out << "cstCalled";
    case cstAnswering:          return out << "cstAnswering";
    case cstEstablished:        return out << "cstEstablished";
    case cstReleasing:          return out << "cstReleasing";
  }

  return out << "cst" << INT(state);
}
#endif
///////////////////////////////////////////////////////////////
#if PTRACING
struct CallStateAndSubState {
  CallStateAndSubState(CallState s, int ss) : state(s), subState(ss) {}

  CallState state;
  int subState;
};

static ostream & operator<<(ostream & out, const CallStateAndSubState &stateAndSubState)
{
  out << stateAndSubState.state;

  if (stateAndSubState.state == cstReleasing)
      out << "." << stateAndSubState.subState;

  return out;
}
#endif
///////////////////////////////////////////////////////////////
enum State {
  stCommand,
  stDial,
  stConnectWait,
  stConnectHandle,
  stReqModeAckWait,
  stReqModeAckHandle,
  stSend,
  stSendBufEmptyHandle,
  stSendAckWait,
  stSendAckHandle,
  stRecvBegWait,
  stRecvBegHandle,
  stRecv,
};

#if PTRACING
static ostream & operator<<(ostream & out, State state)
{
  switch (state) {
    case stCommand:             return out << "stCommand";
    case stDial:                return out << "stDial";
    case stConnectWait:         return out << "stConnectWait";
    case stConnectHandle:       return out << "stConnectHandle";
    case stReqModeAckWait:      return out << "stReqModeAckWait";
    case stReqModeAckHandle:    return out << "stReqModeAckHandle";
    case stSend:                return out << "stSend";
    case stSendBufEmptyHandle:  return out << "stSendBufEmptyHandle";
    case stSendAckWait:         return out << "stSendAckWait";
    case stSendAckHandle:       return out << "stSendAckHandle";
    case stRecvBegWait:         return out << "stRecvBegWait";
    case stRecvBegHandle:       return out << "stRecvBegHandle";
    case stRecv:                return out << "stRecv";
  }

  return out << "st" << INT(state);
}
#endif
///////////////////////////////////////////////////////////////
enum SubStateConnectHandle {
  chConnected,
  chWaitAudioEngine,
  chAudioEngineAttached,
  chWaitPlayTone,
  chTonePlayed,
  chConnectionEstablishDelay,
  chConnectionEstablished,
};

#if PTRACING
static ostream & operator<<(ostream & out, SubStateConnectHandle subState)
{
  switch (subState) {
    case chConnected:                   return out << "chConnected";
    case chWaitAudioEngine:             return out << "chWaitAudioEngine";
    case chAudioEngineAttached:         return out << "chAudioEngineAttached";
    case chWaitPlayTone:                return out << "chWaitPlayTone";
    case chTonePlayed:                  return out << "chTonePlayed";
    case chConnectionEstablishDelay:    return out << "chConnectionEstablishDelay";
    case chConnectionEstablished:       return out << "chConnectionEstablished";
  }

  return out << "ch" << INT(subState);
}
#endif
///////////////////////////////////////////////////////////////
#if PTRACING
struct StateAndSubState {
  StateAndSubState(State s, int ss) : state(s), subState(ss) {}

  State state;
  int subState;
};

static ostream & operator<<(ostream & out, const StateAndSubState &stateAndSubState)
{
  out << stateAndSubState.state;

  if (stateAndSubState.state == stConnectHandle)
      out << "." << SubStateConnectHandle(stateAndSubState.subState);

  return out;
}
#endif
///////////////////////////////////////////////////////////////
enum ModemClassEngine {
  mceAudio,
  mceT38,
  mceNumberOfItems,
};

#if PTRACING
static ostream & operator<<(ostream & out, ModemClassEngine mce)
{
  switch (mce) {
    case mceAudio:           return out << "mceAudio";
    case mceT38:             return out << "mceT38";
    default:                 break;
  }

  return out << "mce" << INT(mce);
}
#endif
///////////////////////////////////////////////////////////////
#define DeclareResultCode(name, v0, v1)	\
  PString name() const { return P.asciiResultCodes() ? (v1) : (v0); }
///////////////////////////////////////////////////////////////
class ModemEngineBody : public PObject
{
    PCLASSINFO(ModemEngineBody, PObject);
  public:

  /**@name Construction */
  //@{
    ModemEngineBody(ModemEngine &_parent, const PNotifier &_callbackEndPoint);
    ~ModemEngineBody();
  //@}

  /**@name Operations */
  //@{
    PBoolean Request(PStringToString &request);
    EngineBase *NewPtrEngine(ModemClassEngine mce);
    void OnParentStop();
    void HandleData(const PBYTEArray &buf, PBYTEArray &bresp);
    void CheckState(PBYTEArray &bresp);

    PBoolean IsReady() const {
      PWaitAndSignal mutexWait(Mutex);
      return state == stCommand && !off_hook && callState == cstCleared && (PTime() - lastOnHookActivity) > 5*1000;
    }

    PBoolean isOutBufFull() const {
      PWaitAndSignal mutexWait(Mutex);
      return currentClassEngine && currentClassEngine->isOutBufFull();
    }
  //@}

  protected:
    PBoolean Echo() const { return P.Echo(); }
    PBoolean HandleClass1Cmd(const char **ppCmd, PString &resp, PBoolean &ok, PBoolean &crlf);
    PBoolean HandleClass8Cmd(const char **ppCmd, PString &resp, PBoolean &ok, PBoolean &crlf);
    PBoolean Answer();
    void HandleCmd(PString &resp);
    void HandleCmdRest(PString &resp);

    unsigned EstablishmentTimeout() const {
      return unsigned(P.S7() ? P.S7() : Profiles[0].S7()) * 1000;
    }

    void ResetDleData() {
      dleData.Clean();
      dataCount = 0;
      moreFrames = FALSE;
    }

    PBoolean SetBitRevDleData() {
      switch (P.ModemClassId()) {
      case EngineBase::mcAudio:
        switch (P.Vcml()) {
          case 132:
            dleData.BitRev(TRUE);
            break;
          default:
            dleData.BitRev(FALSE);
        }
        break;
      case EngineBase::mcFax:
        dleData.BitRev(TRUE);
        break;
      default:
        return FALSE;
      }

      return TRUE;
    }

    PBoolean SendSilence(int ms) {
      if (P.ModemClassId() == EngineBase::mcFax && currentClassEngine) {
        dataType = EngineBase::dtSilence;
        SetState(stSend);

        if (currentClassEngine->SendStart(dataType, ms)) {
          SetState(stSendAckWait);
          if (currentClassEngine->SendStop(FALSE, NextSeq()))
            return TRUE;
        }
      }

      SetState(stCommand);
      return FALSE;
    }

    PBoolean _SendStart(EngineBase::DataType dt, int br, PString &resp) {
      dataType = dt;
      ResetDleData();
      SetBitRevDleData();
      SetState(stSend);

      if (!currentClassEngine || !currentClassEngine->SendStart(dataType, br)) {
        SetState(stCommand);
        return FALSE;
      }

      resp = RC_CONNECT();
      return TRUE;
    }

    PBoolean SendStart(EngineBase::DataType dt, int br, PString &resp) {
      PWaitAndSignal mutexWait(Mutex);
      return _SendStart(dt, br, resp);
    }

    PBoolean RecvStart(EngineBase::DataType dt, int br) {
      PBoolean done = FALSE;

      PWaitAndSignal mutexWait(Mutex);
      dataType = dt;
      SetBitRevDleData();
      SetState(stRecvBegWait);
      timeout.Start(60000);

      if (!currentClassEngine || !currentClassEngine->RecvWait(dataType, br, NextSeq(), done)) {
        SetState(stCommand);
        timeout.Stop();
        return FALSE;
      }

      if (done) {
        SetState(stRecvBegHandle);
        timeout.Stop();
        parent.SignalDataReady();
      }

      return TRUE;
    }

    void _AttachEngine(ModemClassEngine mce);
    void _DetachEngine(ModemClassEngine mce);
    void _ClearCall();

    int NextSeq() { return seq = ++seq & EngineBase::cbpUserDataMask; }

    ModemEngine &parent;

    EngineBase *activeEngines[mceNumberOfItems];
    EngineBase *currentClassEngine;

    const PNotifier callbackEndPoint;

    PDECLARE_NOTIFIER(PObject, ModemEngineBody, OnEngineCallback);
    PDECLARE_NOTIFIER(PObject, ModemEngineBody, OnTimerCallback);
    const PNotifier engineCallback;
    const PNotifier timerCallback;
    Timeout timerRing;
    Timeout timerBusy;
    Timeout timeout;
    PTime lastOnHookActivity;

    int seq;

    PBoolean forceFaxMode;
    PBoolean connectionEstablished;

    PBoolean off_hook;

    int lockReleasingState;

    enum CallDirection {
      cdUndefined,
      cdOutgoing,
      cdIncoming,
    };

    CallDirection callDirection;
    CallState callState;
    int callSubState;
    State state;
    int subState;

    #define TRACE_STATE(level, header) \
        PTRACE(level, header \
                  " " << (off_hook ? "off" : "on") << "-hook" \
                  " " << CallStateAndSubState(callState, callSubState) << \
                  " " << StateAndSubState(state, subState))

    PBoolean OffHook() {
      if (!off_hook) {
        off_hook = TRUE;
        TRACE_STATE(4, "ModemEngineBody::OffHook:");
        return TRUE;
      }
      return FALSE;
    }

    void OnHook();

    void SetCallState(CallState newState) {
      if (callState != newState || callSubState != 0) {
        callState = newState;
        callSubState = 0;
        TRACE_STATE(4, "ModemEngineBody::SetCallState:");
      }
    }

    void SetCallSubState(int newSubState) {
      if (callSubState != newSubState) {
        callSubState = newSubState;
        TRACE_STATE(4, "ModemEngineBody::SetCallSubState:");
      }
    }

    void SetState(State newState, int newSubState = 0) {
      if (state != newState || subState != newSubState) {
        state = newState;
        subState = newSubState;
        TRACE_STATE(4, "ModemEngineBody::SetState:");
      }
    }

    void SetSubState(int newSubState) {
      if (subState != newSubState) {
        subState = newSubState;
        TRACE_STATE(4, "ModemEngineBody::SetSubState:");
      }
    }

    void OnChangeModemClass() {
      for (int i = 0 ; i < (int)(sizeof(activeEngines)/sizeof(activeEngines[0])) ; i++) {
        if (activeEngines[i])
          activeEngines[i]->ChangeModemClass(P.ModemClassId());
      }

      switch (P.ModemClassId()) {
        case EngineBase::mcAudio:
          currentClassEngine = activeEngines[mceAudio];
          break;
        case EngineBase::mcFax:
          currentClassEngine = activeEngines[mceT38];
          break;
        default:
          currentClassEngine = NULL;
      }
    }

    void SendOnIdle(EngineBase::DataType dt) {
      if (sendOnIdle != dt) {
        sendOnIdle = dt;

        PTRACE(4, "ModemEngineBody::SendOnIdle " << sendOnIdle);

        for (int i = 0 ; i < (int)(sizeof(activeEngines)/sizeof(activeEngines[0])) ; i++) {
          if (activeEngines[i])
            activeEngines[i]->SendOnIdle(sendOnIdle);
        }
      }
    }

    int param;
    PStringToString params;
    PString cmd;
    PString cmdRest;
    EngineBase::DataType dataType;
    EngineBase::DataType sendOnIdle;

    PDTMFEncoder *pPlayTone;

    DLEData dleData;
    PINDEX dataCount;
    PBoolean moreFrames;
    FCS fcs;

    Profile P;

    PMutex Mutex;

    DeclareStringParam(CallToken)
    DeclareStringParam(SrcNum)
    DeclareStringParam(DstNum)

    DeclareResultCode(RC_PREF,            "", "\r\n")

    DeclareResultCode(RC_OK,           "0\r", "OK\r\n")
    DeclareResultCode(RC_CONNECT,      "1\r", "CONNECT\r\n")
    DeclareResultCode(RC_RING,         "2\r", "RING\r\n")
    DeclareResultCode(RC_NO_CARRIER,   "3\r", "NO CARRIER\r\n")
    DeclareResultCode(RC_ERROR,        "4\r", "ERROR\r\n")
    DeclareResultCode(RC_CONNECT_1200, "5\r", "CONNECT 1200\r\n")
    DeclareResultCode(RC_NO_DIALTONE,  "6\r", "NO DIALTONE\r\n")
    DeclareResultCode(RC_BUSY,         "7\r", "BUSY\r\n")
    DeclareResultCode(RC_NO_ANSWER,    "8\r", "NO ANSWER\r\n")
    DeclareResultCode(RC_RINGING,      "9\r", "RINGING\r\n")
    DeclareResultCode(RC_FCERROR,    "+F4\r", "+FCERROR\r\n")
};
///////////////////////////////////////////////////////////////

#define new PNEW

///////////////////////////////////////////////////////////////
ModemEngine::ModemEngine(PseudoModemBody &_parent)
  : ModemThreadChild(_parent)
{
  body = new ModemEngineBody(*this, Parent().GetCallbackEndPoint());
}

ModemEngine::~ModemEngine()
{
  if( body )
    delete body;
}

PBoolean ModemEngine::IsReady() const
{
  return body && body->IsReady();
}

T38Engine *ModemEngine::NewPtrT38Engine() const
{
  if (!body)
    return NULL;

  EngineBase *engine = body->NewPtrEngine(mceT38);

  PAssert(engine == NULL || PIsDescendant(engine, T38Engine), PInvalidCast);

  return (T38Engine *)engine;
}

AudioEngine *ModemEngine::NewPtrAudioEngine() const
{
  if (!body)
    return NULL;

  EngineBase *engine = body->NewPtrEngine(mceAudio);

  PAssert(engine == NULL || PIsDescendant(engine, AudioEngine), PInvalidCast);

  return (AudioEngine *)engine;
}

PBoolean ModemEngine::Request(PStringToString &request) const
{
  return body && body->Request(request);
}

void ModemEngine::Main()
{
  RenameCurrentThread(ptyName() + "(e)");

  myPTRACE(1, "<-> Started");
  if( !body ) {
    myPTRACE(1, "<-> no body" << ptyName());
    SignalStop();
    return;
  }

  for(;;) {
    PBYTEArray bresp;

    if (stop)
      break;

    body->CheckState(bresp);

    if (stop)
      break;

    while( !body->isOutBufFull() ) {
      PBYTEArray *buf = Parent().FromInPtyQ();

      if (buf)  {
        body->HandleData(*buf, bresp);
        delete buf;
        if (stop)
          break;
      } else
          break;
    }

    if (stop)
      break;

    if (bresp.GetSize()) {
      ToPtyQ(bresp, bresp.GetSize());
    }

    if (stop)
      break;

    WaitDataReady();
  }

  body->OnParentStop();

  myPTRACE(1, "<-> Stopped" << GetThreadTimes(", CPU usage: "));
}

///////////////////////////////////////////////////////////////
Profile::Profile() {
  for( PINDEX r = 0 ; r <= MaxReg ; r++ ) {
    S[r] = 0;
  }

  S7(60);
  DialTimeComma(2);
  DialTimeDTMF(70);
  Echo(TRUE);
  VraInterval(50);
  VrnInterval(10);
  Vsds(128);
  Vsdi(50);
  Vcml(132);
  Vtd(100);
  asciiResultCodes(TRUE);
  noResultCodes(FALSE);
  ModemClass("1");
}

Profile &Profile::operator=(const Profile &p) {
  for( PINDEX r = 0 ; r <= MaxReg ; r++ ) {
    S[r] = p.S[r];
  }
  ModemClass(p.ModemClass());
  return *this;
}

Profile &Profile::SetVoiceProfile(const Profile &p) {
  for (PINDEX r = MinRegVoice ; r <= MaxRegVoice ; r++)
    S[r] = p.S[r];
  return *this;
}
///////////////////////////////////////////////////////////////
ModemEngineBody::ModemEngineBody(ModemEngine &_parent, const PNotifier &_callbackEndPoint)
  : parent(_parent),
    currentClassEngine(NULL),
    callbackEndPoint(_callbackEndPoint),
#ifdef _MSC_VER
#pragma warning(disable:4355) // warning C4355: 'this' : used in base member initializer list
#endif
    engineCallback(PCREATE_NOTIFIER(OnEngineCallback)),
    timerCallback(PCREATE_NOTIFIER(OnTimerCallback)),
#ifdef _MSC_VER
#pragma warning(default:4355)
#endif
    timerRing(timerCallback, TRUE),
    timerBusy(timerCallback, TRUE),
    timeout(timerCallback),
    seq(0),
    forceFaxMode(FALSE),
    connectionEstablished(FALSE),
    off_hook(FALSE),
    lockReleasingState(0),
    callDirection(cdUndefined),
    callState(cstCleared),
    state(stCommand),
    dataType(EngineBase::dtNone),
    sendOnIdle(EngineBase::dtNone),
    pPlayTone(NULL)
{
  for (int i = 0 ; i < (int)(sizeof(activeEngines)/sizeof(activeEngines[0])) ; i++)
    activeEngines[i] = NULL;
}

ModemEngineBody::~ModemEngineBody()
{
  PWaitAndSignal mutexWait(Mutex);

  OnHook();

  timeout.Stop();
  timerRing.Stop();
  timerBusy.Stop();
}

void ModemEngineBody::OnParentStop()
{
  PWaitAndSignal mutexWait(Mutex);

  OnHook();
}

void ModemEngineBody::OnHook()
{
  lastOnHookActivity = PTime();

  if (off_hook) {
    timerBusy.Stop();
    timeout.Stop();
    off_hook = FALSE;
    callDirection = cdUndefined;
    forceFaxMode = FALSE;
    state = stCommand;
    subState = 0;
    //_DetachEngine(mceT38);
    //_DetachEngine(mceAudio);
    TRACE_STATE(4, "ModemEngineBody::OnHook:");
  }

  _ClearCall();
}

void ModemEngineBody::_ClearCall()
{
  if (callState == cstCleared)
    return;

  lockReleasingState++;

  if (callState != cstReleasing) {
    SetCallState(cstReleasing);

    timerRing.Stop();
    connectionEstablished = FALSE;

    if (pPlayTone) {
      delete pPlayTone;
      pPlayTone = NULL;
    }
  }

  _DetachEngine(mceT38);
  _DetachEngine(mceAudio);

  if (!CallToken().IsEmpty()) {
    PStringToString request;
    request.SetAt("modemtoken", parent.modemToken());
    request.SetAt("command", "clearcall");
    request.SetAt("calltoken", CallToken());
    CallToken("");

    Mutex.Signal();
    callbackEndPoint(request, 1);
    Mutex.Wait();
  }

  if (--lockReleasingState == 0 && !off_hook)
      SetCallState(cstCleared);

  parent.SignalDataReady();
}

PBoolean ModemEngineBody::Request(PStringToString &request)
{
  myPTRACE(3, "ModemEngineBody::Request: " << state << " request={\n" << request << "}");

  PString command = request("command");

  request.SetAt("response", "reject");

  if( command == "call" ) {
    PWaitAndSignal mutexWait(Mutex);

    if (callState != cstCleared) {
      myPTRACE(1, "ModemEngineBody::Request: call already in " << callState << " state");
    }
    else
    if (off_hook) {
      myPTRACE(1, "ModemEngineBody::Request: line already in off-hook state");
    }
    else {
      SetCallState(cstCalled);

      CallToken(request("calltoken"));
      SrcNum(request("srcnum"));
      DstNum(request("dstnum"));
      timerRing.Start(5000);
      P.RingCount(0);
      request.SetAt("response", "confirm");
    }
  }
  else
  if (command == "alerting") {
    PWaitAndSignal mutexWait(Mutex);

    if (callState != cstDialing) {
      myPTRACE(1, "ModemEngineBody::Request: call already in " << callState << " state");
    }
    else
    if (!off_hook) {
      myPTRACE(1, "ModemEngineBody::Request: line already in on-hook state");
    }
    else
    if (CallToken() == request("calltoken")) {
      SetCallState(cstAlerted);

      if (state == stConnectWait && !pPlayTone && P.ModemClassId() == EngineBase::mcAudio) {
        _AttachEngine(mceAudio);
        SetState(stConnectHandle, chConnected);
        timerRing.Start(5000);
        parent.SignalDataReady();
        request.SetAt("response", "confirm");
      }
    }
    else {
      myPTRACE(1, "ModemEngineBody::Request: line already in use by " << CallToken());
    }
  }
  else
  if (command == "established") {
    PWaitAndSignal mutexWait(Mutex);

    if (callState != cstDialing && callState != cstAlerted && callState != cstAnswering) {
      myPTRACE(1, "ModemEngineBody::Request: call already in " << callState << " state");
    }
    else
    if (!off_hook) {
      myPTRACE(1, "ModemEngineBody::Request: line already in on-hook state");
    }
    else
    if (CallToken() == request("calltoken")) {
      timerRing.Stop();
      SetCallState(cstEstablished);

      if (state == stConnectWait) {
        SetState(stConnectHandle, chConnected);
        parent.SignalDataReady();
        request.SetAt("response", "confirm");
      }
    }
    else {
      myPTRACE(1, "ModemEngineBody::Request: line already in use by " << CallToken());
    }
  }
  else
  if (command == "clearcall") {
    PWaitAndSignal mutexWait(Mutex);

    if (callState == cstCleared || callState == cstReleasing) {
      myPTRACE(1, "ModemEngineBody::Request: call already in " << callState << " state");
    }
    else
    if (CallToken() == request("calltoken")) {
      CallToken("");

      if (callState == cstDialing && state == stConnectWait && request("trynextcommand") == "dial") {
        timerRing.Stop();
        SetCallState(cstCleared);
        SetState(stDial);
        params = request;
        params.RemoveAt("command");
        params.RemoveAt("calltoken");
        params.RemoveAt("trynextcommand");
        parent.SignalDataReady();
      } else {
        _ClearCall();
      }

      request.SetAt("response", "confirm");
    }
    else {
      myPTRACE(1, "ModemEngineBody::Request: line already in use by " << CallToken());
    }
  }
  else {
    myPTRACE(1, "ModemEngineBody::Request: unknown request " << command);
  }
  return TRUE;
}

EngineBase *ModemEngineBody::NewPtrEngine(ModemClassEngine mce)
{
  PAssert(mce == mceT38 || mce == mceAudio, "mce is not valid");

  PWaitAndSignal mutexWait(Mutex);

  _AttachEngine(mce);

  if (activeEngines[mce]) {
    activeEngines[mce]->AddReference();

    myPTRACE(1, "ModemEngineBody::NewPtrEngine created pointer for engine " << mce);
  }

  return activeEngines[mce];
}

void ModemEngineBody::_AttachEngine(ModemClassEngine mce)
{
  PAssert(mce == mceT38 || mce == mceAudio, "mce is not valid");

  if (activeEngines[mce] == NULL) {
    EngineBase *engine;

    switch (mce) {
      case mceT38:
        engine = new T38Engine(parent.ptyName());
        break;
      case mceAudio:
        engine = new AudioEngine(parent.ptyName());
        break;
      default:
        myPTRACE(1, parent.ptyName() << " ModemEngineBody::_AttachEngine Invalid mce " << mce);
        return;
    }

    if (engine->TryLockModemCallback()) {
      if (!engine->Attach(engineCallback)) {
        myPTRACE(1, parent.ptyName() << " ModemEngineBody::_AttachEngine Can't attach engineCallback to " << mce);
        engine->UnlockModemCallback();
        ReferenceObject::DelPointer(engine);
        return;
      }
      engine->UnlockModemCallback();
      activeEngines[mce] = engine;
    } else {
      myPTRACE(1, parent.ptyName() << " ModemEngineBody::_AttachEngine Can't lock ModemCallback for " << mce);
      ReferenceObject::DelPointer(engine);
      return;
    }
  }

  activeEngines[mce]->ChangeModemClass(P.ModemClassId());
  activeEngines[mce]->SendOnIdle(sendOnIdle);

  switch (mce) {
    case mceT38:
      if (P.ModemClassId() == EngineBase::mcFax)
        currentClassEngine = activeEngines[mce];

      if (state == stReqModeAckWait) {
        SetState(stReqModeAckHandle);
        timeout.Stop();
        parent.SignalDataReady();
      }
      break;
    case mceAudio:
      if (P.ModemClassId() == EngineBase::mcAudio) {
        currentClassEngine = activeEngines[mce];
      }

      if (state == stConnectHandle && subState == chWaitAudioEngine) {
        SetSubState(chAudioEngineAttached);
        timeout.Stop();
        parent.SignalDataReady();
      }
      break;
    default:
      break;
  }

  myPTRACE(1, "ModemEngineBody::_AttachEngine Attached " << mce);
}

void ModemEngineBody::_DetachEngine(ModemClassEngine mce)
{
  PAssert(mce == mceT38 || mce == mceAudio, "mce is not valid");

  if (activeEngines[mce] == NULL)
    return;

  if (!CallToken().IsEmpty() && activeEngines[mce]->SendingNotCompleted()) {
    Mutex.Signal();
    myPTRACE(2, "ModemEngineBody::_DetachEngine: sending is not completed for " << mce);
    PThread::Sleep(100);
    Mutex.Wait();

    if (activeEngines[mce] == NULL)
      return;
  }

  for (;;) {
    if (activeEngines[mce]->TryLockModemCallback()) {
      activeEngines[mce]->Detach(engineCallback);
      activeEngines[mce]->UnlockModemCallback();
      ReferenceObject::DelPointer(activeEngines[mce]);
      activeEngines[mce] = NULL;
      break;
    }

    Mutex.Signal();
    PThread::Sleep(20);
    Mutex.Wait();

    if (activeEngines[mce] == NULL)
      return;
  }

  switch (mce) {
    case mceT38:
      if (P.ModemClassId() == EngineBase::mcFax) {
        currentClassEngine = NULL;
        parent.SignalDataReady();
      }
      break;
    case mceAudio:
      if (P.ModemClassId() == EngineBase::mcAudio) {
        currentClassEngine = NULL;
        parent.SignalDataReady();
      }
      break;
    default:
      break;
  }

  myPTRACE(1, "ModemEngineBody::_DetachEngine Detached " << mce);
}

void ModemEngineBody::OnEngineCallback(PObject & PTRACE_PARAM(from), INT extra)
{
  PTRACE(extra < 0 ? 2 : 4, "ModemEngineBody::OnEngineCallback "
      << from.GetClass() << " " << EngineBase::ModemCallbackParam(extra)
      << " (" << seq << ", " << state << ")");

  PWaitAndSignal mutexWait(Mutex);

  if (extra == seq) {
    switch (state) {
      case stSendAckWait:
        SetState(stSendAckHandle);
        timeout.Stop();
        break;
      case stRecvBegWait:
        SetState(stRecvBegHandle);
        timeout.Stop();
        break;
      case stConnectHandle:
        if (subState == chWaitPlayTone) {
          SetSubState(chTonePlayed);
          timeout.Stop();
        }
        break;
      default:
        break;
    }
  }
  else
  switch (extra) {
    case EngineBase::cbpOutBufEmpty:
      switch (state) {
        case stSend:
          SetState(stSendBufEmptyHandle);
          break;
        default:
          break;
      }
      break;
    case EngineBase::cbpReset:
    case EngineBase::cbpOutBufNoFull:
    case EngineBase::cbpUpdateState:
    case EngineBase::cbpUserInput:
      break;
    default:
      myPTRACE(1, "ModemEngineBody::OnEngineCallback extra(" << extra << ") != seq(" << seq << ")");
  }

  parent.SignalDataReady();
}

void ModemEngineBody::OnTimerCallback(PObject & PTRACE_PARAM(from), INT PTRACE_PARAM(extra))
{
  PTRACE(2, "ModemEngineBody::OnTimerCallback " << state << " " << from.GetClass() << " " << extra);

  parent.SignalDataReady();
}

static int ParseNum(const char **ppCmd,
  PINDEX minDigits = 1, PINDEX maxDigits = 3, int maxNum = 255, int defNum = 0)
{
    const char *pEnd = *ppCmd;
    int num = 0;

    for( ; isdigit(*pEnd) ; pEnd++ ) {
      num = (num * 10) + (*pEnd - '0');
    }

    PINDEX len = PINDEX(pEnd - *ppCmd);
    *ppCmd = pEnd;

    if (len < minDigits || len > maxDigits || num > maxNum)
      return -1;
    else
    if (!len)
      return defNum;

    return num;
}

PBoolean ModemEngineBody::HandleClass1Cmd(const char **ppCmd, PString &resp, PBoolean &ok, PBoolean &crlf)
{
  PBoolean T;

  switch (*(*ppCmd - 2)) {
    case 'T':
      T = TRUE;
      break;
    case 'R':
      T = FALSE;
      break;
    default:
      return FALSE;
  }

  EngineBase::DataType dt;

  switch (*(*ppCmd - 1)) {
    case 'S':
      dt = EngineBase::dtSilence;
      break;
    case 'M':
      dt = EngineBase::dtRaw;
      break;
    case 'H':
      dt = EngineBase::dtHdlc;
      break;
    default:
      return FALSE;
  }

  if (dt == EngineBase::dtSilence) {
    switch( *(*ppCmd)++ ) {
      case '=':
        switch( **ppCmd ) {
          case '?':
            (*ppCmd)++;
            resp += "\r\n0-255";
            crlf = TRUE;
            break;
          default:
            if (P.ModemClassId() == EngineBase::mcFax) {
              int dms = ParseNum(ppCmd);
              if( dms >= 0 ) {
                ok = FALSE;

                PWaitAndSignal mutexWait(Mutex);

                if (T) {
                  if (!SendSilence(dms*10))
                    return FALSE;
                } else {
                  PBoolean done = FALSE;

                  timeout.Start(60000);
                  dataType = dt;
                  param = dms*10;
                  SetState(stRecvBegWait);
                  if (currentClassEngine && currentClassEngine->RecvWait(dataType, 0, NextSeq(), done)) {
                    if (!done)
                      break;

                    timeout.Stop();

                    if (!SendSilence(param))
                      return FALSE;
                  } else {
                    timeout.Stop();
                    SetState(stCommand);
                    return FALSE;
                  }
                }
              } else {
                return FALSE;
              }
            } else {
              return FALSE;
            }
        }
        break;
      default:
        return FALSE;
    }
  } else {
    switch( *(*ppCmd)++ ) {
      case '=':
        switch( **ppCmd ) {
          case '?':
            (*ppCmd)++;
            if (dt == EngineBase::dtRaw)
              resp += "\r\n24,48,72,73,74,96,97,98,121,122,145,146";
            else
              resp += "\r\n3" /*",24,48,72,73,74,96,97,98,121,122,145,146"*/;
            crlf = TRUE;
            break;
          default:
            if (P.ModemClassId() == EngineBase::mcFax) {
              int br = ParseNum(ppCmd);

              switch( br ) {
                case 3:
                  if (dt == EngineBase::dtRaw)
                    return FALSE;
                case 24:
                case 48:
                case 72:
                case 73:
                case 74:
                case 96:
                case 97:
                case 98:
                case 121:
                case 122:
                case 145:
                case 146:
                  ok = FALSE;
                  {
                    PString _resp;
                    PBoolean res = T ? SendStart(dt, br, _resp) : RecvStart(dt, br);

                    if (_resp.GetLength()) {
                      if (crlf) {
                        resp += "\r\n";
                        crlf = FALSE;
                      } else {
                        resp += RC_PREF();
                      }

                      resp += _resp;
                    }

                    if (!res)
                      PThread::Sleep(100);	// workaround
                    return res;
                  }
                default:
                  return FALSE;
              }
            } else {
              return FALSE;
            }
        }
        break;
      default:
        return FALSE;
    }
  }
  return TRUE;
}

PBoolean ModemEngineBody::HandleClass8Cmd(const char **ppCmd, PString &resp, PBoolean &ok, PBoolean &crlf)
{
  PBoolean T;

  switch (*(*ppCmd - 2)) {
    case 'T':
      T = TRUE;
      break;
    case 'R':
      T = FALSE;
      break;
    default:
      return FALSE;
  }

#define TONE_FREQUENCY_MIN  PDTMFEncoder::MinFrequency
#define TONE_FREQUENCY_MAX  (8000/4)
#define TONE_DMS_MAX        500
#define TONE_VOLUME         15

  switch (*(*ppCmd - 1)) {
    case 'S':
      switch (*(*ppCmd)++) {
        case '=':
          switch (**ppCmd) {
            case '?': {
              (*ppCmd)++;
              resp += PString(PString::Printf, "\r\n(0,%u-%u),(0,%u-%u),(0-%u)",
                              (unsigned)TONE_FREQUENCY_MIN, (unsigned)TONE_FREQUENCY_MAX,
                              (unsigned)TONE_FREQUENCY_MIN, (unsigned)TONE_FREQUENCY_MAX,
                              (unsigned)TONE_DMS_MAX);
              crlf = TRUE;
              break;
            }
            default:
              if (P.ModemClassId() == EngineBase::mcAudio) {
                ok = FALSE;

                PDTMFEncoder tone;

                for (;;) {
                  int dms = P.Vtd();

                  switch (**ppCmd) {
                    case '[': {
                      (*ppCmd)++;

                      int f1 = 0;
                      int f2 = 0;

                      f1 = ParseNum(ppCmd, 0, 5, TONE_FREQUENCY_MAX, f1);

                      if (f1 && f1 < TONE_FREQUENCY_MIN) {
                        myPTRACE(1, "Parse error: wrong f1 before " << *ppCmd);
                        return FALSE;
                      }

                      if (**ppCmd == ',') {
                        (*ppCmd)++;
                        f2 = ParseNum(ppCmd, 0, 5, TONE_FREQUENCY_MAX, f2);

                        if (f2 && f2 < TONE_FREQUENCY_MIN) {
                          myPTRACE(1, "Parse error: wrong f2 before " << *ppCmd);
                          return FALSE;
                        }

                        if (**ppCmd == ',') {
                          (*ppCmd)++;
                          dms = ParseNum(ppCmd, 0, 5, TONE_DMS_MAX, dms);

                          if (dms < 0) {
                            myPTRACE(1, "Parse error: wrong dms before " << *ppCmd);
                            return FALSE;
                          }
                        }
                      }

                      if (**ppCmd != ']') {
                        myPTRACE(1, "Parse error: no ']' before " << *ppCmd);
                        return FALSE;
                      }

                      if (dms) {
                        unsigned ms = dms*10;
                        char op;

                        if (f1 && f2) {
                          op = '+';
                        }
                        else
                        if (f1) {
                          op = '-';
                        }
                        else
                        if (f2) {
                          op = '-';
                          f1 = f2;
                          f2 = 0;
                        }
                        else {
                          op = ' ';
                        }

                        if (!tone.Generate(op, f1, f2, ms, TONE_VOLUME)) {
                          myPTRACE(1, "Cannot encode tone \"" << f1 << op << f2 << ":" << ms << "\"");
                          return FALSE;
                        }

                        myPTRACE(2, "Encoded tone \"" << f1 << op << f2 << ":" << ms << "\", size=" << tone.GetSize());
                      }

                      (*ppCmd)++;
                      break;
                    }
                    case '{': {
                      (*ppCmd)++;

                      char dtmf = **ppCmd;

                      if (isdigit(dtmf) || (dtmf >= 'A' && dtmf <= 'C') || dtmf == '*' || dtmf == '#')
                        (*ppCmd)++;
                      else
                        dtmf = ' ';

                      if (**ppCmd == ',') {
                        (*ppCmd)++;
                        dms = ParseNum(ppCmd, 0, 5, TONE_DMS_MAX, dms);

                        if (dms < 0) {
                          myPTRACE(1, "Parse error: wrong dms before " << *ppCmd);
                          return FALSE;
                        }
                      }

                      if (**ppCmd != '}') {
                        myPTRACE(1, "Parse error: no '}' before " << *ppCmd);
                        return FALSE;
                      }

                      if (dms) {
                        unsigned ms = dms*10;

                        if (dtmf == ' ') {
                          if (!tone.Generate(' ', 0, 0, ms)) {
                            myPTRACE(1, "Cannot encode tone \"0 0:" << ms << "\"");
                            return FALSE;
                          }

                          myPTRACE(2, "Encoded tone \"0 0:" << ms << "\", size=" << tone.GetSize());
                        } else {
                          tone.AddTone(dtmf, ms);
                          myPTRACE(2, "Encoded DTMF tone \"" << dtmf << ":" << ms << "\", size=" << tone.GetSize());
                        }
                      }

                      (*ppCmd)++;
                      break;
                    }
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case 'A':
                    case 'B':
                    case 'C':
                    case 'D':
                    case '*':
                    case '#': {
                      char dtmf = *(*ppCmd)++;
                      unsigned ms = dms*10;

                      tone.AddTone(dtmf, ms);
                      myPTRACE(2, "Encoded DTMF tone \"" << dtmf << ":" << ms << "\", size=" << tone.GetSize());
                      break;
                    }
                    case ',': {
                      unsigned ms = dms*10;

                      if (!tone.Generate(' ', 0, 0, ms)) {
                        myPTRACE(1, "Cannot encode tone \"0 0:" << ms << "\"");
                        return FALSE;
                      }

                      myPTRACE(2, "Encoded tone \"0 0:" << ms << "\", size=" << tone.GetSize());
                      break;
                    }
                    default:
                      break;
                  }

                  if (**ppCmd != ',')
                    break;

                  (*ppCmd)++;
                }

                PWaitAndSignal mutexWait(Mutex);
                dataType = EngineBase::dtRaw;
                moreFrames = FALSE;
                SetState(stSend);
                if (currentClassEngine && currentClassEngine->SendStart(dataType, 0)) {
                  PINDEX len = tone.GetSize();

                  if (len) {
                    const PInt16 *ps = tone.GetPointer();
                    currentClassEngine->Send(ps, len*sizeof(*ps));
                  }

                  SetState(stSendAckWait);

                  if (!currentClassEngine->SendStop(FALSE, NextSeq())) {
                    SetState(stCommand);
                    return FALSE;
                  }
                } else {
                  SetState(stCommand);
                  return FALSE;
                }
              } else {
                return FALSE;
              }
          }
          break;
        default:
          return FALSE;
      }
      break;
    case 'X':
      if (P.ModemClassId() == EngineBase::mcAudio) {
        ok = FALSE;

        PString _resp;
        PBoolean res = T ? SendStart(EngineBase::dtRaw, 0, _resp) : RecvStart(EngineBase::dtRaw, 0);

        if (_resp.GetLength()) {
          if (crlf) {
            resp += "\r\n";
            crlf = FALSE;
          } else {
            resp += RC_PREF();
          }

          resp += _resp;
        }

        if (!res)
          PThread::Sleep(100);	// workaround

        return res;
      } else {
        return FALSE;
      }
    default:
      return FALSE;
  }

  return TRUE;
}

PBoolean ModemEngineBody::Answer()
{
  OffHook();
  timerRing.Stop();
  forceFaxMode = TRUE;

  if (!connectionEstablished) {
    PBoolean ok;

    if (callState == cstCalled) {
      SetCallState(cstAnswering);

      SetState(stConnectWait);
      timeout.Start(EstablishmentTimeout());

      PStringToString request;
      request.SetAt("modemtoken", parent.modemToken());
      request.SetAt("command", "answer");
      request.SetAt("calltoken", CallToken());

      Mutex.Signal();
      callbackEndPoint(request, 2);
      Mutex.Wait();

      ok = (request("response") == "confirm");
    } else {
      ok = FALSE;
    }

    if (!ok) {
      OnHook();

      return FALSE;
    }
  } else {
    timeout.Stop();
    SetState(stConnectHandle, chConnectionEstablished);
    parent.SignalDataReady();
  }

  return TRUE;
}

void ModemEngineBody::HandleCmd(PString &resp)
{
  PINDEX i;

  for (i = 0 ;  ; i++) {
    i = cmd.FindOneOf("Aa", i);

    if (i == P_MAX_INDEX) {
      myPTRACE(1, "--> " << cmd.GetLength() << " bytes of binary");
      cmd.MakeEmpty();
      return;
    }

    PString at = cmd.Mid(i, 2);

    if (at == "AT" || at == "at")
      break;
  }

#if PTRACING
  if (i) {
    PBYTEArray bin((const BYTE *)(const char *)cmd, i);

    myPTRACE(1, "--> " << PRTHEX(bin));
  }
#endif

  const char *pCmd;

  pCmd = ((const char *)cmd) + i;

  myPTRACE(1, "--> " << pCmd);

  pCmd += 2;  // skip AT

  cmdRest = PString(pCmd).ToUpper();
  cmd.MakeEmpty();

  HandleCmdRest(resp);
}

#define ToSBit(funk)                    \
  switch (ParseNum(&pCmd, 0, 1, 1)) {   \
    case 0:                             \
      P.funk(FALSE);                    \
      break;                            \
    case 1:                             \
      P.funk(TRUE);                     \
      break;                            \
    default:                            \
      err = TRUE;                       \
  }

void ModemEngineBody::HandleCmdRest(PString &resp)
{
  PString tmp = cmdRest;
  cmdRest.MakeEmpty();

  const char *pCmd = tmp;
  PBoolean err = FALSE;
  PBoolean ok = TRUE;
  PBoolean crlf = FALSE;

  while (state == stCommand && !err && *pCmd) {
      switch( *pCmd++ ) {
        case ' ':
        case ';':
          break;
        case 'A':	// Accept incoming call
          ok = FALSE;

          {
            PWaitAndSignal mutexWait(Mutex);

            callDirection = cdIncoming;
            if (!Answer())
              err = TRUE;
          }

          break;
        case 'B':       // Turn ITU-T V.22/BELL 212A
          if (ParseNum(&pCmd, 0, 1) >= 0) {
          } else {
            err = TRUE;
          }
          break;
        case 'D':	// Dial
          ok = FALSE;

          {
            PWaitAndSignal mutexWait(Mutex);

            PBoolean wasOnHook = OffHook();

            PString num;
            PString numTone;
            PBoolean addNumTone;
            PString LocalPartyName;
            PBoolean local = FALSE;
            PBoolean setForceFaxMode = FALSE;
            CallDirection setCallDirection = cdOutgoing;

            if (!CallToken().IsEmpty()) {
              addNumTone = TRUE;
            } else {
              addNumTone = FALSE;

              if (pPlayTone) {
                myPTRACE(1, "ModemEngineBody::HandleCmd pPlayTone is not NULL");
                delete pPlayTone;
                pPlayTone = NULL;
              }
            }

            while (!err) {
              char ch = *pCmd++;

              if (ch == 0) {
                pCmd--;
                break;
              }

              if (ch == ';') {
                setCallDirection = cdUndefined;
                cmdRest = PString(pCmd);
                break;
              }

              switch (ch) {
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                case '8':
                case '9':
                case '*':
                case '#':
                  if (local) {
                    if (!CallToken().IsEmpty()) {
                      // Dialing of calling number is not
                      // allowed in online command state
                      err = TRUE;
                      continue;
                    }
                    LocalPartyName += ch;
                    continue;
                  }
                  break;
                case '+':
                  if (local)
                    continue;
                  if (addNumTone)
                    continue;
                  if (!num.IsEmpty())
                    continue;
                  break;
                case 'A':
                case 'B':
                case 'C':
                  if (local)
                    continue;
                  break;
                case 'D':
                  if (local) {
                    local = FALSE;
                    continue;
                  }
                  break;
                case ',':
                  if (!addNumTone)
                    continue;
                  break;
                case 'F':
                  setForceFaxMode = TRUE;
                  continue;
                case 'V':
                  setForceFaxMode = FALSE;
                  continue;
                case 'L':
                  LocalPartyName = "";
                  local = TRUE;
                  continue;
                case 'T':
                case 'P':
                  local = FALSE;
                  continue;
                case '@':
                  local = FALSE;
                  addNumTone = TRUE;
                  numTone.MakeEmpty();
                  continue;
                default:
                  // V.250
                  // Any characters appearing in the dial string that
                  // the DCE does not recognize ... shall be ignored.
                  continue;
              }

              if (!addNumTone)
                num += ch;
              else
                numTone += ch;
            }

            myPTRACE(2, "Dial string: " << num << "@" << numTone << "L" << LocalPartyName << (setForceFaxMode ? "F" : "V") << (err ? "" : " - OK"));

            if (!err) {
              PDTMFEncoder playTone;

              for (PINDEX i = 0 ; i < numTone.GetLength() ; i++) {
                char ch = numTone[i];

                switch (ch) {
                  case ',': {
                    unsigned ms = unsigned(P.DialTimeComma()) * 1000;

                    playTone.Generate(' ', 0, 0, ms);
                    myPTRACE(2, "Encoded tone \"0 0:" << ms << "\", size=" << playTone.GetSize());
                    break;
                  }
                  default: {
                    unsigned ms = P.DialTimeDTMF();

                    playTone.AddTone(ch, ms);
                    myPTRACE(2, "Encoded DTMF tone \"" << ch << ":" << ms << "\", size=" << playTone.GetSize());
                    playTone.Generate(' ', 0, 0, ms);
                    myPTRACE(2, "Encoded tone \"0 0:" << ms << "\", size=" << playTone.GetSize());
                    break;
                  }
                }
              }

              if (!CallToken().IsEmpty()) {
                if (connectionEstablished) {
                  callDirection = setCallDirection;
                  forceFaxMode = (forceFaxMode || setForceFaxMode);
                  SetState(stConnectHandle, chConnected);
                  parent.SignalDataReady();

                  if (numTone.GetLength()) {
                    if (!pPlayTone)
                      pPlayTone = new PDTMFEncoder();

                    pPlayTone->Concatenate(playTone);
                  }

                  break;
                }

                OnHook();

                if (crlf) {
                  resp += "\r\n";
                  crlf = FALSE;
                } else {
                  resp += RC_PREF();
                }

                resp += RC_BUSY();
                break;
              }

              callDirection = setCallDirection;
              forceFaxMode = setForceFaxMode;

              timerRing.Stop();
              SetState(stDial);

              if (numTone.GetLength()) {
                if (!pPlayTone)
                  pPlayTone = new PDTMFEncoder();

                pPlayTone->Concatenate(playTone);
              }

              timeout.Start(EstablishmentTimeout());

              params.RemoveAll();
              params.SetAt("number", num);
              params.SetAt("localpartyname", LocalPartyName);

              parent.SignalDataReady();  // try to Dial w/o delay
            } else {
              if (wasOnHook)
                OnHook();
            }
          }
          break;
        case 'E': {	// Turn Echo on/off
          PWaitAndSignal mutexWait(Mutex);
          ToSBit(Echo);
          break;
        }
        case 'H':	// On/Off-hook
          if( ParseNum(&pCmd, 0, 1, 0) >= 0 ) {	// ATH & ATH0
            PWaitAndSignal mutexWait(Mutex);

            P.ModemClass("1");
            OnChangeModemClass();

            if (off_hook || P.ClearMode())
              OnHook();
          } else {
            err = TRUE;
          }
          break;
        case 'I':	// Information
          {
            int val = ParseNum(&pCmd, 0, 1);

            switch (val) {
              case 0:
                resp += "\r\n" + PString(Model);
                crlf = TRUE;
                break;
              case 3:
                resp += "\r\n" + PString(Manufacturer);
                crlf = TRUE;
                break;
              case 8:
                resp += "\r\nNMBR = " + SrcNum();
                crlf = TRUE;
                break;
              case 9:
                resp += "\r\nNDID = " + DstNum();
                crlf = TRUE;
                break;
              default:
                if( val < 0 )
                  err = TRUE;
            }
          }
          break;
        case 'L':	// Turn Speaker on/off
          if( ParseNum(&pCmd, 0, 1) >= 0 ) {
          } else {
            err = TRUE;
          }
          break;
        case 'M':	// Turn Speaker on/off
          if( ParseNum(&pCmd, 0, 1) >= 0 ) {
          } else {
            err = TRUE;
          }
          break;
        case 'N':	// Ring volume
          if( ParseNum(&pCmd, 0, 1) >= 0 ) {
          } else {
            err = TRUE;
          }
          break;
        case 'O':	// Go online
          err = TRUE;
          break;
        case 'Q': {	// Turn result codes on/off
          PWaitAndSignal mutexWait(Mutex);
          ToSBit(noResultCodes);
          break;
        }
        case 'S':	// Set/Get Register
          {
          int r = ParseNum(&pCmd);

          if (r >= 0) {
            switch (*pCmd++) {
              case '=':
                {
                  int val = ParseNum(&pCmd);

                  PWaitAndSignal mutexWait(Mutex);

                  if( val < 0 || !P.SetReg(r, (BYTE)val) ) {
                    err = TRUE;
                  }
                }
                break;
              case '?':
                {
                  BYTE val;

                  PWaitAndSignal mutexWait(Mutex);

                  if (P.GetReg(r, val)) {
                    resp.sprintf("\r\n%3.3u", (unsigned)val);
                    crlf = TRUE;
                  } else {
                    err = TRUE;
                  }
                }
                break;
              case '.':
                {
                  int b = ParseNum(&pCmd, 1, 1, 7);

                  if (b >= 0) {
                    switch (*pCmd++) {
                      case '=':
                        {
                          int val = ParseNum(&pCmd, 1, 1, 1);
                          if( val < 0 || !P.SetBit(r, b, (PBoolean)val) ) {
                            err = TRUE;
                          }
                        }
                        break;
                      case '?':
                        {
                          PBoolean val;

                          if (P.GetBit(r, b, val)) {
                            resp.sprintf("\r\n%u", (unsigned)val);
                            crlf = TRUE;
                          } else {
                            err = TRUE;
                          }
                        }
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                }
                break;
              default:
                err = TRUE;
            }
          } else {
            err = TRUE;
          }
          }
          break;
        case 'V': {	// Numeric or ASCII result codes
          PWaitAndSignal mutexWait(Mutex);
          ToSBit(asciiResultCodes);
          break;
        }
        case 'X':	// Which result codes
          if (ParseNum(&pCmd, 0, 1) >= 3) {
          } else {
            err = TRUE;
          }
          break;
        case 'Z':	// Load Registers from Profile
          {
            int val = ParseNum(&pCmd, 0, 1, sizeof(Profiles)/sizeof(Profiles[0]) - 1);
            if( val >= 0 ) {
              PWaitAndSignal mutexWait(Mutex);

              P = Profiles[val];
              OnChangeModemClass();

              if (off_hook)  // clear call only in off-hook state
                OnHook();
            } else {
              err = TRUE;
            }
          }
          break;
        case '+':
          switch( *pCmd++ ) {
            case 'F':	// FAX
              switch( *pCmd++ ) {
                case 'A':
                  if (*pCmd == 'A') {			// +FAA
                    pCmd += 1;
                    switch( *pCmd++ ) {
                      case '=':
                        switch( *pCmd ) {
                          case '?':
                            pCmd++;
                            resp += "\r\n0";
                            crlf = TRUE;
                            break;
                          default:
                            switch( ParseNum(&pCmd) ) {
                              case 0:
                                break;
                              default:
                                err = TRUE;
                            }
                        }
                        break;
                      case '?':
                        resp += "\r\n0";
                        crlf = TRUE;
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                  break;
                case 'C':
                  if( strncmp(pCmd, "LASS", 4) == 0 ) {	// +FCLASS
                    pCmd += 4;
                    switch( *pCmd++ ) {
                      case '=':
                        switch( *pCmd ) {
                          case '?':
                            pCmd++;
                            resp += "\r\n1,8";
                            crlf = TRUE;
                            break;
                          default: {
                            PWaitAndSignal mutexWait(Mutex);
                            const char *modemClass;

                            switch (ParseNum(&pCmd)) {
                              case 0:
                                modemClass = "0";
                                break;
                              case 1:
                                modemClass = "1";
                                break;
                              case 8:
                                modemClass = "8";
                                break;
                              default:
                                modemClass = NULL;
                                err = TRUE;
                            }

                            if (modemClass) {
                              P.ModemClass(modemClass);
                              OnChangeModemClass();
                            }
                          }
                        }
                        break;
                      case '?':
                        resp += "\r\n" + P.ModemClass();
                        crlf = TRUE;
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                  break;
                case 'L':
                  switch (*pCmd++) {
                    case 'O':				// +FLO
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n0-2";
                              crlf = TRUE;
                              break;
                            default:
                              {
                                int val = ParseNum(&pCmd);
                                switch (val) {
                                  case 0:
                                  case 1:
                                  case 2: {
                                    PWaitAndSignal mutexWait(Mutex);
                                    P.Flo((BYTE)val);
                                    break;
                                  }
                                  default:
                                    err = TRUE;
                                }
                              }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u", (unsigned)P.Flo());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'M':
                  switch (*pCmd++) {
                    case 'D':
                      switch (*pCmd++) {
                        case 'L':			// +FMDL
                          switch (*pCmd++) {
                            case '?':
                              resp += "\r\n" + PString(Model);
                              crlf = TRUE;
                              break;
                            default:
                              err = TRUE;
                          }
                          break;
                      default:
                          err = TRUE;
                      }
                      break;
                    case 'F':
                      switch (*pCmd++) {
                        case 'R':			// +FMFR
                          switch (*pCmd++) {
                            case '?':
                              resp += "\r\n" + PString(Manufacturer);
                              crlf = TRUE;
                              break;
                            default:
                              err = TRUE;
                          }
                          break;
                      default:
                          err = TRUE;
                      }
                      break;
                    case 'I':				// +FMI
                      switch (*pCmd++) {
                        case '?':
                          resp += "\r\n" + PString(Manufacturer);
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'M':				// +FMM
                      switch (*pCmd++) {
                        case '?':
                          resp += "\r\n" + PString(Model);
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'R':				// +FMR
                      switch (*pCmd++) {
                        case '?':
                          resp += "\r\n" + PString(Revision);
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'R':
                  switch( *pCmd ) {
                    case 'M':				// +FRM
                    case 'H':				// +FRH
                    case 'S':				// +FRS
                      pCmd++;
                      if (!HandleClass1Cmd(&pCmd, resp, ok, crlf))
                        err = TRUE;
                      break;
                    default:
                      if( strncmp(pCmd, "EV", 2) == 0 ) {	// +FREV
                        pCmd += 2;
                        switch( *pCmd++ ) {
                          case '?':
                            resp += "\r\n" + PString(Revision);
                            crlf = TRUE;
                            break;
                          default:
                            err = TRUE;
                        }
                      } else {
                        err = TRUE;
                      }
                  }
                  break;
                case 'T':
                  switch( *pCmd ) {
                    case 'M':				// +FTM
                    case 'H':				// +FTH
                    case 'S':				// +FTS
                      pCmd++;
                      if (!HandleClass1Cmd(&pCmd, resp, ok, crlf))
                        err = TRUE;
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                default:
                  err = TRUE;
              }
              break;
            case 'I':	// DTE-DCE interface commands
              switch (*pCmd++) {
                case 'F':
                  switch (*pCmd++) {
                    case 'C':				// +IFC
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n+IFC:(0-2),(0-2)";
                              crlf = TRUE;
                              break;
                            default: {
                              int valByDTE = ParseNum(&pCmd, 1, 1, 2);

                              if (valByDTE < 0) {
                                err = TRUE;
                                break;
                              }

                              int valByDCE;

                              if (*pCmd == ',') {
                                pCmd++;

                                valByDCE = ParseNum(&pCmd);

                                if (valByDCE < 0) {
                                  err = TRUE;
                                  break;
                                }
                              } else {
                                valByDCE = P.IfcByDCE();
                              }

                              PWaitAndSignal mutexWait(Mutex);

                              P.IfcByDTE((BYTE)valByDTE);
                              P.IfcByDCE((BYTE)valByDCE);
                            }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n+IFC:%u,%u", (unsigned)P.IfcByDTE(), (unsigned)P.IfcByDCE());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                default:
                  err = TRUE;
              }
              break;
            case 'V':	// Voice
              switch (*pCmd++) {
                case 'C':
                  if (strncmp(pCmd, "ID", 2) == 0) {	// +VCID
                    pCmd += 2;
                    switch (*pCmd++) {
                      case '=':
                        switch (*pCmd) {
                          case '?':
                            pCmd++;
                            resp += "\r\n(0,1)";
                            crlf = TRUE;
                            break;
                          default:
                            {
                              int val = ParseNum(&pCmd);
                              switch (val) {
                                case 0:
                                case 1: {
                                  PWaitAndSignal mutexWait(Mutex);
                                  P.CidMode((BYTE)val);
                                  break;
                                }
                                default:
                                  err = TRUE;
                              }
                            }
                        }
                        break;
                      case '?':
                        resp.sprintf("\r\n%u", (unsigned)P.CidMode());
                        crlf = TRUE;
                        break;
                      default:
                        err = TRUE;
                    }
                  } else {
                    err = TRUE;
                  }
                  break;
                case 'E':
                  switch (*pCmd++) {
                    case 'M':				// +VEM
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n(0)";
                              crlf = TRUE;
                              break;
                            default:
                              if (ParseNum(&pCmd) != 0)
                                err = TRUE;
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n0");
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'G':
                  switch (*pCmd++) {
                    case 'R':				// +VGR
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n(0-255)";
                              crlf = TRUE;
                              break;
                            default:
                              {
                                int val = ParseNum(&pCmd);

                                if (val >= 0) {
                                  PWaitAndSignal mutexWait(Mutex);
                                  P.VgrInterval((BYTE)val);
                                } else {
                                  err = TRUE;
                                }
                              }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u", (unsigned)P.VgrInterval());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'T':				// +VGT
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n(0-255)";
                              crlf = TRUE;
                              break;
                            default:
                              {
                                int val = ParseNum(&pCmd);

                                if (val >= 0) {
                                  PWaitAndSignal mutexWait(Mutex);
                                  P.VgtInterval((BYTE)val);
                                } else {
                                  err = TRUE;
                                }
                              }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u", (unsigned)P.VgtInterval());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'I':
                  switch (*pCmd++) {
                    case 'P':				// +VIP
                      {
                        int val;

                        if (*pCmd == '=') {
                          pCmd++;
                          if (*pCmd == '?') {
                            pCmd++;
                            resp.sprintf("\r\n(0-%u)", sizeof(Profiles)/sizeof(Profiles[0]) - 1);
                            crlf = TRUE;
                            break;
                          }

                          val = ParseNum(&pCmd);

                          if( val < 0 || val > ((int)(sizeof(Profiles)/sizeof(Profiles[0])) - 1)) {
                            err = TRUE;
                            break;
                          }
                        } else {
                          val = 0;
                        }

                        PWaitAndSignal mutexWait(Mutex);
                        P.SetVoiceProfile(Profiles[val]);
                      }
                      break;
                    case 'T':				// +VIT
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n(0)";
                              crlf = TRUE;
                              break;
                            default:
                              if (ParseNum(&pCmd) < 0)
                                err = TRUE;
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n0");
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'L':
                  switch (*pCmd++) {
                    case 'S':				// +VLS
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n0,\"\",00000000,00000000,00000000";
                              resp += "\r\n1,\"T\",00000000,00000000,00000000";
                              resp += "\r\n5,\"ST\",00000000,00000000,00000000";
                              resp += "\r\n7,\"MST\",00000000,00000000,00000000";
                              crlf = TRUE;
                              break;
                            default:
                              switch (ParseNum(&pCmd)) {
                                case 0: {
                                  PWaitAndSignal mutexWait(Mutex);

                                  if (off_hook || P.ClearMode())
                                    OnHook();
                                  break;
                                }
                                case 1:
                                case 5:
                                case 7:
                                  ok = FALSE;

                                  {
                                    PWaitAndSignal mutexWait(Mutex);

                                    callDirection = cdUndefined;
                                    if (!Answer())
                                      err = TRUE;
                                  }
                                  break;
                                default:
                                  err = TRUE;
                              }
                          }
                          break;
                        case '?':
                          resp += off_hook ? "\r\n1" : "\r\n0";
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'R':
                  switch (*pCmd++) {
                    case 'A':				// +VRA
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n(0-255)";
                              crlf = TRUE;
                              break;
                            default:
                              {
                                int val = ParseNum(&pCmd);

                                if (val >= 0) {
                                  PWaitAndSignal mutexWait(Mutex);
                                  P.VraInterval((BYTE)val);
                                } else {
                                  err = TRUE;
                                }
                              }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u", (unsigned)P.VraInterval());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'N':				// +VRN
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n(0-255)";
                              crlf = TRUE;
                              break;
                            default:
                              {
                                int val = ParseNum(&pCmd);

                                if (val >= 0) {
                                  PWaitAndSignal mutexWait(Mutex);
                                  P.VrnInterval((BYTE)val);
                                } else {
                                  err = TRUE;
                                }
                              }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u", (unsigned)P.VrnInterval());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'X':				// +VRX
                      if (!HandleClass8Cmd(&pCmd, resp, ok, crlf))
                        err = TRUE;
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'S':
                  switch (*pCmd++) {
                    case 'D':				// +VSD
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n(0-255),(0-255)";
                              crlf = TRUE;
                              break;
                            default:
                              {
                                int sds = ParseNum(&pCmd);

                                if (sds < 0 || *pCmd != ',') {
                                  err = TRUE;
                                  break;
                                }

                                pCmd++;

                                int sdi = ParseNum(&pCmd);

                                if (sdi < 0) {
                                  err = TRUE;
                                  break;
                                }

                                PWaitAndSignal mutexWait(Mutex);

                                P.Vsds((BYTE)sds);
                                P.Vsdi((BYTE)sdi);
                              }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u,%u", (unsigned)P.Vsds(), (unsigned)P.Vsdi());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'M':				// +VSM
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n0,\"SIGNED PCM\",8,0,(8000),(0),(0)";
                              resp += "\r\n1,\"UNSIGNED PCM\",8,0,(8000),(0),(0)";
                              resp += "\r\n4,\"G.711U\",8,0,(8000),(0),(0)";
                              resp += "\r\n5,\"G.711A\",8,0,(8000),(0),(0)";
                              resp += "\r\n128,\"8-BIT LINEAR\",8,0,(8000),(0),(0)";
                              resp += "\r\n129,\"ADPCM (NOT IMPLEMENTED)\",0,0,(0),(0),(0)";
                              resp += "\r\n130,\"UNSIGNED PCM\",8,0,(8000),(0),(0)";
                              resp += "\r\n131,\"G.711 ULAW\",8,0,(8000),(0),(0)";
                              resp += "\r\n132,\"G.711 ALAW\",8,0,(8000),(0),(0)";
                              crlf = TRUE;
                              break;
                            default: {
                              int cml = ParseNum(&pCmd);

                              switch (cml) {
                                case 0:
                                case 1:
                                case 4:
                                case 5:
                                case 128:
                                case 130:
                                case 131:
                                case 132:
                                  break;
                                default:
                                  err = TRUE;
                              }

                              if (err)
                                break;

                              if (*pCmd == ',') {
                                pCmd++;

                                int vsr = ParseNum(&pCmd, 4, 4, 8000);

                                if (vsr != 8000) {
                                  err = TRUE;
                                  break;
                                }

                                int scs = 0;
                                int sel = 0;

                                if (*pCmd == ',') {
                                  pCmd++;
                                  scs = ParseNum(&pCmd);

                                  if (scs != 0) {
                                    err = TRUE;
                                    break;
                                  }

                                  if (*pCmd == ',') {
                                    pCmd++;
                                    sel = ParseNum(&pCmd);

                                    if (sel != 0) {
                                      err = TRUE;
                                      break;
                                    }
                                  }
                                }
                              }

                              PWaitAndSignal mutexWait(Mutex);
                              P.Vcml((BYTE)cml);
                            }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u,8000,0,0", (unsigned)P.Vcml());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                case 'T':
                  switch (*pCmd++) {
                    case 'D':				// +VTD
                      switch (*pCmd++) {
                        case '=':
                          switch (*pCmd) {
                            case '?':
                              pCmd++;
                              resp += "\r\n(0-255)";
                              crlf = TRUE;
                              break;
                            default:
                              {
                                int val = ParseNum(&pCmd);

                                if (val >= 0) {
                                  PWaitAndSignal mutexWait(Mutex);
                                  P.Vtd((BYTE)val);
                                } else {
                                  err = TRUE;
                                }
                              }
                          }
                          break;
                        case '?':
                          resp.sprintf("\r\n%u", (unsigned)P.Vtd());
                          crlf = TRUE;
                          break;
                        default:
                          err = TRUE;
                      }
                      break;
                    case 'X':				// +VTX
                    case 'S':				// +VTS
                      if (!HandleClass8Cmd(&pCmd, resp, ok, crlf))
                        err = TRUE;
                      break;
                    default:
                      err = TRUE;
                  }
                  break;
                default:
                  err = TRUE;
              }
              break;
            default:
              err = TRUE;
          }
          break;
        case '&':
          switch( *pCmd++ ) {
            case 'C':					// &C
              {
                int val = ParseNum(&pCmd, 0, 1, 1);
                if( val >= 0 ) {
                } else {
                  err = TRUE;
                }
              }
              break;
            case 'D':					// &D
              {
                int val = ParseNum(&pCmd, 0, 1, 3);
                if( val >= 0 ) {
                } else {
                  err = TRUE;
                }
              }
              break;
            case 'F':					// &F
              {
                PWaitAndSignal mutexWait(Mutex);

                P = Profiles[0];
                OnChangeModemClass();

                if (off_hook)  // clear call only in off-hook state
                  OnHook();
              }
              break;
            case 'H':					// &H
              {
                int val = ParseNum(&pCmd, 0, 1, 7);
                if( val >= 0 ) {
                  PWaitAndSignal mutexWait(Mutex);
                  P.SetBits(27, 3, 5, (BYTE)val);
                } else {
                  err = TRUE;
                }
              }
              break;
            default:
              err = TRUE;
          }
          break;
        case '#':
          if (strncmp(pCmd, "CID", 3) == 0) {		// #CID
            pCmd += 3;
            switch( *pCmd++ ) {
              case '=':
                switch( *pCmd ) {
                  case '?':
                    pCmd++;
                    resp += "\r\n(0,1,10,11)";
                    crlf = TRUE;
                    break;
                  default:
                    {
                      int val = ParseNum(&pCmd);

                      PWaitAndSignal mutexWait(Mutex);

                      switch (val) {
                        case 0:
                          P.CidMode(0);
                          P.DidMode(0);
                          break;
                        case 1:
                          P.CidMode(1);
                          P.DidMode(0);
                          break;
                        case 10:
                          P.CidMode(1);
                          P.DidMode(1);
                          break;
                        case 11:
                          P.CidMode(0);
                          P.DidMode(1);
                          break;
                        default:
                          err = TRUE;
                      }
                    }
                }
                break;
              case '?': {
                unsigned cid;

                if (P.CidMode()) {
                  if (P.DidMode()) {
                    cid = 10;
                  } else {
                    cid = 1;
                  }
                } else {
                  if (P.DidMode()) {
                    cid = 11;
                  } else {
                    cid = 0;
                  }
                }

                resp.sprintf("\r\n%u", P.CidMode());
                crlf = TRUE;
                break;
              }
              default:
                err = TRUE;
            }
          } else
          if (strncmp(pCmd, "DFRMC", 5) == 0) {         // #DFRMC
            pCmd += 5;
            switch( *pCmd++ ) {
              case '=':
                switch( *pCmd ) {
                  case '?':
                    pCmd++;
                    resp += "\r\n(0-255)";
                    crlf = TRUE;
                    break;
                  default:
                    {
                      int val = ParseNum(&pCmd);
                      if (val >= 0) {
                        PWaitAndSignal mutexWait(Mutex);
                        P.DelayFrmConnect((BYTE)val);
                      } else {
                        err = TRUE;
                      }
                    }
                }
                break;
              case '?':
                resp.sprintf("\r\n%u", (unsigned)P.DelayFrmConnect());
                crlf = TRUE;
                break;
              default:
                err = TRUE;
            }
          } else
          if (strncmp(pCmd, "HCLR", 4) == 0) {         // #HCLR
            pCmd += 4;
            switch( *pCmd++ ) {
              case '=':
                switch( *pCmd ) {
                  case '?':
                    pCmd++;
                    resp += "\r\n(0,1)";
                    crlf = TRUE;
                    break;
                  default:
                    {
                      int val = ParseNum(&pCmd);
                      switch (val) {
                        case 0:
                        case 1: {
                          PWaitAndSignal mutexWait(Mutex);
                          P.ClearMode((BYTE)val);
                          break;
                        }
                        default:
                          err = TRUE;
                      }
                    }
                }
                break;
              case '?':
                resp.sprintf("\r\n%u", (unsigned)P.ClearMode());
                crlf = TRUE;
                break;
              default:
                err = TRUE;
            }
          } else {
            err = TRUE;
          }
          break;
        default:
          err = TRUE;
      }
  }

  if (crlf)
    resp += "\r\n";

  if (err) {
    if (!crlf)
      resp += RC_PREF();
    resp += RC_ERROR();
  }
  else
  if (ok) {
    if (!crlf)
      resp += RC_PREF();
    resp += RC_OK();
  }
}

void ModemEngineBody::HandleData(const PBYTEArray &buf, PBYTEArray &bresp)
{
    int len = buf.GetSize();
    const BYTE *pBuf = buf;

    while (len > 0) {
      switch (state) {
        case stCommand:
          if (!off_hook) {
            PWaitAndSignal mutexWait(Mutex);
            lastOnHookActivity = PTime();
          }

          while (state == stCommand && len > 0) {
            const BYTE *pEnd = (const BYTE *)memchr(pBuf, '\r', len);

            if (pEnd == NULL) {
              cmd += PString((const char *)pBuf, len);
              if( Echo() )
                bresp.Concatenate(PBYTEArray(pBuf, len));
              len = 0;
            } else {
              int rlen = int(pEnd - pBuf);
              if( rlen ) {
                cmd += PString((const char *)pBuf, rlen);
                if( Echo() ) {
                  bresp.Concatenate(PBYTEArray(pBuf, rlen));
                }
                len -= rlen;
                pBuf += rlen;
              }
              len--;
              pBuf++;

              if (Echo())
                bresp.Concatenate(PBYTEArray((const BYTE *)"\r", 1));

              PString resp;

              HandleCmd(resp);

              if (resp.GetLength()) {
                PBYTEArray _bresp((const BYTE *)(const char *)resp, resp.GetLength());

                myPTRACE(1, "<-- " << PRTHEX(_bresp));
                bresp.Concatenate(_bresp);
              }
            }
          }
          break;
        case stSend:
          {
            int lendone = dleData.PutDleData(pBuf, len);

            if (lendone > 0) {
                PTRACE(4, "--> DLE " << lendone << " bytes");

                if (Echo())
                  bresp.Concatenate(PBYTEArray(pBuf, lendone));
                len -= lendone;
                pBuf += lendone;
            }

            int dt = dataType;
            BYTE Buf[1024];

            for(;;) {
              int count = dleData.GetData(Buf, sizeof(Buf));

              PWaitAndSignal mutexWait(Mutex);
              switch( count ) {
                case -1:
                  SetState(stSendAckWait);
                  if (!currentClassEngine || !currentClassEngine->SendStop(moreFrames, NextSeq())) {
                    SetState(stSendAckHandle);
                    timeout.Stop();
                    parent.SignalDataReady();  // try to SendAckHandle w/o delay
                  }
                  break;
                case 0:
                  break;
                default:
                  switch( dt ) {
                    case EngineBase::dtHdlc:
                      if (dataCount < 2 && (dataCount + count) >= 2 &&
                          (Buf[1 - dataCount] & 0x08) == 0)
                      {
                        moreFrames = TRUE;
                      }
                  }
                  dataCount += count;
                  if (P.ModemClassId() == EngineBase::mcAudio) {
                    if (currentClassEngine) {
                      const signed char *pb = (const signed char *)Buf;
                      PInt16 Buf2[sizeof(Buf)];
                      PInt16 *ps = Buf2;

                      switch (P.Vcml()) {
                        case 0:
                          while (count--)
                            *ps++ = (PInt16)((PInt16)(*pb++)*256);
                          break;
                        case 1:
                        case 128:
                        case 130:
                          while (count--)
                            *ps++ = (PInt16)((PInt16)(*pb++)*256 - 0x8000);
                          break;
                        case 4:
                        case 131:
                          while (count--)
                            *ps++ = (PInt16)ulaw2linear(*pb++);
                          break;
                        case 5:
                        case 132:
                          while (count--)
                            *ps++ = (PInt16)alaw2linear(*pb++);
                          break;
                      }

                      count = int(pb - (const signed char *)Buf);

                      currentClassEngine->Send(Buf2, count*sizeof(*ps));
                    }
                  }
                  else
                  if (P.ModemClassId() == EngineBase::mcFax) {
                    if (currentClassEngine)
                      currentClassEngine->Send(Buf, count);
                  }
              }
              if( count <= 0 ) break;
            }
          }
          break;
        case stSendAckWait:
          myPTRACE(1, "Reset state stSendAckWait");
          {
            PWaitAndSignal mutexWait(Mutex);
            SetState(stCommand);
            timeout.Stop();
            if (currentClassEngine)
              currentClassEngine->ResetModemState();
          }
          break;
        default:
          myPTRACE(1, "Reset state " << state);
          if( Echo() )
            bresp.Concatenate(PBYTEArray(pBuf, 1));
          len--;
          pBuf++;
          {
            PWaitAndSignal mutexWait(Mutex);
            timeout.Stop();

            if (state == stRecv && (dataCount || P.ModemClassId() == EngineBase::mcAudio)) {
              PBYTEArray _bresp((const BYTE *)"\x10\x03", 2); // add <DLE><ETX>

              myPTRACE(1, "<-- " << PRTHEX(_bresp));
              bresp.Concatenate(_bresp);
            }

            SetState(stCommand);

            PString resp = RC_PREF();

            if (currentClassEngine && P.ModemClassId() == EngineBase::mcAudio) {
              currentClassEngine->ResetModemState();
              resp += RC_OK();
            } else {
              OnHook();
              resp += RC_NO_CARRIER();
            }

            PBYTEArray _bresp((const BYTE *)(const char *)resp, resp.GetLength());

            myPTRACE(1, "<-- " << PRTHEX(_bresp));
            bresp.Concatenate(_bresp);
          }
      }
    }

    if (state == stCommand)
      SendOnIdle(EngineBase::dtNone);
}

void ModemEngineBody::CheckState(PBYTEArray & bresp)
{
  PString resp;
  PWaitAndSignal mutexWait(Mutex);

  if (cmd.IsEmpty()) {
    if (timerBusy.Get()) {
      if (P.ModemClassId() == EngineBase::mcAudio) {
        PBYTEArray _bresp((const BYTE *)"\x10" "b", 2);		// <DLE>b
        bresp.Concatenate(_bresp);
        myPTRACE(2, "<-- DLE " << PRTHEX(_bresp));
      }
    }

    if (timerRing.Get()) {
      if (off_hook && !pPlayTone && P.ModemClassId() == EngineBase::mcAudio) {
        BYTE b[2] = {'\x10', 'r'};
        PBYTEArray _bresp(b, sizeof(b));
        bresp.Concatenate(_bresp);
        myPTRACE(2, "<-- DLE " << PRTHEX(_bresp));
      }
      else
      if (!off_hook && callState == cstCalled) {
        resp = RC_RING();
        BYTE ringCount = P.RingCount();
        if (!ringCount) {
          if(P.CidMode())
            resp += "NMBR = " + SrcNum() + "\r\n";
          if(P.DidMode())
            resp += "NDID = " + DstNum() + "\r\n";
          if(P.CidMode() || P.DidMode())
            resp += RC_RING();
        }
        P.RingCount(++ringCount);

        if (P.AutoAnswer() > 0 && (ringCount >= P.AutoAnswer())) {
          callDirection = cdIncoming;
          Answer();
        }
      }
    }
  }

  if (timeout.Get()) {
    switch( state ) {
      case stReqModeAckWait:
      case stRecvBegWait:
        resp = RC_NO_CARRIER();
        SetState(stCommand);
        if (currentClassEngine)
          currentClassEngine->ResetModemState();
        else
          OnHook();
        break;
      case stConnectHandle:
        if (subState == chConnectionEstablishDelay) {
          SetSubState(chConnectionEstablished);
          break;
        }
      case stConnectWait:
        resp = RC_NO_CARRIER();
        OnHook();
        break;
      default:
        break;
    }
  }

  for (int i = 0 ; i < (int)(sizeof(activeEngines)/sizeof(activeEngines[0])) ; i++) {
      EngineBase *engine = activeEngines[i];

      if (engine == NULL)
        continue;

      for(;;) {
        BYTE c;

        if (engine->RecvUserInput(&c, 1) <= 0)
          break;

        if (!P.ModemClassId() == EngineBase::mcAudio)
          continue;

        switch (c) {
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
          case 'A':
          case 'B':
          case 'C':
          case 'D':
          case '*':
          case '#': {
            // <DLE>'/'<DLE>c<DLE>'~'
            BYTE b[6] = {'\x10', '/', '\x10', c, '\x10', '~'};
            PBYTEArray _bresp(b, sizeof(b));
            bresp.Concatenate(_bresp);
            myPTRACE(2, "<-- DLE " << PRTHEX(_bresp));
            break;
          }
          default: {
            // <DLE>c
            BYTE b[2] = {'\x10', c};
            PBYTEArray _bresp(b, sizeof(b));
            bresp.Concatenate(_bresp);
            myPTRACE(2, "<-- DLE " << PRTHEX(_bresp));
            break;
          }
        }
      }
  }

  if (off_hook && callState == cstReleasing && callSubState == 0) {
    SetCallSubState(1);

    timerBusy.Start(1000);

    switch (state) {
      case stCommand:
        break;
      case stDial:
      case stConnectWait:
      case stConnectHandle:
      case stReqModeAckWait:
      case stReqModeAckHandle:
        if (callDirection == cdOutgoing)
          resp = RC_BUSY();
        else
          resp = RC_ERROR();

        timeout.Stop();
        OnHook();
        break;
      case stSend:
      case stSendBufEmptyHandle:
        break;
      case stSendAckWait:
        SetState(stSendAckHandle);
        timeout.Stop();
        break;
      case stSendAckHandle:
        break;
      case stRecvBegWait:
        resp = RC_NO_CARRIER();
        SetState(stCommand);
        break;
      case stRecvBegHandle:
        break;
      case stRecv:
        if (dataCount || P.ModemClassId() == EngineBase::mcAudio) {
          PBYTEArray _bresp((const BYTE *)"\x10\x03", 2);		// <DLE><ETX>
          bresp.Concatenate(_bresp);
          PTRACE(2, "<-- DLE " << PRTHEX(_bresp));
        }

        resp = (P.ModemClassId() == EngineBase::mcAudio ? RC_OK() : RC_ERROR());
        SetState(stCommand);
        break;
      default:
        resp = RC_ERROR();
        SetState(stCommand);
    }
  }

  switch (state) {
    case stDial:
      if (callState == cstCleared) {  // we can't dial after releasing w/o on-hook
        SetCallState(cstDialing);
        SetState(stConnectWait);
        timeout.Start(EstablishmentTimeout());

        PStringToString request = params;
        request.SetAt("modemtoken", parent.modemToken());
        request.SetAt("command", "dial");

        Mutex.Signal();
        callbackEndPoint(request, 5);
        Mutex.Wait();

        if (request("response") == "confirm") {
          CallToken(request("calltoken"));
          break;
        } else {
          timeout.Stop();
        }
      }
      OnHook();
      resp = RC_BUSY();
      break;
    case stSendBufEmptyHandle:
      {
        SetState(stSendAckWait);
        if( !activeEngines[mceT38] || !activeEngines[mceT38]->SendStop(moreFrames, NextSeq()) ) {
          SetState(stSendAckHandle);
          timeout.Stop();
          parent.SignalDataReady();  // try to SendAckHandle w/o delay
        }
      }
      break;
    case stConnectHandle:
      {
        switch(subState) {
          case chConnected:
            if (!activeEngines[mceAudio]) {
              SetSubState(chWaitAudioEngine);
              timeout.Start(60000);
              break;
            }
            SetSubState(chAudioEngineAttached);
          case chAudioEngineAttached:
            if (pPlayTone) {
              if (!activeEngines[mceAudio]) {
                SetSubState(chWaitAudioEngine);
                break;
              }

              if (!activeEngines[mceAudio]->IsOpenOut())
                break;

              PBoolean err = FALSE;

              if (activeEngines[mceAudio]->SendStart(EngineBase::dtRaw, 0)) {
                PINDEX len = pPlayTone->GetSize();

                if (len) {
                  const PInt16 *ps = pPlayTone->GetPointer();
                  activeEngines[mceAudio]->Send(ps, len*sizeof(*ps));
                }

                SetSubState(chWaitPlayTone);
                if (activeEngines[mceAudio]->SendStop(FALSE, NextSeq())) {
                  timeout.Start(60000);
                } else {
                  SetSubState(chAudioEngineAttached);
                  err = TRUE;
                }
              } else {
                err = TRUE;
              }

              if (err) {
                resp = RC_ERROR();
                SetState(stCommand);
                OnHook();
              }

              if (pPlayTone) {
                delete pPlayTone;
                pPlayTone = NULL;
              }
              break;
            }
            SetSubState(chTonePlayed);
          case chTonePlayed:
            if (!connectionEstablished && P.ModemClassId() == EngineBase::mcFax) {
              SetSubState(chConnectionEstablishDelay);
              timeout.Start(1000);    // wait 1 sec before request mode
              break;
            }
            SetSubState(chConnectionEstablished);
          case chConnectionEstablished:
            if (callDirection == cdOutgoing && P.ModemClassId() == EngineBase::mcFax)
              SendOnIdle(EngineBase::dtCng);

            connectionEstablished = TRUE;

            if (P.ModemClassId() == EngineBase::mcAudio || callDirection == cdUndefined) {
              SetState(stCommand);
              timeout.Stop();

              PString _resp;
              HandleCmdRest(_resp);

              PBYTEArray _bresp((const BYTE *)(const char *)_resp, _resp.GetLength());

              myPTRACE(1, "<-- " << PRTHEX(_bresp));
              bresp.Concatenate(_bresp);
            }
            else
            if (activeEngines[mceT38]) {
              SetState(stReqModeAckHandle);
              timeout.Stop();
              parent.SignalDataReady();
            } else {
              SetState(stReqModeAckWait);

              timeout.Start(forceFaxMode ? 10000 : 60000);

              PStringToString request;

              request.SetAt("modemtoken", parent.modemToken());
              request.SetAt("command", "requestmode");
              request.SetAt("calltoken", CallToken());
              request.SetAt("mode", forceFaxMode ? "fax" : "fax-no-force");

              Mutex.Signal();
              callbackEndPoint(request, 4);
              Mutex.Wait();

              PString response = request("response");

              if (forceFaxMode && response != "confirm") {
                SetState(stCommand);
                timeout.Stop();
                resp = RC_NO_CARRIER();
              }
            }
            break;
        }
      }
      break;
    case stReqModeAckHandle:
      {
        switch( callDirection ) {
          case cdIncoming:
            dataType = EngineBase::dtCed;
            SetState(stSend);
            if (activeEngines[mceT38] && activeEngines[mceT38]->SendStart(dataType, 3000)) {
              SetState(stSendAckWait);
              if (!activeEngines[mceT38]->SendStop(FALSE, NextSeq())) {
                resp = RC_ERROR();
                SetState(stCommand);
              }
            } else {
              resp = RC_NO_CARRIER();
              SetState(stCommand);
            }
            break;
          case cdOutgoing:
            if (!RecvStart(EngineBase::dtHdlc, 3))
              resp = RC_ERROR();
            break;
          default:
            resp = RC_NO_CARRIER();
            SetState(stCommand);
        }
      }
      break;
    case stSendAckHandle:
      switch (dataType) {
        case EngineBase::dtCed:
          if (!_SendStart(EngineBase::dtHdlc, 3, resp))
            resp = RC_ERROR();
          break;
        case EngineBase::dtSilence:
            resp = RC_OK();
            SetState(stCommand);
          break;
        case EngineBase::dtHdlc:
        case EngineBase::dtRaw:
          {
            if (moreFrames) {
              resp = RC_CONNECT();
              SetState(stSend);
            } else {
              resp = RC_OK();
              SetState(stCommand);
            }
            ResetDleData();
          }
          break;
        default:
          myPTRACE(1, "Unexpected dataType=" << dataType);
      }
      break;
    case stRecvBegHandle:
      {
        SendOnIdle(EngineBase::dtNone);

        if (dataType == EngineBase::dtSilence) {
          if (!SendSilence(param))
            resp = RC_ERROR();
        }
        else
        if (!currentClassEngine || !currentClassEngine->RecvStart(NextSeq())) {
          resp = RC_ERROR();
          SetState(stCommand);
        }
        else
        if (P.ModemClassId() == EngineBase::mcAudio) {
          resp = RC_CONNECT();
          SetState(stRecv);
          parent.SignalDataReady();	// try to Recv w/o delay
        }
        else
        if (P.ModemClassId() == EngineBase::mcFax) {
          if ((currentClassEngine->RecvDiag() & EngineBase::diagDiffSig) == 0) {
            if (dataType != EngineBase::dtRaw) {
              resp = RC_CONNECT();

              if (dataType == EngineBase::dtHdlc)
                fcs = FCS();
            }
            SetState(stRecv);
            parent.SignalDataReady();	// try to Recv w/o delay
          } else {
            currentClassEngine->RecvStop();
            resp = RC_FCERROR();
            SetState(stCommand);
          }
        } else {
          resp = RC_ERROR();
          SetState(stCommand);
        }
        ResetDleData();
      }
      break;
    case stRecv:
      {
        BYTE Buf[1024];
        int count;

        for(;;) {
          if (!currentClassEngine) {
            if (P.ModemClassId() != EngineBase::mcAudio)
              dleData.SetDiag(EngineBase::diagError);

            dleData.PutEof();
            count = -1;
            break;
          }

          count = currentClassEngine->Recv(Buf, sizeof(Buf));

          switch (count) {
            case -1:
              if (P.ModemClassId() == EngineBase::mcAudio) {
                dleData.PutEof();
              } else {
                int diag = currentClassEngine->RecvDiag();

                if (dataType == EngineBase::dtHdlc) {
                  Buf[0] = BYTE(fcs >> 8);
                  Buf[1] = BYTE(fcs);
                  if (diag & EngineBase::diagBadFcs)
                     Buf[0]++;
                  dleData.PutData(Buf, 2);
                }
                dleData.SetDiag(diag).PutEof();
              }

              currentClassEngine->RecvStop();

              if (dataCount == 0 && P.ModemClassId() == EngineBase::mcFax)
                dleData.GetDleData(Buf, sizeof(Buf));	// discard ...<DLE><ETX>
              break;
            case 0:
              break;
            default:
              if (P.ModemClassId() == EngineBase::mcFax) {
                dleData.PutData(Buf, count);

                switch (dataType) {
                case EngineBase::dtHdlc:
                  fcs.build(Buf, count);
                  break;
                case EngineBase::dtRaw:
                  if (!dataCount) {
                    int dms = P.DelayFrmConnect();

                    if (dms) {
                      Mutex.Signal();
                      PThread::Sleep(dms * 10);
                      Mutex.Wait();
                    }

                    // send CONNECT just before data for AT+FRM command

                    PString _resp = RC_PREF() + RC_CONNECT();

                    PBYTEArray _bresp((const BYTE *)(const char *)_resp, _resp.GetLength());

                    myPTRACE(1, "<-- " << PRTHEX(_bresp));
                    bresp.Concatenate(_bresp);
                  }
                  break;
                default:
                  myPTRACE(1, "Unexpected dataType=" << dataType);
                }
              } else {
                const PInt16 *ps = (const PInt16 *)Buf;
                signed char *pb = (signed char *)Buf;

                count /= sizeof(*ps);

                switch (P.Vcml()) {
                  case 0:
                    while (count--)
                      *pb++ = (signed char)((*ps++)/256);
                    break;
                  case 1:
                  case 128:
                  case 130:
                    while (count--)
                      *pb++ = (signed char)((*ps++ + 0x8000)/256);
                    break;
                  case 4:
                  case 131:
                    while (count--)
                      *pb++ = (signed char)linear2ulaw(*ps++);
                    break;
                  case 5:
                  case 132:
                    while (count--)
                      *pb++ = (signed char)linear2alaw(*ps++);
                    break;
                }

                count = int(pb - (signed char *)Buf);

                dleData.PutData(Buf, count);
              }

              dataCount += count;
          }
          if (count <= 0)
            break;
        }

        if (P.ModemClassId() == EngineBase::mcAudio) {
          if (count < 0) {
            PBYTEArray _bresp((const BYTE *)"\x10" "b", 2);	// <DLE>b
            bresp.Concatenate(_bresp);
            myPTRACE(2, "<-- DLE " << PRTHEX(_bresp));
          }
        }

        for(;;) {
          count = dleData.GetDleData(Buf, sizeof(Buf));

          switch (count) {
            case -1:
              {
                SetState(stCommand);

                if (P.ModemClassId() == EngineBase::mcAudio) {
                  resp = RC_OK();
                }
                else
                if (dataType == EngineBase::dtHdlc) {
                  int diag = dleData.GetDiag();

                  if (diag == 0)
                    resp = RC_OK();
                  else
                  if (dataCount == 0 && (diag & ~EngineBase::diagNoCarrier) == 0)
                    resp = RC_NO_CARRIER();
                  else
                    resp = RC_ERROR();
                } else {
                  resp = RC_NO_CARRIER();
                }
              }
              break;
            case 0:
              break;
            default: {
              PBYTEArray _bresp(PBYTEArray(Buf, count));
#if PTRACING
              if (PTrace::CanTrace(4)) {
                 if (count <= 16) {
                   PTRACE(4, "<-- DLE " << PRTHEX(_bresp));
                 } else {
                   PTRACE(4, "<-- DLE " << count << " bytes");
                 }
              }
#endif
              bresp.Concatenate(_bresp);
            }
          }
          if( count <= 0 ) break;
        }
      }
      break;
    default:
      break;
  }

  if (resp.GetLength()) {
    resp = RC_PREF() + resp;

    PBYTEArray _bresp((const BYTE *)(const char *)resp, resp.GetLength());

    myPTRACE(1, "<-- " << PRTHEX(_bresp));
    bresp.Concatenate(_bresp);
  }

  if (state == stCommand)
    SendOnIdle(EngineBase::dtNone);
}
///////////////////////////////////////////////////////////////

