SimpleSipxTerm
=====================================================================

Important notice: there is a bug in SimpleSipxTerm that it does not exit correctly
                  when closing application. So when closing, check via TaskManager
                  if it has realy shut down...


What is this
------------
This project is a very quick and ugly piece of MFC application that implements
a simple SIP Softphone based on top of SipXtapi. Up to 4 lines (accounts) can
be assigned per instance and they are showing up as separate terminals.

Each instance will ask which local ports to use and how many accounts there 
should be created. It will then use the starting DNR to up-count the DNR for 
each next terminal.

It was made do to some quick check and testing with our 3rd party PBX'es 
and trying out quickly some scenarios on top of sipXTapi.

