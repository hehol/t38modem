/*
 * modemep.h
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
 * $Log: modemep.h,v $
 * Revision 1.8  2010-03-15 13:40:28  vfrolov
 * Removed unused code
 *
 * Revision 1.8  2010/03/15 13:40:28  vfrolov
 * Removed unused code
 *
 * Revision 1.7  2010/02/16 16:21:25  vfrolov
 * Added --force-fax-mode and --no-force-t38-mode options
 *
 * Revision 1.6  2010/02/08 17:30:31  vfrolov
 * Disabled OPAL version < 3.8.0
 *
 * Revision 1.5  2009/11/02 18:02:19  vfrolov
 * Removed pre v3.7 compatibility code
 *
 * Revision 1.4  2009/07/22 14:42:49  vfrolov
 * Added Descriptions(args) to endpoints
 *
 * Revision 1.3  2009/07/13 15:08:17  vfrolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#ifndef _MODEM_EP_H
#define _MODEM_EP_H

/////////////////////////////////////////////////////////////////////////////
#define PACK_VERSION(major, minor, build) (((((major) << 8) + (minor)) << 8) + (build))

#if !(PACK_VERSION(OPAL_MAJOR, OPAL_MINOR, OPAL_BUILD) >= PACK_VERSION(3, 16, 1))
  #error *** Incompatible OPAL version (required >= 3.16.1) ***
#endif

#undef PACK_VERSION
/////////////////////////////////////////////////////////////////////////////
#include <t38/t38proto.h>
#include "manager.h"
/////////////////////////////////////////////////////////////////////////////
class PseudoModem;
class PseudoModemQ;

class ModemEndPoint : public OpalFaxEndPoint, public MyManagerEndPoint
{
    PCLASSINFO(ModemEndPoint, OpalFaxEndPoint);
  public:
  /**@name Construction */
  //@{
    /**Create a new endpoint.
     */
    ModemEndPoint(
      MyManager & manager,              ///< Manager of all endpoints.
      const char * g711Prefix = "fax",  ///< Prefix for URL style address strings
      const char * t38Prefix = "t38"    ///< Prefix for URL style address strings
    );
    ~ModemEndPoint();
  //@}
    static PString GetArgumentSpec();
    virtual bool Initialise(PArgList & args, bool verbose, const PString & defaultRoute);

    PseudoModem * PMAlloc(const PString &number) const;
    void PMFree(PseudoModem *pmodem) const;

  /**@name Overrides from OpalEndPoint */
  //@{
    virtual PSafePtr<OpalConnection> MakeConnection(
      OpalCall & call,              ///< Owner of connection
      const PString & party,        ///< Remote party to call
      void * userData = NULL,       ///< Arbitrary data to pass to connection
      unsigned int options = 0,     ///< Options to pass to conneciton
      OpalConnection::StringOptions * stringOptions = NULL
    );

    virtual OpalMediaFormatList GetMediaFormats() const;
  //@}

    static PStringToString defaultStringOptions;

  protected:
    PseudoModemQ *pmodem_pool;
    PString m_prefix;

    PDECLARE_NOTIFIER(PObject, ModemEndPoint, OnMyCallback);
};
/////////////////////////////////////////////////////////////////////////////

#endif  // _MODEM_EP_H

