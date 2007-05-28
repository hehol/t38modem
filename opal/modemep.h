/*
 * modemep.h
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
 * $Log: modemep.h,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _MODEM_EP_H
#define _MODEM_EP_H

#include <opal/endpoint.h>

/////////////////////////////////////////////////////////////////////////////
class PseudoModem;
class PseudoModemQ;

class ModemEndPoint : public OpalEndPoint
{
    PCLASSINFO(ModemEndPoint, OpalEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    ModemEndPoint(
      OpalManager & manager,        /// Manager of all endpoints.
      const char * prefix = "modem" /// Prefix for URL style address strings
    );
  //@}

    static PString ArgSpec();
    static PStringArray Descriptions();
    static BOOL Create(OpalManager & mgr, const PConfigArgs & args);
    BOOL Initialise(const PConfigArgs & args);

    PseudoModem * PMAlloc(const PString &number) const;
    void PMFree(PseudoModem *pmodem) const;

    void SetReadTimeout(
        OpalConnection &connection,
        const PTimeInterval &timeout
    );

  /**@name Overrides from OpalEndPoint */
  //@{
    virtual BOOL MakeConnection(
      OpalCall & call,        /// Owner of connection
      const PString & party,  /// Remote party to call
      void * userData = NULL,  /// Arbitrary data to pass to connection
      unsigned int options = 0,     ///<  options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    );

    virtual OpalMediaFormatList GetMediaFormats() const;
  //@}

  protected:
    PStringArray routes;
    PseudoModemQ *pmodem_pool;

    PDECLARE_NOTIFIER(PObject, ModemEndPoint, OnMyCallback);
};
/////////////////////////////////////////////////////////////////////////////

#endif  // _MODEM_EP_H

