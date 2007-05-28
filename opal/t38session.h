/*
 * t38session.h
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
 * $Log: t38session.h,v $
 * Revision 1.1  2007-05-28 12:47:52  vfrolov
 * Initial revision
 *
 * Revision 1.1  2007/05/28 12:47:52  vfrolov
 * Initial revision
 *
 *
 */

#ifndef _T38_SESSION_H
#define _T38_SESSION_H

#include <rtp/rtp.h>

/////////////////////////////////////////////////////////////////////////////
class OpalConnection;
class OpalTransport;

RTP_Session * CreateSessionT38(
    const OpalConnection & connection,
    const OpalTransport & transport,
    unsigned sessionID,
    RTP_QOS * rtpqos,
    RTP_DataFrame::PayloadTypes pt,
    int in_redundancy,
    int ls_redundancy,
    int hs_redundancy,
    int re_interval);
/////////////////////////////////////////////////////////////////////////////

#endif  // _T38_SESSION_H

