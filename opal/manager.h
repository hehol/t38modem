/*
 * manager.h
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
 * $Log: manager.h,v $
 * Revision 1.6  2010-03-15 13:40:27  vfrolov
 * Removed unused code
 *
 * Revision 1.6  2010/03/15 13:40:27  vfrolov
 * Removed unused code
 *
 * Revision 1.5  2009/11/10 11:30:57  vfrolov
 * Implemented G.711 fallback to fax pass-through mode
 *
 * Revision 1.4  2009/07/15 13:23:20  vfrolov
 * Added Descriptions(args)
 *
 * Revision 1.3  2009/01/15 08:46:34  vfrolov
 * Fixed OnRouteConnection() be able to compile with OPAL trunk since 21925
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 */

#ifndef _PM_MANAGER_H
#define _PM_MANAGER_H

#include <opal/manager.h>

/////////////////////////////////////////////////////////////////////////////
class MyManager : public OpalManager
{
  PCLASSINFO(MyManager, OpalManager);

  public:
    MyManager();

    static PString ArgSpec();
    static PStringArray Descriptions();
    static PStringArray Descriptions(const PConfigArgs & args);
    PBoolean Initialise(const PConfigArgs & args);

    virtual bool OnRouteConnection(
      PStringSet & routesTried,     ///< Set of routes already tried
      const PString & a_party,      ///< Source local address
      const PString & b_party,      ///< Destination indicated by source
      OpalCall & call,              ///< Call for new connection
      unsigned options,             ///< Options for new connection (can't use default as overrides will fail)
      OpalConnection::StringOptions * stringOptions
    );
    virtual void OnClearedCall(OpalCall & call);
    virtual PBoolean AddRouteEntry(const PString & spec);

    virtual PBoolean OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    virtual void OnClosedMediaStream(const OpalMediaStream & stream);

    virtual PString ApplyRouteTable(const PString & proto, const PString & addr, PINDEX & entry);
};
/////////////////////////////////////////////////////////////////////////////

#endif  // _PM_MANAGER_H

