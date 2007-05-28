/*
 * manager.h
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
 * $Log: manager.h,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
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
    BOOL Initialise(const PConfigArgs & args);

    void SetWriteInterval(
        OpalConnection &connection,
        const PTimeInterval &interval
    );

    virtual PString OnRouteConnection(OpalConnection & connection);
    virtual void OnClearedCall(OpalCall & call);

    virtual BOOL OnOpenMediaStream(OpalConnection & connection, OpalMediaStream & stream);
    virtual void OnClosedMediaStream(const OpalMediaStream & stream);

    virtual PString ApplyRouteTable(const PString & proto, const PString & addr);

    virtual BOOL OnRequestModeChange(
      OpalConnection & connection,
      const OpalMediaFormatList & mediaFormatList
    );
};
/////////////////////////////////////////////////////////////////////////////

#endif  // _PM_MANAGER_H

