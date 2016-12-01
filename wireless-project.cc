/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/* this is a sample code without hidden terminal */
#include "ns3/core-module.h"
#include "ns3/propagation-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-module.h"
#include <fstream>		// newly added
#include <iostream>

using namespace ns3;
/// Run single 10 seconds experiment with enabled or disabled RTS/CTS mechanism
double randomStartTime(double seed);
double experiment (bool enableCtsRts, uint32_t numWifiSta, enum ns3::WifiPhyStandard standard, bool isHiddenTerminal, std::string cbr)
{
	// uint32_t numWifiSta = 5;
  	uint32_t numAP = 1;
	// standard = WIFI_PHY_STANDARD_80211a;
	// 0. Enable or disable CTS/RTS, hidden terminal?
  	UintegerValue ctsThr = (enableCtsRts ? UintegerValue (100) : UintegerValue (2200));
  	Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);

  	// 1. Create n number of nodes and AP
  	NodeContainer wifiStaNodes;
  	wifiStaNodes.Create(numWifiSta);
  	NodeContainer wifiAPNodes;
  	wifiAPNodes.Create(numAP);

  	// 2. Setup wifi channel, default constant, transmission range is set here?
  	YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default(); // default PHY values
  	YansWifiChannelHelper wifiChannel;
  	wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
	if (!isHiddenTerminal)
  		wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(100));	// propagation loss depend only on distance, no hidden terminals
	else
		wifiChannel.AddPropagationLoss("ns3::RangePropagationLossModel", "MaxRange", DoubleValue(1)); 	// reduce the transmission range in order to create hidden terminal

  	wifiPhy.SetChannel(wifiChannel.Create());
  	// 5. Install wireless devices

  	WifiHelper wifi;
  	wifi.SetStandard (standard);			// standard change parameters, right now it is 802.11a
	if (standard == WIFI_PHY_STANDARD_80211a)	// different remote standards needs to be set differently, confused part
	{
  		wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
        		                        "DataMode",StringValue ("OfdmRate6Mbps"),
        	        	                "ControlMode",StringValue ("OfdmRate6Mbps"));
	}
	else if (standard == WIFI_PHY_STANDARD_80211n_2_4GHZ)
  {
      wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                    "DataMode", StringValue ("OfdmRate54Mbps"),
                                    "ControlMode", StringValue ("OfdmRate54Mbps"));
  }
  else
	{
		wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager",
                                  "DataMode",StringValue ("DsssRate2Mbps"),
                                  "ControlMode",StringValue ("DsssRate2Mbps"));
	}
  	Ssid ssid = Ssid("wifi-default");

  	NqosWifiMacHelper wifiMac = NqosWifiMacHelper::Default();

  	wifiMac.SetType ("ns3::StaWifiMac", "Ssid", SsidValue(ssid), "ActiveProbing", BooleanValue(false)); // use stawifi
  	NetDeviceContainer staDevices;
  	staDevices = wifi.Install (wifiPhy, wifiMac, wifiStaNodes);
  	wifiMac.SetType("ns3::ApWifiMac", "Ssid", SsidValue(ssid), "BeaconGeneration", BooleanValue(true));
  	NetDeviceContainer apDevices;
  	apDevices = wifi.Install(wifiPhy, wifiMac, wifiAPNodes);

  	// Add mobility on AP node
  	MobilityHelper mobility;
  	// Set AP stationary, do not change on AP mobility stuff, AP is set to stationary
  	mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");	// coordinate (0,0) on (x,y) axis for AP node
  	mobility.Install(wifiAPNodes);

  	// Set STA mobility
  	/*Ptr<UniformRandomVariable> rv = CreateObject<UniformRandomVariable>();
	uniformed mobility model on a circle, no hidden terminal here, all within 50 meter circle */
	if (!isHiddenTerminal)
  		mobility.SetPositionAllocator("ns3::UniformDiscPositionAllocator","X", StringValue("0.0"), "Y", StringValue("0.0"), "rho", StringValue("50.0"));
	else
		mobility.SetPositionAllocator ("ns3::GridPositionAllocator",
   	                                       "MinX", DoubleValue (-0.5),
                                               "MinY", DoubleValue (0.0),
                                               "DeltaX", DoubleValue (1.1),			// broadcasting range is 1.0, causing adjacent terminals to be hidden terminals
                                               "DeltaY", DoubleValue (0.0005),			// nodes position growing up, vertical grow very small, still comming to AP
                                               "GridWidth", UintegerValue (2),			// 2 nodes per row
                                               "LayoutType", StringValue ("RowFirst"));		// row priority first
	// put the nodes on a grid system model, make sure they are within AP transmission range but they are not able to overhear each other!!!!!!!!!!!!!!!!!!!!!!!!!!
	// set grid system layout in this way, see what output it will get. propagation loss is 1 meter
  	//// mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  	mobility.Install(wifiStaNodes);		// install STA node
  	//// print locations
  	Ptr<MobilityModel> apmob = wifiAPNodes.Get(0)->GetObject<MobilityModel>();
  	//Vector posap = apmob->GetPosition();
  	//std::cout<<"AP cordinates: x = "<<posap.x<<", y = "<<posap.y<<"\n";
  	for (uint32_t k=0; k<numWifiSta; k++)
  	{
 		Ptr<MobilityModel> mob = wifiStaNodes.Get(k)->GetObject<MobilityModel>();
  		//Vector pos = mob->GetPosition();
  		//std::cout<<"Node "<<k<<" cordinates: x = "<<pos.x<<", y = "<<pos.y<<"\n";
  	}
  	// Install protocol stack
  	InternetStackHelper stack;
  	stack.Install(wifiStaNodes);
  	stack.Install(wifiAPNodes);
  	// uncomment the following to have athstats output
  	// AthstatsHelper athstats;
  	// athstats.EnableAthstats(enableCtsRts ? "rtscts-athstats-node" : "basic-athstats-node" , nodes);

  	// uncomment the following to have pcap output
  	// wifiPhy.EnablePcap (enableCtsRts ? "rtscts-pcap-node" : "basic-pcap-node" , nodes);

  	// 6. Install TCP/IP stack & assign IP addresses
  	Ipv4AddressHelper ipv4;
  	ipv4.SetBase ("10.0.0.0", "255.0.0.0");
  	ipv4.Assign (staDevices);
  	ipv4.Assign(apDevices);

  	Ptr<Node> apn = wifiAPNodes.Get(0);
  	Ptr<Ipv4> ipv4add = apn->GetObject<Ipv4>();
  	Ipv4Address APaddr = ipv4add->GetAddress(1,0).GetLocal();

  	double starttime = 1.000000;
  	double stoptime = 10.000000;
  	double duration = stoptime - starttime;
  	ApplicationContainer cbrApps;
  	uint16_t cbrPort = 12345;
  	StringValue cbrate = StringValue(cbr);		// cbr traffic rate set to change
  	OnOffHelper onOffHelper ("ns3::UdpSocketFactory", InetSocketAddress (APaddr, cbrPort));
  	onOffHelper.SetAttribute ("PacketSize", UintegerValue (1400));
  	onOffHelper.SetAttribute ("OnTime",  StringValue ("ns3::ConstantRandomVariable[Constant=1]"));
  	onOffHelper.SetAttribute ("OffTime", StringValue ("ns3::ConstantRandomVariable[Constant=0]"));

  	ApplicationContainer pingApps;
  	uint16_t echoPort = 9;
  	UdpEchoClientHelper echoClientHelper(APaddr, echoPort);
  	echoClientHelper.SetAttribute("MaxPackets", UintegerValue(1));
  	echoClientHelper.SetAttribute("Interval", TimeValue(Seconds(0.1)));
  	echoClientHelper.SetAttribute("PacketSize", UintegerValue(10));

  	//slightly different start times
  	for (uint16_t j=0; j<numWifiSta; j++)
  	{
 	 	double randTime = randomStartTime(starttime);
         	double randEchoTime = randomStartTime(0.0);
  	 	//std::cout<<"Random start time of flow "<<j+1<<" is: "<<randTime<<"\n";
         	//std::cout<<"Echo random start time of flow "<<j+1<<" is: "<<randEchoTime<<"\n";
  	 	onOffHelper.SetAttribute ("DataRate", cbrate);
   	 	onOffHelper.SetAttribute ("StartTime", TimeValue (Seconds (randTime)));
  	 	cbrApps.Add (onOffHelper.Install (wifiStaNodes.Get(j)));
         	echoClientHelper.SetAttribute("StartTime", TimeValue(Seconds(randEchoTime)));
         	pingApps.Add(echoClientHelper.Install(wifiStaNodes.Get(j)));
        }

  	// 8. Install FlowMonitor on all nodes
  	FlowMonitorHelper flowmon;
  	Ptr<FlowMonitor> monitor = flowmon.InstallAll ();

  	// 9. Run simulation for 10 seconds
  	Simulator::Stop (Seconds (stoptime));
  	Simulator::Run ();

  	// 10. Print per flow statistics
  	monitor->CheckForLostPackets ();
  	Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier> (flowmon.GetClassifier ());
  	FlowMonitor::FlowStatsContainer stats = monitor->GetFlowStats ();
  	double totalThroughput = 0.0;
	// int p = 0;
  	for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin (); i != stats.end (); ++i)
    	{
      		//// first 2 FlowIds are for ECHO apps, we don't want to display them
      		//
      		//// Duration for throughput measurement is 9.0 seconds, since
      		//// StartTime of the OnOffApplication is at about "second 1"
      		//// and
      		//// Simulator::Stops at "second 10".
     		//// double totalThroughput = 0.0;
      		if (i->first > numWifiSta)
        	{
          		Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
          		std::cout << "Flow " << i->first - numWifiSta << " (" << t.sourceAddress << " -> " << t.destinationAddress << ")\n";
          		std::cout << "  Tx Packets: " << i->second.txPackets << "\n";
          		std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
          		std::cout << "  TxOffered:  " << i->second.txBytes * 8.0 / duration / 1000 / 1000  << " Mbps\n";
          		std::cout << "  Rx Packets: " << i->second.rxPackets << "\n";
          		std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
	  		      totalThroughput = totalThroughput+i->second.rxBytes*8.0/duration/1000/1000;
          		std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / duration / 1000 / 1000  << " Mbps\n";
        	}
    	}
  	std::cout<<"Total throughput: "<<totalThroughput<<" Mbps\n";
  	//// 11. Cleanup
  	Simulator::Destroy ();
	return totalThroughput;
}

double randomStartTime(double seed)
{
  	double min = seed;
  	double max = min+0.01;
  	Ptr<UniformRandomVariable> x = CreateObject<UniformRandomVariable>();
  	x->SetAttribute("Min", DoubleValue(min));
  	x->SetAttribute("Max", DoubleValue(max));
  	return x->GetValue();
}

int main (int argc, char **argv)
{

	// FileHelper fileHelper;
	// fileHelper.ConfigureFile ("project-result-count",FileAggregator::FORMATTED);
	// Set the labels for this formatted output file.
	// fileHelper.Set2dFormat ("No.of Nodes: %d\tThroughput: %.3f");
	std::ofstream result;
	result.open ("result.txt");
	if (result.is_open())
		std::cout << "file is open\n";
	double a;

	/*std::cout << "no hidden terminal, sta node of 1, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
	result << "no hidden terminal, sta node of 1, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
	a = experiment (false, 1, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 2, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 2, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 2, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 3, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 3, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 4, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 4, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 4, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 5, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 5, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 5, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 6, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 6, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 6, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 7, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 7, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 7, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 8, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 8, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 8, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 9, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 9, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 9, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 10, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 10, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 10, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 11, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 11, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 11, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	std::cout << "no hidden terminal, sta node of 12, under 802.11a, cbrate of 20Mbps, explore saturation: \n" << std::flush;
        result << "no hidden terminal, sta node of 12, under 802.11a, cbrate of 20Mbps, explore saturation: \n";
        a = experiment (false, 12, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "------------------------------------------------\n";
	result << "---------------------------------------------------\n";


 	std::cout << "no hidden terminal, sta node of 3, under 802.11a, cbrate of 20Mbps: \n" << std::flush;
	result << "no hidden terminal, sta node of 3, under 802.11a, cbrate of 20Mbps: \n";
	a = experiment (false, 3, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
	result << "throughput: " << a << "\n";
	std::cout << "------------------------------------------------\n";

	std::cout << "no hidden terminal, sta node of 6, under 802.11a, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 6, under 802.11a, cbrate of 20Mbps: \n";
        a = experiment (false, 6, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "no hidden terminal, sta node of 9, under 802.11a, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 9, under 802.11a, cbrate of 20Mbps: \n";
        a = experiment (false, 9, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "no hidden terminal, sta node of 12, under 802.11a, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 12, under 802.11a, cbrate of 20Mbps: \n";
        a = experiment (false, 12, WIFI_PHY_STANDARD_80211a, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	result << "---------------------------------------------------\n";

	std::cout << "no hidden terminal, sta node of 3, under 802.11b, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 3, under 802.11b, cbrate of 20Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211b, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "no hidden terminal, sta node of 6, under 802.11b, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 6, under 802.11b, cbrate of 20Mbps: \n";
        a = experiment (false, 6, WIFI_PHY_STANDARD_80211b, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "no hidden terminal, sta node of 9, under 802.11b, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 9, under 802.11b, cbrate of 20Mbps: \n";
        a = experiment (false, 9, WIFI_PHY_STANDARD_80211b, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "no hidden terminal, sta node of 12, under 802.11b, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 12, under 802.11b, cbrate of 20Mbps: \n";
        a = experiment (false, 12, WIFI_PHY_STANDARD_80211b, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	result << "---------------------------------------------------\n";

	std::cout << "no hidden terminal, sta node of 3, under 802.11g, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 3, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211g, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "no hidden terminal, sta node of 6, under 802.11g, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 6, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 6, WIFI_PHY_STANDARD_80211g, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "no hidden terminal, sta node of 9, under 802.11g, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 9, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 9, WIFI_PHY_STANDARD_80211g, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "no hidden terminal, sta node of 12, under 802.11g, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 12, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 12, WIFI_PHY_STANDARD_80211g, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        result << "---------------------------------------------------\n";
	result << "---------------------------------------------------\n";

	std::cout << "have hidden terminal, sta node of 3, under 802.11a, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 3, under 802.11a, cbrate of 20Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211a, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "have hidden terminal, sta node of 6, under 802.11a, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 6, under 802.11a, cbrate of 20Mbps: \n";
        a = experiment (false, 6, WIFI_PHY_STANDARD_80211a, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "have hidden terminal, sta node of 9, under 802.11a, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 9, under 802.11a, cbrate of 20Mbps: \n";
        a = experiment (false, 9, WIFI_PHY_STANDARD_80211a, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "have hidden terminal, sta node of 12, under 802.11a, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 12, under 802.11a, cbrate of 20Mbps: \n";
        a = experiment (false, 12, WIFI_PHY_STANDARD_80211a, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	result << "---------------------------------------------------\n";

	std::cout << "have hidden terminal, sta node of 3, under 802.11b, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 3, under 802.11b, cbrate of 20Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211b, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "have hidden terminal, sta node of 6, under 802.11b, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 6, under 802.11b, cbrate of 20Mbps: \n";
        a = experiment (false, 6, WIFI_PHY_STANDARD_80211b, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "have hidden terminal, sta node of 9, under 802.11b, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 9, under 802.11b, cbrate of 20Mbps: \n";
        a = experiment (false, 9, WIFI_PHY_STANDARD_80211b, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "have hidden terminal, sta node of 12, under 802.11b, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 12, under 802.11b, cbrate of 20Mbps: \n";
        a = experiment (false, 12, WIFI_PHY_STANDARD_80211b, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        result << "---------------------------------------------------\n";

	std::cout << "have hidden terminal, sta node of 3, under 802.11g, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 3, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211g, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "have hidden terminal, sta node of 6, under 802.11g, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 6, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 6, WIFI_PHY_STANDARD_80211g, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "have hidden terminal, sta node of 9, under 802.11g, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 9, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 9, WIFI_PHY_STANDARD_80211g, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        std::cout << "have hidden terminal, sta node of 12, under 802.11g, cbrate of 20Mbps: \n" << std::flush;
        result << "have hidden terminal, sta node of 12, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 12, WIFI_PHY_STANDARD_80211g, true, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

        result << "---------------------------------------------------\n";
	result << "---------------------------------------------------\n";
  */
	std::cout << "with hidden terminal, sta node of 3, under 802.11n cbrate of 10Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 3, under 802.11g, cbrate of 10Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211n_2_4GHZ, false, "10Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "with hidden terminal, sta node of 3, under 802.11n, cbrate of 20Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 3, under 802.11g, cbrate of 20Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211n_2_4GHZ, false, "20Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";


	std::cout << "with hidden terminal, sta node of 3, under 802.11n, cbrate of 30Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 3, under 802.11g, cbrate of 30Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211n_2_4GHZ, false, "30Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "with hidden terminal, sta node of 3, under 802.11n, cbrate of 40Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 3, under 802.11g, cbrate of 40Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211n_2_4GHZ, false, "40Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";

	std::cout << "with hidden terminal, sta node of 3, under 802.11n, cbrate of 50Mbps: \n" << std::flush;
        result << "no hidden terminal, sta node of 3, under 802.11g, cbrate of 50Mbps: \n";
        a = experiment (false, 3, WIFI_PHY_STANDARD_80211n_2_4GHZ, false, "50Mbps");
        result << "throughput: " << a << "\n";
        std::cout << "------------------------------------------------\n";
	result << "---------------------------------------------------\n";
	result << "---------------------------------------------------\n";
	result << "---------------------------------------------------\n";

	// std::cout << "no hidden terminal, sta node of 3, under 802.11n:\n" << std::flush; no definition on
        // experiment (false, 3, WIFI_PHY_STANDARD_80211n);
        // std::cout << "------------------------------------------------\n";
        // std::cout << "no hidden terminal, sta node of 6, under 802.11n:\n" << std::flush;
        // experiment (false, 6, WIFI_PHY_STANDARD_80211n);
        // std::cout << "------------------------------------------------\n";
        // std::cout << "no hidden terminal, sta node of 9, under 802.11n:\n" << std::flush;
        // experiment (false, 9, WIFI_PHY_STANDARD_80211n);
        // std::cout << "------------------------------------------------\n";
        // std::cout << "no hidden terminal, sta node of 12, under 802.11n:\n" << std::flush;
        // experiment (false, 12, WIFI_PHY_STANDARD_80211n);
        // std::cout << "------------------------------------------------\n";
        // std::cout << "------------------------------------------------\n";
	result.close();
	return 0;
}