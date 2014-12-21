/*
 * sipep.h
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
 * $Log: sipep.h,v $
 * Revision 1.6  2010-03-15 13:40:28  vfrolov
 * Removed unused code
 *
 * Revision 1.6  2010/03/15 13:40:28  vfrolov
 * Removed unused code
 *
 * Revision 1.5  2010/01/21 16:00:55  vfrolov
 * Changed --sip-audio to accept multiple wildcards
 * Implemented OPAL-Enable-Audio route option
 *
 * Revision 1.4  2009/11/10 11:30:57  vfrolov
 * Implemented G.711 fallback to fax pass-through mode
 *
 * Revision 1.3  2009/07/22 14:42:49  vfrolov
 * Added Descriptions(args) to endpoints
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#ifndef _MY_SIPEP_H
#define _MY_SIPEP_H

#include <sip/sipep.h>

/////////////////////////////////////////////////////////////////////////////
class MySIPEndPoint : public SIPEndPoint
{
  PCLASSINFO(MySIPEndPoint, SIPEndPoint);

  public:
  /**@name Construction */
  //@{
    MySIPEndPoint(
      OpalManager & manager
    )
    : SIPEndPoint(manager)
      {}
  //@}

    static PString ArgSpec();
    static PStringArray Descriptions();
    static PStringArray Descriptions(const PConfigArgs & args);
    static PBoolean Create(OpalManager & mgr, const PConfigArgs & args);
    PBoolean Initialise(const PConfigArgs & args);
    void OnRegistrationStatus(const RegistrationStatus & status);

    virtual SIPConnection * CreateConnection(
      OpalCall & call,                         ///<  Owner of connection
      const PString & token,                   ///<  token used to identify connection
      void * userData,                         ///<  User data for connection
      const SIPURL & destination,              ///<  Destination for outgoing call
      OpalTransport * transport,               ///<  Transport INVITE has been received on
      SIP_PDU * invite,                        ///<  Original INVITE pdu
      unsigned int options = 0,                ///<  connection options
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

  protected:
    PStringToString defaultStringOptions;
};
/////////////////////////////////////////////////////////////////////////////

#endif  // _MY_SIPEP_H

