This folder contains some headers that were developed for NetBee and, for some reasons, are not integrated in the current version of the library.

It includes classes to abstract the NetVM interface and a set of classes that were intended to create a clean interface to packet capture. That class was then partially replaced by the PacketEngine, but some of the feature of the nbPacketCapture() class are still missing.


This is the description of the Packet Processing classes as it was in the Doxygen main page.

	<tr>
		<td valign="top">\ref NetBeePacketProcessing</td>
		<td> <p>
		This section defines some classes that can be used to process network packets by means of the 
		capabilities of the NetVM virtual machine. These classes integrates the processing capabilities of 
		the NetVM with the "real" external environment such as physical network interfaces for receiving packets.<p>
		This section provides some classes that can be used to list the available adapters, capture packet from a 
		network device, filter traffic and delivering it to the application, and more.<p>
		For standard applications, programmers do not have to use the primitives defined in section \ref NetBeeNetVM. 
		Classes defined in this section usually cover most of the needs of standard network applications.</td>
	</tr>

