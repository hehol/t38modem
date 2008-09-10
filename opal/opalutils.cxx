/*
 * opalutils.cxx
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
 * $Log: opalutils.cxx,v $
 * Revision 1.2  2008-09-10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.2  2008/09/10 11:15:00  frolov
 * Ported to OPAL SVN trunk
 *
 * Revision 1.1  2007/07/20 14:26:40  vfrolov
 * Initial revision
 *
 */

#include <ptlib.h>

#include <opal/buildopts.h>

#include "opalutils.h"

#define new PNEW

/////////////////////////////////////////////////////////////////////////////
PString GetPartyName(const PString & party)
{
  PINDEX beg = party.Find(':');

  if (beg != P_MAX_INDEX)
    beg++;

  PINDEX end = party.Find('@');

  if (end != P_MAX_INDEX)
    end--;

  return party(beg, end);
}
/////////////////////////////////////////////////////////////////////////////

