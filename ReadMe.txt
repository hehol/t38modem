/*
 * $Id: ReadMe.txt,v 1.1 2002-01-01 23:06:54 craigs Exp $
 *
 * T38FAX Pseudo Modem
 *
 * Original author: Vyacheslav Frolov
 *
 * $Log: ReadMe.txt,v $
 * Revision 1.1  2002-01-01 23:06:54  craigs
 * Initial version
 *
 * Revision 1.1  2002/01/01 23:06:54  craigs
 * Initial version
 *
 */

1. Building
-----------

1.1. Patching openh323

Some of the Request Mode methods of openh323 shold be virtual
The folloving patch do virtual all of them.

$ export POSIXLY_CORRECT=1
$ patch -d $OPENH323DIR/src/include -p0 --verbose < virtual.patch

----------------------[cut]----------------------
Index: h323con.h
===================================================================
RCS file: /home/cvsroot/openh323/include/h323con.h,v
retrieving revision 1.15
diff -u -r1.15 h323con.h
--- h323con.h	2001/12/15 08:09:54	1.15
+++ h323con.h	2001/12/21 14:02:45
@@ -1398,11 +1398,11 @@
   //@{
     /**Make a request to mode change to remote.
       */
-    void RequestModeChange();
+    virtual void RequestModeChange();
 
     /**Received request for mode change from remote.
       */
-    BOOL OnRequestModeChange(
+    virtual BOOL OnRequestModeChange(
       const H245_RequestMode & pdu,     /// Received PDU
       H245_RequestModeAck & ack,        /// Ack PDU to send
       H245_RequestModeReject & reject   /// Reject PDU to send
@@ -1410,13 +1410,13 @@
 
     /**Received acceptance of last mode change request from remote.
       */
-    void OnAcceptModeChange(
+    virtual void OnAcceptModeChange(
       const H245_RequestModeAck & pdu  /// Received PDU
     );
 
     /**Received reject of last mode change request from remote.
       */
-    void OnRefusedModeChange(
+    virtual void OnRefusedModeChange(
       const H245_RequestModeReject * pdu  /// Received PDU, if NULL is a timeout
     );
   //@}
Index: h323neg.h
===================================================================
RCS file: /home/cvsroot/openh323/include/h323neg.h,v
retrieving revision 1.26
diff -u -r1.26 h323neg.h
--- h323neg.h	2001/09/12 01:54:45	1.26
+++ h323neg.h	2001/12/21 14:02:45
@@ -336,12 +336,12 @@
   public:
     H245NegRequestMode(H323EndPoint & endpoint, H323Connection & connection);
 
-    BOOL StartRequest();
-    BOOL HandleRequest(const H245_RequestMode & pdu);
-    BOOL HandleAck(const H245_RequestModeAck & pdu);
-    BOOL HandleReject(const H245_RequestModeReject & pdu);
-    BOOL HandleRelease(const H245_RequestModeRelease & pdu);
-    void HandleTimeout(PTimer &, INT);
+    virtual BOOL StartRequest();
+    virtual BOOL HandleRequest(const H245_RequestMode & pdu);
+    virtual BOOL HandleAck(const H245_RequestModeAck & pdu);
+    virtual BOOL HandleReject(const H245_RequestModeReject & pdu);
+    virtual BOOL HandleRelease(const H245_RequestModeRelease & pdu);
+    virtual void HandleTimeout(PTimer &, INT);
 
   protected:
     BOOL awaitingResponse;
----------------------[cut]----------------------

1.2. Compiling
-------------

$ make debug

2. Examples:
-----------

2.1. Starting
-------------

$ ./obj_linux_x86_d/pmodem -k -m sample_message -n -o trace.log -f -p ttyx0,ttyx1 --remote 172.16.33.21

Creates two modems /dev/ttyx0 and /dev/ttyx1
If dialed number begins with '9' then it will be routed to 172.16.33.21 ('9' will be stripped).
If not then it will be routed to local host.

2.2. Testing (you need two consoles)
------------------------------------

$ cu -l /dev/ttyx0	    $ cu -l /dev/ttyx1
Connected.		    Connected.
<-- at		            <-- at
--> OK                      --> OK
                            (wait at least 10 secs)
<-- atdt12345

                            --> 
                            --> RING
                            --> 
                            --> RING
                            <-- ati9
                            --> NDID=12345
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


2.3. Example of Cisco config (loopback)
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

2.4. Example of HylaFAX modem config filas

Copy HylaFAX/etc/config.ttyx to HylaFAX's etc directory

Create simbolic links:

config.ttyx0 -> config.ttyx
config.ttyx1 -> config.ttyx

Start HylaFAX with new modems:

$ .../faxgetty -D ttyx0
$ .../faxgetty -D ttyx1

