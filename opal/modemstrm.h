/*
 * modemstrm.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007-2008 Vyacheslav Frolov
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
 * Revision 1.2  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
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
class AudioModemMediaStream : public OpalRawMediaStream
{
    PCLASSINFO(AudioModemMediaStream, OpalRawMediaStream);
  public:
    AudioModemMediaStream(
      OpalConnection & conn,
      const OpalMediaFormat & mediaFormat, ///<  Media format for stream
      unsigned sessionID,                  ///<  Session number for stream
      PBoolean isSource,                   ///<  Is a source stream
      PChannel * channel                   ///<  I/O channel to stream to/from
    )
    : OpalRawMediaStream(conn, mediaFormat, sessionID, isSource, channel, FALSE) {}

    virtual PBoolean IsSynchronous() const { return FALSE; }
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

    static const OpalMediaFormat & GetT38MediaFormat();

  /**@name Overrides of OpalMediaStream class */
  //@{
    virtual PBoolean Open();
    virtual PBoolean Close();

    virtual PBoolean ReadPacket(
      RTP_DataFrame & packet
    );

    virtual PBoolean WritePacket(
      RTP_DataFrame & packet
    );

    virtual PBoolean IsSynchronous() const;
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

