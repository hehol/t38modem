/*
 * t30.h
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2003 Vyacheslav Frolov
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
 * Revision 1.2  2003-12-04 15:51:54  vfrolov
 * Removed unused DIS member
 *
 * Revision 1.2  2003/12/04 15:51:54  vfrolov
 * Removed unused DIS member
 *
 * Revision 1.1  2003/12/04 13:38:52  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _T30_H
#define _T30_H

#include "pmutils.h"

///////////////////////////////////////////////////////////////
class T30
{
  public:
    void v21Begin() { v21frame = PBYTEArray(); }
    void v21Data(void *pBuf, PINDEX len) { v21frame.Concatenate(PBYTEArray((BYTE *)pBuf, len)); }
    void v21End(BOOL sent);
    BOOL hdlcOnly() const { return cfr && ecm; }

  private:
    PBYTEArray v21frame;
    BOOL cfr;
    BOOL ecm;
};
///////////////////////////////////////////////////////////////

#endif  // _T30_H

