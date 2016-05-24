/*
 * h323ep.h
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
 * $Log: h323ep.h,v $
 * Revision 1.7  2010-03-15 13:40:27  vfrolov
 * Removed unused code
 *
 * Revision 1.7  2010/03/15 13:40:27  vfrolov
 * Removed unused code
 *
 * Revision 1.6  2010/01/21 16:05:33  vfrolov
 * Changed --h323-audio to accept multiple wildcards
 * Implemented OPAL-Enable-Audio route option
 * Renamed route option OPAL-H323-Bearer-Capability to OPAL-Bearer-Capability
 *
 * Revision 1.5  2009/12/23 17:54:24  vfrolov
 * Implemented --h323-bearer-capability option
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

#ifndef _MY_H323EP_H
#define _MY_H323EP_H

#include <h323/h323.h>

/////////////////////////////////////////////////////////////////////////////
//
//  myH323EndPoint
//    creates MyH323Connection
//
class OpalMediaFormat;

class MyH323EndPoint : public H323EndPoint
{
  PCLASSINFO(MyH323EndPoint, H323EndPoint);

  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    MyH323EndPoint(
      OpalManager & manager)
    : H323EndPoint(manager)
      {}
  //@}

    static PString ArgSpec();
    static PStringArray Descriptions();
    static PStringArray Descriptions(const PConfigArgs & args);
    static PBoolean Create(OpalManager & mgr, const PConfigArgs & args);
    PBoolean Initialise(const PConfigArgs & args);

    virtual H323Connection * CreateConnection(
      OpalCall & call,                         ///<  Call object to attach the connection to
      const PString & token,                   ///<  Call token for new connection
      void * userData,                         ///<  Arbitrary user data from MakeConnection
      OpalTransport & transport,               ///<  Transport for connection
      const PString & alias,                   ///<  Alias for outgoing call
      const H323TransportAddress & address,    ///<  Address for outgoing call
      H323SignalPDU * setupPDU,                ///<  Setup PDU for incoming call
      unsigned options = 0,
      OpalConnection::StringOptions * stringOptions = NULL ///<  complex string options
    );

    static PStringToString defaultStringOptions;
};
/////////////////////////////////////////////////////////////////////////////

#endif  // _MY_H323EP_H

