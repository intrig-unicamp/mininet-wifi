#!/bin/bash

#
# Copyright (c) 2002 - 2013
# NetGroup, Politecnico di Torino (Italy)
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without 
# modification, are permitted provided that the following condition 
# is met:
# 
# Neither the name of the Politecnico di Torino nor the names of its 
# contributors may be used to endorse or promote products derived from 
# this software without specific prior written permission. 
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY 
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
# 

netpdl_file="../../bin/netpdl.xml"
protocol_path="../netpdl-dissectors/"
schema_path="../xml-schema/netpdl-schema.xsd"
schema_file="../../bin/netpdl.xsd"

echo
echo "****************************************************************************"
echo "* NetPDL Database creation"
echo "****************************************************************************"
echo
echo "Creating a unique NetPDL file with all the available dissectors."
echo "The output file will be copied in " $netpdl_file "."
echo
echo "Processing..."


#List of protocols. Note that, for each protocol, in ../netpdl-dissectors there must 
#be a file having "protocol_name.xml" as a name
declare -a protocols=(
	"startproto"
	#Link-Layer Protocols
	"ethernet" "fddi" "tokenring" "vlan" "llc" "stp" "etherpadding" "mpls" "linux"
	#BlueTooth-related Protocols
	"hci_acl_data" "hci_command" "hci_error_message" "hci_event" "hci_negotiation"
	"hci_packet_type" "hci_sco_data" "hci_unknown_type" "l2cap_command"
	#IPX-related Protocols
	"ipx" "ripx" "ipx_sap" "ncp"
	#Network-Layer Protocols
	"bootp" "dhcp" "icmp" "igmp" "arp" "ip" "ipfrag" "ipv6" "icmp6" "dns" "dns_tcp"
	#Transport-Layer Protocols
	"tcp" "udp"
	#Routing Protocols
	"rip" "rip6" "igrp" "eigrp" "ospf" "ospf6" "bgp" "dvmrp" "pim" "pim6"
	#P2P Protocols
	"ares" "winmx" "bittorrent" "edonk" "edonkudp" "fasttrack" "gnutella" "dcpp"
	"peerenabler" "slsk" "tvants" "sopcast" "pplive"	
	#Messaging Protocols
	"skype" "yahoomsg" "msnmsg" "irc"
	#Mail Protocols
	"imap" "pop3" "simap4" "smtp" "spop3" "ssmtp"
	#RPC Protocols
	"dce_rpc_tcp" "dce_rpc_udp" "onc_rpc_udp" "mnt" "rpcbind"
	#Well-Known Application Protocols
	"ftp" "ftpdata" "http" "rtsp" "sip" "stun" "snmp" "telnet"
	#Resource Sharing
	"netbeui" "smb" "cifs_browser" "netbios" "netbiosdgm" "netbiosssn" "samba" "nfs"
	#Other Protocols
	"ismp" "btsdp" "cdp" "hsrp" "vrrp" "ccp" "chap" "gre" "ipcp" "lcp" "ppp" "pptp"
	"esp" "kerberos" "ntp" "pppoe" "pppoed" "radius" "rpcap" "rtcp" "rtp" "auth"
	"cldap" "icp" "ipp" "isakmp" "jrmi" "ldap" "rdp" "ssdp" "ssh" "ssl" "syslog"
	"xmpp" "fsecure" "sql" "oracle_sql" "rfb" "wins" "ms_sql_monitor" "ms_sql_server"
	"nt_security_log" "cvs" "pcanywhere" "openflow"
	#Miscellaneous
	"defaultproto" "visualization"
	)
					
echo "<?xml version=\"1.0\" encoding=\"utf-8\"?>" > $netpdl_file
echo "<netpdl name=\"nbee.org NetPDL Database\" version=\"0.2\" creator=\"nbee.org\" date=\""`date +"%d-%m-%y"`"\"" >> $netpdl_file
#Comment the following two lines if you don't want to add the XSD schema to the NetPDL file.
echo "    xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\"" >> $netpdl_file
echo "    xsi:noNamespaceSchemaLocation=\"netpdl.xsd\"" >> $netpdl_file
echo ">" >> $netpdl_file
echo >> $netpdl_file


for i in ${protocols[@]}
do
   echo "    " $protocol_path$i".xml"
   cat $protocol_path$i.xml >> $netpdl_file
done

echo "</netpdl>" >> $netpdl_file

echo
echo "The NetPDL database has been created correctly."
echo "Now copying the XSD schema file..."

cp $schema_path $schema_file

echo
echo "The XSD schema file has been copied correctly."
echo "Please validate the output file with the proper XSD schema before use."
echo
