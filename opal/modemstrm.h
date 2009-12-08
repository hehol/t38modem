/*
 * modemstrm.h
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
 * $Log: modemstrm.h,v $
 * Revision 1.5  2009-12-08 15:06:22  vfrolov
 * Fixed incompatibility with OPAL trunk
 *
 * Revision 1.5  2009/12/08 15:06:22  vfrolov
 * Fixed incompatibility with OPAL trunk
 *
 * Revision 1.4  2009/11/20 16:37:27  vfrolov
 * Fixed audio class application blocking by forced T.38 mode
 *
 * Revision 1.3  2009/10/27 19:03:50  vfrolov
 * Added ability to re-open T38Engine
 * Added ability to prepare IFP packets with adaptive delay/period
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#ifndef _MY_MODEM_MEDIA_STREAM_H
#define _MY_MODEM_MEDIA_STREAM_H

#include <opal/mediastrm.h>

/////////////////////////////////////////////////////////////////////////////
class AudioEngine;

class AudioModemMediaStream : public OpalRawMediaStream
{
    PCLASSINFO(AudioModemMediaStream, OpalRawMediaStream);
  public:
    AudioModemMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource,                   ///<  Is a source stream
      AudioEngine *_audioEngine            ///<  I/O channel to stream to/from
    );

  /**@name Overrides of OpalRawMediaStream class */
  //@{
    virtual PBoolean Open();
    virtual PBoolean Close();

    virtual PBoolean IsSynchronous() const { return FALSE; }
  //@}

  protected:
    AudioEngine *audioEngine;
};
/////////////////////////////////////////////////////////////////////////////
class T38Engine;

class T38ModemMediaStream : public OpalMediaStream
{
    PCLASSINFO(T38ModemMediaStream, OpalMediaStream);
  public:
  /**@name Construction */
  //@{
    /**Construct a new media stream.
      */
    T38ModemMediaStream(
      OpalConnection & conn,
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource,                   ///<  Is a source stream
      T38Engine *_t38engine
    );
  //@}

  /**@name Overrides of OpalMediaStream class */
  //@{
    virtual PBoolean Open();
    virtual PBoolean Close();
    virtual void OnStartMediaPatch();

    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    virtual PBoolean IsSynchronous() const { return FALSE; }
  //@}

  protected:
    long currentSequenceNumber;
#if PTRACING
    int totallost;
#endif
    T38Engine * t38engine;
};
/////////////////////////////////////////////////////////////////////////////

#endif  // _MY_MODEM_MEDIA_STREAM_H

