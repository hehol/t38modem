#
# Makefile
#
# T38FAX Pseudo Modem
#
# Copyright (c) 2001-2011 Vyacheslav Frolov
#
# t38modem Project
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and limitations
# under the License.
#
# The Original Code is Open H323 Library.
#
# The Initial Developer of the Original Code is Vyacheslav Frolov
#
# Contributor(s): Equivalence Pty ltd
#
# $Log: Makefile,v $
# Revision 1.29  2011/01/12 12:23:43  vfrolov
# Replaced hardcoded workaround for mgetty-voice by conditional one
#
# Revision 1.28  2010/10/12 16:46:25  vfrolov
# Implemented fake streams
#
# Revision 1.27  2010/10/06 16:54:19  vfrolov
# Redesigned engine opening/closing
#
# Revision 1.26  2010/02/12 08:35:51  vfrolov
# Added fake_codecs.cxx
#
# Revision 1.25  2009/07/31 17:34:40  vfrolov
# Removed --h323-old-asn and --sip-old-asn options
#
# Revision 1.24  2009/07/29 10:39:04  vfrolov
# Moved h323lib specific code to h323lib directory
#
# Revision 1.23  2009/07/27 16:21:24  vfrolov
# Moved h323lib specific code to h323lib directory
#
# Revision 1.22  2008/09/11 16:10:54  frolov
# Ported to H323 Plus trunk
#
# Revision 1.21  2008/09/10 13:39:41  frolov
# Fixed OBJDIR_SUFFIX for OPAL
#
# Revision 1.20  2008/09/10 11:15:00  frolov
# Ported to OPAL SVN trunk
#
# Revision 1.19  2007/07/20 14:28:38  vfrolov
# Added opalutils.cxx
#
# Revision 1.18  2007/07/17 10:03:22  vfrolov
# Added Unix98 PTY support
#
# Revision 1.17  2007/05/28 12:52:27  vfrolov
# Added OPAL support
#
# Revision 1.16  2007/05/17 08:32:44  vfrolov
# Moved class T38Modem from main.h and main.cxx to main_process.cxx
#
# Revision 1.15  2007/05/03 09:21:47  vfrolov
# Added compile time optimization for original ASN.1 sequence
# in T.38 (06/98) Annex A or for CORRIGENDUM No. 1 fix
#
# Revision 1.14  2007/03/23 10:14:35  vfrolov
# Implemented voice mode functionality
#
# Revision 1.13  2006/10/20 10:06:43  vfrolov
# Added REPEAT_INDICATOR_SENDING ifdef
#
# Revision 1.12  2006/10/19 10:45:59  vfrolov
# Added FD_TRACE_LEVEL ifdef
#
# Revision 1.11  2004/07/07 12:38:32  vfrolov
# The code for pseudo-tty (pty) devices that communicates with fax application formed to PTY driver.
#
# Revision 1.10  2004/03/09 17:22:58  vfrolov
# Added PROCESS_PER_THREAD ifdef
#
# Revision 1.9  2003/12/04 15:56:45  vfrolov
# Added hdlc.cxx t30.cxx fcs.cxx
#
# Revision 1.8  2002/08/05 10:10:29  robertj
# Normalised Makefile usage of openh323u.mak include file, fixing odd messages.
#
# Revision 1.7  2002/04/30 11:05:17  vfrolov
# Implemented T.30 Calling Tone (CNG) generation
#
# Revision 1.6  2002/04/30 03:52:28  craigs
# Added option for G.723.1 codec
#
# Revision 1.5  2002/04/27 10:17:20  vfrolov
# Added checking if COUT_TRACE or MYPTRACE_LEVEL defined
# Do not add -DCOUT_TRACE by default
#
# Revision 1.4  2002/02/11 08:35:08  vfrolov
# myPTRACE() outputs trace to cout only if defined COUT_TRACE
#
# Revision 1.3  2002/01/10 06:10:02  craigs
# Added MPL header
#
#

PROG		= t38modem
SOURCES		:= pmutils.cxx dle.cxx pmodem.cxx pmodemi.cxx drivers.cxx \
		   t30tone.cxx tone_gen.cxx hdlc.cxx t30.cxx fcs.cxx \
		   pmodeme.cxx enginebase.cxx t38engine.cxx audio.cxx \
		   drv_pty.cxx \
		   main_process.cxx

#
# Build t38modem for
#  - Open Phone Abstraction Library if defined USE_OPAL
#  - Open H323 Library or H323 Plus Library if not defined USE_OPAL
#    (NOTE: define NO_PBOOLEAN for Open H323 Library)
#
ifdef USE_OPAL
  VPATH_CXX := opal

  SOURCES += \
             opalutils.cxx \
             modemep.cxx modemstrm.cxx \
             h323ep.cxx \
             sipep.cxx \
             manager.cxx \
             fake_codecs.cxx \

  ifndef OPALDIR
    OPALDIR=$(HOME)/opal
  endif

  OBJDIR_SUFFIX = _opal$(OBJ_SUFFIX)
  STDCCFLAGS += -DUSE_OPAL

  include $(OPALDIR)/opal_inc.mak
else
  VPATH_CXX := h323lib

  SOURCES += t38protocol.cxx audio_chan.cxx g7231_fake.cxx h323ep.cxx

  ifndef OPENH323DIR
    OPENH323DIR=$(HOME)/openh323
  endif

  include $(OPENH323DIR)/openh323u.mak

  ifdef NO_PBOOLEAN
    STDCCFLAGS += -DPBoolean=BOOL
  endif
endif

#
# If defined COUT_TRACE then enable duplicate the
# output of myPTRACE() to cout
#
ifdef COUT_TRACE
STDCCFLAGS += -DCOUT_TRACE
endif

#
# By default the code will be optimized for original ASN.1
# sequence in T.38 (06/98) Annex A (it's optimal if you use
# --old-asn option).
# If defined OPTIMIZE_CORRIGENDUM_IFP then the code will be
# optimized for CORRIGENDUM No. 1 fix (it's optimal if you
# do not use --old-asn option).
#
ifdef OPTIMIZE_CORRIGENDUM_IFP
STDCCFLAGS += -DOPTIMIZE_CORRIGENDUM_IFP
endif

#
# If defined MYPTRACE_LEVEL=N then myPTRACE() will
# output the trace with level N
#
ifdef MYPTRACE_LEVEL
STDCCFLAGS += -DMYPTRACE_LEVEL=$(MYPTRACE_LEVEL)
endif

#
# If defined FD_TRACE_LEVEL=N then myPTRACE() will
# output the warnings on level N for big file descriptors
#
ifdef FD_TRACE_LEVEL
STDCCFLAGS += -DFD_TRACE_LEVEL=$(FD_TRACE_LEVEL)
endif

#
# If defined PROCESS_PER_THREAD then
#  - PID will be used in thread name rather then TID
#  - CPU usage will be traced
#
ifdef PROCESS_PER_THREAD
STDCCFLAGS += -DPROCESS_PER_THREAD
endif

#
# If defined REPEAT_INDICATOR_SENDING then t38modem
# will repeat indicator sending on idle
#
ifdef REPEAT_INDICATOR_SENDING
STDCCFLAGS += -DREPEAT_INDICATOR_SENDING
endif

#
# If defined USE_UNIX98_PTY then t38modem will use
# Unix98 scheme for pty devices.
# If defined USE_LEGACY_PTY or not defined USE_UNIX98_PTY
# then t38modem will use legacy scheme for pty devices.
# Both schemes cen be used simultaneously.
#
ifdef USE_UNIX98_PTY
  STDCCFLAGS += -DUSE_UNIX98_PTY

  ifdef USE_LEGACY_PTY
    STDCCFLAGS += -DUSE_LEGACY_PTY
  endif
else
  STDCCFLAGS += -DUSE_LEGACY_PTY
endif

#
# If defined ALAW_132_BIT_REVERSE then t38modem
# will use reversed bit order for +VSM=132
# (workaround for mgetty-voice)
#
ifdef ALAW_132_BIT_REVERSE
  STDCCFLAGS += -DALAW_132_BIT_REVERSE
endif

