/*
 * README
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 */

1. Introduction
---------------

What is t38modem?

From your fax or voice application view point it's a fax/voice modem pool.
From IP network view point it's a H.323/SIP endpoint with T.38 fax support.
From your view point it's a gateway between an application and IP network.

The homepage for t38modem project is http://t38modem.sourceforge.net/.

2. Building
-----------

2.1. Building for Unix
----------------------

Building with Open H323 Library:

  $ export PWLIBDIR=$path_to_libs/pwlib
  $ export OPENH323DIR=$path_to_libs/openh323
  $ make NO_PBOOLEAN=1 opt

Building with H323 Plus Library:

  $ export PTLIBDIR=$path_to_libs/ptlib
  $ export OPENH323DIR=$path_to_libs/h323plus
  $ make opt

Building with Open Phone Abstraction Library (OPAL):

  $ export PTLIBDIR=$path_to_libs/ptlib
  $ export OPALDIR=$path_to_libs/opal
  $ make USE_OPAL=1 opt

2.2. Building for Windows
-------------------------

Building with Open H323 Library:

  Start Microsoft Visual C++ 2005 with h323lib\t38modem_openh323_2005.vcproj file.
  Set Active Configuration to "t38modem - Win32 Release".
  Build t38modem.exe.

Building with H323 Plus Library:

  Start Microsoft Visual C++ 2005 with h323lib\t38modem_2005.vcproj file.
  Set Active Configuration to "t38modem - Win32 Release".
  Build t38modem.exe.

Building with Open Phone Abstraction Library (OPAL):

  Start Microsoft Visual C++ 2005 with opal\t38modem_2005.vcproj file.
  Set Active Configuration to "t38modem - Win32 Release".
  Build t38modem.exe.

3. Examples
-----------

To get info about t38modem command line syntax enter:

  $ t38modem --help


3.1. Starting
-------------

Starting with Open H323 Library or H323 Plus Library:

  $ t38modem -n -o trace.log -p ttyx0,ttyx1 --old-asn \
                               --route 0@127.0.0.1 --route all@172.16.33.21

This will create two modems (/dev/ttyx0 and /dev/ttyx1) and H.323 endpoint.
If dialed number begins with '0' then it will be routed to a local host
(leading '0' will be discarded). Other dialed numbers will be routed to
172.16.33.21.

Starting with OPAL:

  $ t38modem -n -o trace.log -p ttyx0,ttyx1 \
                               --route "modem:0.*=h323:<dn!1>@127.0.0.1" \
                               --route "modem:1.*=sip:<dn>@172.16.33.20" \
                               --route "modem:2.*=h323:<dn>@172.16.33.21" \
                               --route "h323:.*=modem:<dn>" \
                               --route "sip:.*=modem:<dn>"

This will create two modems (/dev/ttyx0 and /dev/ttyx1) and H.323 and SIP endpoints.
If dialed number begins with '0' then it will be routed to a local host.
If dialed number begins with '1' then it will be routed to SIP endpoint 172.16.33.20.
If dialed number begins with '2' then it will be routed to H.323 endpoint 172.16.33.21.
Leading '0', '1' and '2' will be discarded.

Q. I try to use T38modem, but after run "t38modem -p ttyx0" I get a message
   "Could not open /dev/ptyx0: No such file or directory".
A. Looks like you don't have legacy PTY devices compiled in your kernel.
   You need to re-compile the kernel with 'Legacy PTY Support'.
   Alternatively, you can build t38modem with USE_UNIX98_PTY=1 option and use
   -p +/dev/ttyx0,+/dev/ttyx1 instead of -p ttyx0,ttyx1.

FreeBSD Users: You need to use  -p ttypa,ttypb instead of -p ttyx0,ttyx1.
               Remember to replace ttyx0 with ttypa and ttyx1 with ttypb
               when following the rest of these instructions.
               This will create two modems /dev/ttypa and /dev/ttypb.

Windows Users: You need two COM ports connected via Null-modem cable to create one modem.
               If your COM1 connected to COM2 and COM3 connected to COM4 you need to use
               -p \\.\COM2,\\.\COM4 instead of  -p ttyx0,ttyx1.
               This will create two modems COM1 and COM3.
               Q. How to use t38modem without additional COM port hardware on Windows?
               A. Replace a pair of physical COM ports with a pair of virtual COM ports.
                  See http://com0com.sourceforge.net/ project for details.
               Q. What model of modem to select in Add Hardware Wizard?
               A. Select "Standard 1440 bps Modem".

Cisco Users:   Possible additionaly you will need to use --h245tunneldisable option.

3.2. Testing (you need two consoles)
------------------------------------
(FreeBSD users - remeber to use /dev/ttypa and /dev/ttypb with 'cu -l')
(Windows users - use COM1 and COM3 with HyperTerminal)

$ cu -l /dev/ttyx0	    $ cu -l /dev/ttyx1
Connected.		    Connected.
<-- at		            <-- at
--> OK                      --> OK
                            (wait at least 10 secs)
<-- atdt012345

                            -->
                            --> RING
                            -->
                            --> RING
                            <-- ati9
                            --> NDID = 12345
                            --> OK
                            -->
                            --> RING
                            -->
                            --> RING
                            <-- ata
--> CONNECT                 --> CONNECT
<-- x
--> OK
<-- ath
--> OK
                            -->
                            --> ERROR
<-- at
--> OK
                            <-- at
                            --> OK
...                         ...


3.3. Example of Cisco config (loopback)
---------------------------------------

10.0.2.12 --> Cisco port 2:D --E1-cable--> Cisco port 3:D --> 10.0.2.12

dial-peer voice 3340 voip
 incoming called-number 3334....
 codec g711alaw
 fax rate 14400
 fax protocol t38 ls-redundancy 0 hs-redundancy 0

dial-peer voice 3341 pots
 destination-pattern 3334....
 port 2:D
 forward-digits 7

dial-peer voice 3342 pots
 incoming called-number 334....
 direct-inward-dial
 port 3:D
exit

dial-peer voice 3343 voip
 destination-pattern 334....
 session target ipv4:10.0.2.12
 codec g711alaw
 fax rate 14400
 fax protocol t38 ls-redundancy 0 hs-redundancy 0
exit

3.4. Example of HylaFAX modem config files
------------------------------------------

Copy HylaFAX/etc/config.ttyx to HylaFAX's etc directory

Create simbolic links:

config.ttyx0 -> config.ttyx
config.ttyx1 -> config.ttyx

Start HylaFAX with new modems:

$ .../faxgetty -D ttyx0
$ .../faxgetty -D ttyx1

(FreeBSD users - don't forget we are using ttypa and ttypb)

4. AT commands specific to t38modem
-----------------------------------

4.1. AT#CID command
-------------------

4.1.1 Reporting Caller ID and DID after the first RING
------------------------------------------------------

#CID=0	- disable Caller ID and DID reporting (default).
#CID=1	- enable only Caller ID reporting.
#CID=10	- enable Caller ID and DID reporting.
#CID=11	- enable only DID reporting.

Example:

<-- AT#CID=10
--> OK
--> RING
--> DATE = <MMDD>              ; Caller ID
--> TIME = <HHMM>              ; Caller ID
--> NMBR = <calling number>    ; Caller ID
--> NAME = <calling name>      ; Caller ID
--> NDID = <called number>     ; DID
--> RING
--> RING

4.2. ATI command
----------------

4.2.1 calling/called number reporting
-------------------------------------

I8	- reports calling number for last incoming call.
I9	- reports called number for last incoming call.

Example:

<-- ATI8I9
--> NMBR = <calling number>
--> NDID = <called number>
--> OK

4.3. ATD command
----------------

4.3.1 Fax mode modifiers
------------------------

F - force fax mode (T.38 or G.711 pass-trough) after dialing
    (with OPAL can be overriden by route option OPAL-Force-Fax-Mode=false).
V - do not force fax mode after dialing (default)
    (with OPAL can be overriden by route option OPAL-Force-Fax-Mode=true).

Examples:

<-- ATDF<user's number>

    force fax mode after dialing but user can override it by
    inserting V into <user's number>.

<-- ATD<user's number>V

    do not force fax mode after dialing and user can't override it
    by inserting F into <user's number>.

4.3.2 calling/called number modifiers
-------------------------------------

L     - purge calling number and start dialing it.
T,P,D - continue to dial of called number if dialing of calling number was started.

If calling number is empty after processing ATD command then t38modem's
local party number used.

The calling number is a string of 0 or more of the characters:

  "0 1 2 3 4 5 6 7 8 9 * #"

Note: dialing of calling number is not allowed in online command state.

The called number is a string of 0 or more of the characters:

  "0 1 2 3 4 5 6 7 8 9 * # + A B C D"

Note: '+' will be ignored in online command state or after '@' modifier.

Examples:

<-- ATD<user's number>

    calling number is t38modem's local party number but user can override
    it by inserting L<user's calling number> into <user's number>

<-- ATD<user's number>L

    calling number is t38modem's local party number and user can't override
    it by inserting L<user's calling number> into <user's number>

<-- ATDL<calling number>T<user's number>

    calling number is <calling number> but user can override
    it by inserting L<user's calling number> into <user's number>

<-- ATD<user's number>L<calling number>

    calling number is <calling number> and user can't override
    it by inserting L<user's calling number> into <user's number>

4.3.3 Wait for answer modifier
------------------------------

@ - dial the called number part collected before first @, wait till the call
    answered and play the called number part collected after last @.
    Implicitly '@' will continue dialing of called number (see 4.3.2).

Examples:

<-- ATS8?
--> 002
--> OK
<-- ATD12345@,,,4321

    dial number 12345, wait till the call answered, wait 6 seconds
    (3 commas * 2 seconds), play digits 4321.

<-- ATD12345@,,,4321@

    dial number 12345.

4.4. AT#DFRMC command
---------------------

4.4.1 Set delay for CONNECT result code for AT+FRM command
----------------------------------------------------------

#DFRMC=0	- disable delay (default).
#DFRMC=25	- set delay to 250 ms.

4.5. AT#HCLR command
--------------------

4.5.1 Set clear call mode for ATH command
-----------------------------------------

#HCLR=0	- ATH command will not clear not answered incoming call (default).
#HCLR=1	- ATH command will clear not answered incoming call.

4.6. AT#CIDFMT command
----------------------

AT#CIDFMT command allows to set value format for NAME and NMBR tags of Caller ID.

Syntax:

  #CIDFMT=[<name format>][,[<nmbr format>]]

If a format element is empty then its value will be untouched.

Subparameter <name format>:

  0   - report "NAME = <calling name>" (default)
  1   - report "NAME = "
  2   - report "NAME = <called number>"
  3   - report "NAME = <called number> <- <calling name>"

Subparameter <nmbr format>:

  0   - report "NMBR = <calling number>" (default)
  1   - report "NMBR = "
  2   - report "NMBR = <called number>"
  3   - report "NMBR = <called number>#<calling number>"

