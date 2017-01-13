/*
 * pmutils.cxx
 *
 * T38FAX Pseudo Modem
 *
 * Copyright (c) 2001-2006 Vyacheslav Frolov
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
 * Revision 1.13  2006-12-11 10:27:35  vfrolov
 * Disabled renaming thread if no PTRACING
 *
 * Revision 1.13  2006/12/11 10:27:35  vfrolov
 * Disabled renaming thread if no PTRACING
 *
 * Revision 1.12  2005/02/03 11:32:12  vfrolov
 * Fixed MSVC compile warnings
 *
 * Revision 1.11  2004/10/20 14:15:09  vfrolov
 * Added reset of signal counter to WaitDataReady()
 *
 * Revision 1.10  2004/07/06 16:07:24  vfrolov
 * Included ptlib.h for precompiling
 *
 * Revision 1.9  2004/03/09 17:23:11  vfrolov
 * Added PROCESS_PER_THREAD ifdef
 *
 * Revision 1.8  2004/02/17 13:23:14  vfrolov
 * Fixed MSVC compile errors
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

#include <ptlib.h>
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

void ModemThread::WaitDataReady()
{
  //do {
    dataReadySyncPoint.Wait();
  //} while(!dataReadySyncPoint.WillBlock());
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
int DataStream::PutData(const void *_pBuf, PINDEX count)
{
  if (eof)
    return -1;

  int done = 0;
  const BYTE *pBuf = (const BYTE *)_pBuf;

  while (count) {
    if (!lastBuf) {
      lastBuf = new ChunkStream();
      bufQ.Enqueue(lastBuf);
    }

    int len = lastBuf->write(pBuf, count);

    if (len < 0) {
      lastBuf = NULL;
    } else {
      pBuf += len;
      count -= len;
      done += len;
    }
  }

  busy += done;

  return done;
}

int DataStream::GetData(void *_pBuf, PINDEX count)
{
  if (!busy) {
    if (eof)
      return -1;
    else
      return 0;
  }

  int done = 0;
  BYTE *pBuf = (BYTE *)_pBuf;

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
      pBuf += len;
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
  while ((buf = bufQ.Dequeue()) != NULL)
    delete buf;
  if (firstBuf)
    delete firstBuf;
  firstBuf = lastBuf = NULL;
  busy = 0;
  eof = FALSE;
  diag = 0;
}
///////////////////////////////////////////////////////////////
#if PTRACING
void RenameCurrentThread(const PString &newname)
{
  PString oldname = PThread::Current()->GetThreadName();

  PThread::Current()->SetThreadName(PString(newname)
    #if defined(PROCESS_PER_THREAD)
        + ":" + PString((unsigned)getpid())
    #else
      #if defined(P_PTHREADS)
        + ":" + PString((unsigned)pthread_self())
      #else
        + ":%0x"
      #endif
    #endif
  );
  PTRACE(2, "RenameCurrentThread old ThreadName=" << oldname);
}
#endif /* PTRACING */
///////////////////////////////////////////////////////////////
#ifdef PROCESS_PER_THREAD
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

