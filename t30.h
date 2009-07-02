/*
 * t30.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2003-2009 Vyacheslav Frolov
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
 * Contributor(s): Equivalence Pty ltd
 *
 * $Log: t30.h,v $
 * Revision 1.4  2009-07-02 11:39:05  vfrolov
 * Added T30() constructor
 *
 * Revision 1.4  2009/07/02 11:39:05  vfrolov
 * Added T30() constructor
 *
 * Revision 1.3  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.2  2003/12/04 15:51:54  vfrolov
 * Removed unused DIS member
 *
 * Revision 1.1  2003/12/04 13:38:52  vfrolov
 * Initial revision
 *
 */

#ifndef _T30_H
#define _T30_H

#include "pmutils.h"

///////////////////////////////////////////////////////////////
class T30
{
  public:
    T30() : cfr(FALSE), ecm(FALSE) {}
    void v21Begin() { v21frame = PBYTEArray(); }
    void v21Data(void *pBuf, PINDEX len) { v21frame.Concatenate(PBYTEArray((BYTE *)pBuf, len)); }
    void v21End(PBoolean sent);
    PBoolean hdlcOnly() const { return cfr && ecm; }

  private:
    PBYTEArray v21frame;
    PBoolean cfr;
    PBoolean ecm;
};
///////////////////////////////////////////////////////////////

#endif  // _T30_H

