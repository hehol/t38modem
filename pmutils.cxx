/*
 * pmutils.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2003 Vyacheslav Frolov
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
 * $Log: pmutils.cxx,v $
 * Revision 1.7  2003-12-04 13:22:28  vfrolov
 * Removed ambiguous isEof()
 * Improved memory usage in DataStream
 * Fixed myPTRACE
 *
 * Revision 1.7  2003/12/04 13:22:28  vfrolov
 * Removed ambiguous isEof()
 * Improved memory usage in DataStream
 * Fixed myPTRACE
 *
 * Revision 1.6  2003/01/08 16:37:25  vfrolov
 * Changed class DataStream:
 *   members moved to private section and added isEof()
 *   added threshold and isFull()
 *
 * Revision 1.5  2002/12/30 12:49:36  vfrolov
 * Added tracing thread's CPU usage (Linux only)
 *
 * Revision 1.4  2002/12/20 10:12:57  vfrolov
 * Implemented tracing with PID of thread (for LinuxThreads)
 *   or ID of thread (for other POSIX Threads)
 *
 * Revision 1.3  2002/03/07 07:55:18  vfrolov
 * Fixed endless recursive call SignalChildStop(). Possible there is
 * a bug in gcc version 2.95.4 20010902 (Debian prerelease).
 * Markus Storm reported the promlem.
 * Added Copyright header.
 *
 * Revision 1.2  2002/01/10 06:10:03  craigs
 * Added MPL header
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

#include "pmutils.h"

#define new PNEW

///////////////////////////////////////////////////////////////
ModemThread::ModemThread()
  : PThread(30000,
            NoAutoDeleteThread,
            NormalPriority),
    stop(FALSE),
    childstop(FALSE)
{
}

void ModemThread::SignalChildStop() {
  childstop = TRUE;
  SignalDataReady();
}

void ModemThread::SignalStop() {
  stop = TRUE;
  SignalDataReady();
}
///////////////////////////////////////////////////////////////
ModemThreadChild::ModemThreadChild(ModemThread &_parent)
  : parent(_parent)
{
}

void ModemThreadChild::SignalStop()
{
  ModemThread::SignalStop();
  parent.SignalChildStop();
}
///////////////////////////////////////////////////////////////
int ChunkStream::write(const void *pBuf, PINDEX count)
{
  int len = sizeof(data) - last;

  if (!len)
    return -1;

  if (len > count)
    len = count;

  memcpy(data + last, pBuf, len);
  last += len;

  return len;
}

int ChunkStream::read(void *pBuf, PINDEX count)
{
  if (sizeof(data) == first)
    return -1;

  int len = last - first;

  if (len > count)
    len = count;

  memcpy(pBuf, data + first, len);
  first += len;

  return len;
}
///////////////////////////////////////////////////////////////
int DataStream::PutData(const void *pBuf, PINDEX count)
{
  if (eof)
    return -1;

  int done = 0;

  while (count) {
    if (!lastBuf) {
      lastBuf = new ChunkStream();
      bufQ.Enqueue(lastBuf);
    }

    int len = lastBuf->write(pBuf, count);

    if (len < 0) {
      lastBuf = NULL;
    } else {
      (const BYTE *)pBuf += len;
      count -= len;
      done += len;
    }
  }

  busy += done;

  return done;
}

int DataStream::GetData(void *pBuf, PINDEX count)
{
  if (!busy) {
    if (eof)
      return -1;
    else
      return 0;
  }

  int done = 0;

  while (count) {
    if (!firstBuf) {
      firstBuf = bufQ.Dequeue();
      if (!firstBuf) {
        lastBuf = NULL;
        break;
      }
    }

    int len = firstBuf->read(pBuf, count);

    if (len < 0) {
      delete firstBuf;
      firstBuf = NULL;
    } else {
      if (!len)
        break;
      (const BYTE *)pBuf += len;
      count -= len;
      done += len;
    }
  }

  busy -= done;

  return done;
}

void DataStream::Clean()
{
  ChunkStream *buf;
  while ((buf = bufQ.Dequeue()))
    delete buf;
  if (firstBuf)
    delete firstBuf;
  firstBuf = lastBuf = NULL;
  busy = 0;
  eof = FALSE;
  diag = 0;
}
///////////////////////////////////////////////////////////////
void RenameCurrentThread(const PString &newname)
{
#if PTRACING
  PString oldname = PThread::Current()->GetThreadName();
#endif
  PThread::Current()->SetThreadName(PString(newname)
    #if defined(P_PTHREADS)
      #if defined(P_LINUX)
        + ":" + PString(getpid())
      #else
        + ":" + PString(pthread_self())
      #endif
    #else
        + ":%0x"
    #endif
  );
#if PTRACING
  PTRACE(2, "RenameCurrentThread old ThreadName=" << oldname);
#endif
}
///////////////////////////////////////////////////////////////
#ifdef P_LINUX
#include <sys/times.h>
static double clktck = sysconf(_SC_CLK_TCK);

const PString GetThreadTimes(const char *head, const char *tail)
{
  struct tms t;

  if (clktck && times(&t) != -1) {
    return psprintf("%suser=%.3f, system=%.3f%s",
                    head,
                    t.tms_utime/clktck,
                    t.tms_stime/clktck,
                    tail);
  }
  return "";
}
#endif
///////////////////////////////////////////////////////////////

