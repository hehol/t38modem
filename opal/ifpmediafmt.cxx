/*
 * ifpmediafmt.cxx
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
 * $Log: ifpmediafmt.cxx,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 *
 */

#include <ptlib.h>
#include <opal/mediafmt.h>
#include "ifpmediafmt.h"

/////////////////////////////////////////////////////////////////////////////
// OPAL's SIP does not support multiple media formats with
// the same RTP encoding name, so we can set it to "t38"
// only for one of them.

static char encodingName_COR[7] = "t38";
static char encodingName_PRE[7] = "t38pre";

// Fortunately we can change it while initialization

void SetT38_IFP_PRE()
{
  strcpy(encodingName_COR, "t38cor");
  strcpy(encodingName_PRE, "t38");
}
/////////////////////////////////////////////////////////////////////////////
const OpalMediaFormat & GetOpalT38_IFP_COR()
{
  static const OpalMediaFormat opalT38_IFP(
    "T.38-IFP-COR",
    OpalMediaFormat::DefaultDataSessionID,
    RTP_DataFrame::IllegalPayloadType,
    encodingName_COR,
    FALSE, // No jitter for data
    1440, // 100's bits/sec
    0,
    0,
    0);

  return opalT38_IFP;
}

const OpalMediaFormat & GetOpalT38_IFP_PRE()
{
  static const OpalMediaFormat opalT38_IFP(
    "T.38-IFP-PRE",
    OpalMediaFormat::DefaultDataSessionID,
    RTP_DataFrame::IllegalPayloadType,
    encodingName_PRE,
    FALSE, // No jitter for data
    1440, // 100's bits/sec
    0,
    0,
    0);

  return opalT38_IFP;
}
/////////////////////////////////////////////////////////////////////////////

