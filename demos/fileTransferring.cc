/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* Licensing:
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 * 
 * Author: 
 * 	Anderson dos Santos Paschoalon <anderson.paschoalon@gmail.com>
 * 
 * Modified:
 * 29/09/2016	first version
 * 
 * About:
 * 	Simulate the tranference of a file of 62.6 Mbytes trough two wifi stations.
 *
 * Topology:
 *             Wifi 10.1.1.0
 *
 *       * <-2.72m-> *  <-4.08m-> *
 *      n0           AP           n1
 *   (source)                  (client)
 * 
 */

#include <stdio.h>
#include <string>
#include <iostream>
#include "ns3/core-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/csma-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"  

using namespace ns3;
using namespace std;

int main(int argc, char *argv[])
{

	//constants
	string application = "bulk";
	uint32_t port = 19;
	uint64_t fileSize = 62.6 * 1024 * 1024;
	uint32_t start_time = 0;
	uint32_t stop_time = 1000;
	uint32_t delta_time = 1;
	uint32_t packetSize = 1470;
	uint32_t txPowerStart = 15;
	uint32_t txPowerEnd = 15;
	//variables
	unsigned int txPacketsum = 0;
	unsigned int rxPacketsum = 0;
	unsigned int DropPacketsum = 0;
	unsigned int LostPacketsum = 0;
	double Delaysum = 0;
	double Jittersum = 0;

	//
	// Network elements
	//

	//Nodes
	NodeContainer ap;
	NodeContainer stas;
	stas.Create(2);
	ap.Create(1);
	//Devices
	NetDeviceContainer staDevs;
	NetDeviceContainer apDev;
	//Channel and physical layer
	YansWifiChannelHelper channel;
	channel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
	channel.AddPropagationLoss("ns3::FriisPropagationLossModel", "Frequency",
			DoubleValue(5e9)); // Free space propagation model
	YansWifiPhyHelper phy = YansWifiPhyHelper::Default();
	phy.SetChannel(channel.Create());
	phy.Set("ChannelNumber", UintegerValue(1));
	phy.Set("TxPowerStart", DoubleValue(txPowerStart)); //dbm
	phy.Set("TxPowerEnd", DoubleValue(txPowerEnd)); //dbm
	//Wifi and MAC layer
	WifiHelper wifi = WifiHelper::Default();
	wifi.SetRemoteStationManager("ns3::AarfWifiManager");
	wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
	NqosWifiMacHelper mac = NqosWifiMacHelper::Default();

	//
	// Create network
	//

	Ssid ssid = Ssid("wifi-file-transference");
	//mobile nodes config
	mac.SetType("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing",
			BooleanValue(false));
	staDevs = wifi.Install(phy, mac, stas);
	//AP config
	mac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid));
	apDev = wifi.Install(phy, mac, ap);

	//
	// Position Allocation and mobility
	//

	MobilityHelper mobility;
	Ptr < ListPositionAllocator > positionAlloc = CreateObject<
			ListPositionAllocator>();

	positionAlloc->Add(Vector(0.0, 0.0, 0.0)); //n0
	positionAlloc->Add(Vector(4.08 + 2.72, 0.0, 0.0)); //n1
	positionAlloc->Add(Vector(2.72, 0.0, 0.0)); //AP

	mobility.SetPositionAllocator(positionAlloc);
	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
	mobility.Install(stas);
	mobility.Install(ap);

	//
	// Internet configuration
	//

	InternetStackHelper stack;
	stack.Install(ap);
	stack.Install(stas);

	Ipv4AddressHelper address;
	Ipv4InterfaceContainer wifiInterfaces;

	address.SetBase("10.1.1.0", "255.255.255.0");
	wifiInterfaces = address.Assign(staDevs);

	Ipv4GlobalRoutingHelper::PopulateRoutingTables();

	//
	// Application
	//

	printf("Starting bulk ftp-like application\n");

	Config::SetDefault("ns3::TcpSocket::SegmentSize",
			UintegerValue(packetSize));

	Address apLocalAddress(
			InetSocketAddress(wifiInterfaces.GetAddress(1), port));
	BulkSendHelper source("ns3::TcpSocketFactory", apLocalAddress);

	source.SetAttribute("MaxBytes", UintegerValue(fileSize));
	ApplicationContainer sourceApps = source.Install(stas.Get(0));
	sourceApps.Start(Seconds(start_time + delta_time));
	sourceApps.Stop(Seconds(stop_time));

	PacketSinkHelper sink("ns3::TcpSocketFactory",
			InetSocketAddress(Ipv4Address::GetAny(), port));
	ApplicationContainer sinkApps = sink.Install(stas.Get(1));
	sinkApps.Start(Seconds(start_time));
	sinkApps.Stop(Seconds(stop_time));

	//
	// Simulation and data collection
	//

	Simulator::Stop(Seconds(stop_time));

	phy.EnablePcap("ap", apDev.Get(0));
	phy.EnablePcap("sender", staDevs.Get(0));
	phy.EnablePcap("receiver", staDevs.Get(1));

	FlowMonitorHelper flowHelper;
	Ptr < FlowMonitor > monitor;
	monitor = flowHelper.InstallAll();

	Simulator::Run();

	std::map < FlowId, FlowMonitor::FlowStats > stats = monitor->GetFlowStats();

	monitor->SerializeToXmlFile("anderson-samira.xml", true, true);
	Ptr < Ipv4FlowClassifier > classifier = DynamicCast < Ipv4FlowClassifier
			> (flowHelper.GetClassifier());

	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i =
			stats.begin(); i != stats.end(); ++i)
	{
		txPacketsum += i->second.txPackets;
		rxPacketsum += i->second.rxPackets;
		LostPacketsum += i->second.lostPackets;
		DropPacketsum += i->second.packetsDropped.size();
		Delaysum += i->second.delaySum.GetSeconds();
		Jittersum += i->second.jitterSum.GetSeconds();

	}

	cout << "txPacketsum = " << txPacketsum << endl;
	cout << "rxPacketsum = " << rxPacketsum << endl;
	cout << "LostPacketsum = " << LostPacketsum << endl;
	cout << "DropPacketsum = " << DropPacketsum << endl;
	cout << "Delaysum = " << Delaysum << endl;
	cout << "Jittersum = " << Jittersum << endl;
	cout << "latency = " << Delaysum / (rxPacketsum) << endl;

	Simulator::Destroy();

	return 0;
}

