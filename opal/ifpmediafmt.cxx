/*
 * ifpmediafmt.cxx
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
 * $Log: ifpmediafmt.cxx,v $
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

#include <ptlib.h>

#include <opal/buildopts.h>

#include <opal/mediafmt.h>
#include "ifpmediafmt.h"

/////////////////////////////////////////////////////////////////////////////
// OPAL's SIP does not support multiple media formats with
// the same RTP encoding name, so we need to workaround it

static PString t38_IFP_FormatForSip = GetOpalT38_IFP_COR().GetName();

// Fortunately we can set valid while initialization

void SetT38_IFP_PRE()
{
  t38_IFP_FormatForSip = GetOpalT38_IFP_PRE().GetName();

  PTRACE(2, "SetT38_IFP_PRE() t38_IFP_FormatForSip" << " = " << t38_IFP_FormatForSip);
}

/////////////////////////////////////////////////////////////////////////////
class T38_IFP_OpalMediaFormatInternal : public OpalMediaFormatInternal
{
    PCLASSINFO(T38_IFP_OpalMediaFormatInternal, OpalMediaFormatInternal);
  public:
    T38_IFP_OpalMediaFormatInternal(
      const char * fullName,                      ///<  Full name of media format
      const OpalMediaType & mediaType,            ///<  media type for this format
      RTP_DataFrame::PayloadTypes rtpPayloadType, ///<  RTP payload type code
      const char * encodingName,                  ///<  RTP encoding name
      PBoolean     needsJitter,                   ///<  Indicate format requires a jitter buffer
      unsigned bandwidth,                         ///<  Bandwidth in bits/second
      PINDEX   frameSize,                         ///<  Size of frame in bytes (if applicable)
      unsigned frameTime,                         ///<  Time for frame in RTP units (if applicable)
      unsigned clockRate,                         ///<  Clock rate for data (if applicable)
      time_t timeStamp = 0                        ///<  timestamp (for versioning)
    )
    : OpalMediaFormatInternal(fullName,
                              mediaType,
                              rtpPayloadType,
                              encodingName,
                              needsJitter,
                              bandwidth,
                              frameSize,
                              frameTime,
                              clockRate,
                              timeStamp)
    {}

    virtual bool IsValidForProtocol(const PString & protocol) const;
};

bool T38_IFP_OpalMediaFormatInternal::IsValidForProtocol(const PString & protocol) const
{
  if (!OpalMediaFormatInternal::IsValidForProtocol(protocol))
    return false;

  if (protocol *= "sip") {
    PWaitAndSignal m(media_format_mutex);

    if (formatName != t38_IFP_FormatForSip) {
      PTRACE(2, "T38_IFP_OpalMediaFormatInternal::IsValidForProtocol(" << protocol << ") " <<
                formatName << " != " << t38_IFP_FormatForSip);
      return false;
    }
  }

  return true;
}
/////////////////////////////////////////////////////////////////////////////
const OpalMediaFormat & GetOpalT38_IFP_COR()
{
  static const OpalMediaFormat opalT38_IFP(new T38_IFP_OpalMediaFormatInternal(
    "T.38-IFP-COR",
    "fax",
    RTP_DataFrame::DynamicBase,
    "t38",
    PFalse, // No jitter for data
    1440,   // 100's bits/sec
    0,
    0,
    0));

  return opalT38_IFP;
}

const OpalMediaFormat & GetOpalT38_IFP_PRE()
{
  static const OpalMediaFormat opalT38_IFP(new T38_IFP_OpalMediaFormatInternal(
    "T.38-IFP-PRE",
    "fax",
    RTP_DataFrame::DynamicBase,
    "t38",
    PFalse, // No jitter for data
    1440,   // 100's bits/sec
    0,
    0,
    0));

  return opalT38_IFP;
}
/////////////////////////////////////////////////////////////////////////////

