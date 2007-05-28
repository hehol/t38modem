/*
 * h323ep.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2007 Vyacheslav Frolov
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
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
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
      OpalManager & manager,
      const char * prefix = "h323",
      WORD defaultSignalPort = DefaultTcpSignalPort)
    : H323EndPoint(manager, prefix, defaultSignalPort),
      in_redundancy(0),
      ls_redundancy(0),
      hs_redundancy(0),
      re_interval(0) {}
  //@}

    static PString ArgSpec();
    static PStringArray Descriptions();
    static BOOL Create(OpalManager & mgr, const PConfigArgs & args);
    BOOL Initialise(const PConfigArgs & args);

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

    void AddMediaFormatList(const OpalMediaFormatList & list) { mediaFormatList += list; }

    int InRedundancy() { return in_redundancy; }
    int LsRedundancy() { return ls_redundancy; }
    int HsRedundancy() { return hs_redundancy; }
    int ReInterval() { return re_interval; }

    void SetWriteInterval(
        OpalConnection &connection,
        const PTimeInterval &interval
    );

    BOOL RequestModeChangeT38(OpalConnection & connection);

  protected:
    OpalMediaFormatList mediaFormatList;

    int in_redundancy;
    int ls_redundancy;
    int hs_redundancy;
    int re_interval;
};
/////////////////////////////////////////////////////////////////////////////

#endif  // _MY_H323EP_H

