/*
 * h323cap.cxx
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
 * $Log: h323cap.cxx,v $
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

#include <h323/h323.h>
#include <t38/h323t38.h>
#include "ifpmediafmt.h"

#define new PNEW


/////////////////////////////////////////////////////////////////////////////
//
//  MyH323_T38Capability
//    maps H323_T38Capability to OpalMediaFormat
//    creates Real Time Logical Channel
//
class MyH323_T38Capability : public H323_T38Capability
{
    PCLASSINFO(MyH323_T38Capability, H323_T38Capability);
  public:
  /**@name Construction */
  //@{
    MyH323_T38Capability(const OpalMediaFormat &_mediaFormat)
      : H323_T38Capability(e_UDP),
        mediaFormat(_mediaFormat) {}
  //@}

  /**@name Overrides from class PObject */
  //@{
    virtual PObject * Clone() const { return new MyH323_T38Capability(*this); }
  //@}

  /**@name Identification functions */
  //@{
    virtual PString GetFormatName() const { return mediaFormat; }
  //@}

  /**@name Operations */
  //@{
    virtual H323Channel * CreateChannel(
      H323Connection & connection,    ///<  Owner connection for channel
      H323Channel::Directions dir,    ///<  Direction of channel
      unsigned sessionID,             ///<  Session ID for RTP channel
      const H245_H2250LogicalChannelParameters * param
                                      ///<  Parameters for channel
    ) const;
  //@}

  protected:
    const OpalMediaFormat &mediaFormat;
};

class MyH323_T38CapabilityCor : public MyH323_T38Capability {
  public:
    MyH323_T38CapabilityCor() : MyH323_T38Capability(OpalT38_IFP_COR) {}
};

class MyH323_T38CapabilityPre : public MyH323_T38Capability {
  public:
    MyH323_T38CapabilityPre() : MyH323_T38Capability(OpalT38_IFP_PRE) {}
};

H323_REGISTER_CAPABILITY(MyH323_T38CapabilityCor, (const char *)OpalT38_IFP_COR)
H323_REGISTER_CAPABILITY(MyH323_T38CapabilityPre, (const char *)OpalT38_IFP_PRE)

/////////////////////////////////////////////////////////////////////////////
//
//  Implementation
//
/////////////////////////////////////////////////////////////////////////////
H323Channel * MyH323_T38Capability::CreateChannel(
    H323Connection & connection,
    H323Channel::Directions direction,
    unsigned int sessionID,
    const H245_H2250LogicalChannelParameters * params) const
{
  PTRACE(1, "MyH323_T38Capability::CreateChannel "
    << connection
    << " sessionID=" << sessionID
    << " direction=" << direction);

  return connection.CreateRealTimeLogicalChannel(*this, direction, sessionID, params);
}
/////////////////////////////////////////////////////////////////////////////

