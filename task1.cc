// Includes
#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ssid.h"
#include "ns3/yans-wifi-helper.h"

using namespace ns3;

NS_LOG_COMPONENT_DEFINE("HW2_Task1_Team_46");

int
main(int argc, char* argv[])
{
    // Gestione parametri
    bool useRtsCts = false;
    bool verbose = false;
    bool useNetAnim = false;

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts", "Se vero viene forzato l'utilizzo dell'handshake RTS/CTS da parte della rete", useRtsCts);
    cmd.AddValue("verbose", "Se vero viene abilitato l'uso dei logs per il server e per i clients della UDP Echo Application", verbose);
    cmd.AddValue("useNetAnim", "Se vero vengono generati tutti i file relativi per NetAnim", useNetAnim);
    cmd.Parse(argc, argv);

    // Imposta LOG
    if(verbose) {
        LogComponentEnable("UdpEchoClientApplication", LOG_LEVEL_INFO);
        LogComponentEnable("UdpEchoServerApplication", LOG_LEVEL_INFO);
    }

    // Imposta RTS/CTS
    if(useRtsCts){
        UintegerValue rtsCtsThreshold = UintegerValue(100);
        Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", rtsCtsThreshold);
    }

    // Creazione rete
    NS_LOG_INFO("Create nodes.");

    uint32_t nNodes = 5;
    NodeContainer nodes;
    nodes.Create(nNodes);


    NS_LOG_INFO("Create and install channels.");

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;

    WifiHelper wifi;
    wifi.SetStandard(WifiStandard(WIFI_STANDARD_80211g));
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");

    NetDeviceContainer device;
    mac.SetType("ns3::AdhocWifiMac",
                "QosSupported", BooleanValue(false));
    device = wifi.Install(phy, mac, nodes);

    NS_LOG_INFO("Setup mobility.");

    MobilityHelper mobility;

    mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                  "MinX", DoubleValue(0.0),
                                  "MinY", DoubleValue(0.0),
                                  "DeltaX", DoubleValue(5.0),
                                  "DeltaY", DoubleValue(10.0),
                                  "GridWidth", UintegerValue(3),
                                  "LayoutType", StringValue("RowFirst"));

    mobility.SetMobilityModel("ns3::RandomWalk2dMobilityModel",
                              "Bounds", RectangleValue(Rectangle(-90, 90, -90, 90)));

    mobility.Install(nodes);


    NS_LOG_INFO("Internet stack.");

    InternetStackHelper internet;
    internet.Install(nodes);


    NS_LOG_INFO("Assign IPv4 Addresses.");

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "/24");
    Ipv4InterfaceContainer interface = address.Assign(device);


    NS_LOG_INFO("Create applications.");
    // n0 (UDP ECHO Server)
    uint16_t portEcho = 20;
    UdpEchoServerHelper echoServer(portEcho);
    ApplicationContainer echoServerApp = echoServer.Install(nodes.Get(0));

    echoServerApp.Start(Seconds(0.0));
    echoServerApp.Stop(Seconds(7.0)); // finisce 3 secondi dopo l'invio dell'ultimo pacchetto

    // UDP ECHO Client
    Address echoServerAddress = interface.GetAddress(0);
    uint32_t packetSize = 512;
    uint32_t maxPacketCount = 2;
    Time interPacketInterval4 = Seconds(1);
    Time interPacketInterval3 = Seconds(2);

    // n4
    UdpEchoClientHelper n4Client(echoServerAddress, portEcho);
    n4Client.SetAttribute("PacketSize", UintegerValue(packetSize));
    n4Client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    n4Client.SetAttribute("Interval", TimeValue(interPacketInterval4));
    ApplicationContainer n4App = n4Client.Install(nodes.Get(4));

    n4App.Start(Seconds(1.0));
    n4App.Stop(Seconds(3.0)); // finisce un secondo dopo l'invio dell'ultimo pacchetto

    // n3
    UdpEchoClientHelper n3Client(echoServerAddress, portEcho);
    n3Client.SetAttribute("PacketSize", UintegerValue(packetSize));
    n3Client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    n3Client.SetAttribute("Interval", TimeValue(interPacketInterval3));
    ApplicationContainer n3App = n3Client.Install(nodes.Get(3));

    n3App.Start(Seconds(2.0));
    n3App.Stop(Seconds(5.0)); // finisce un secondo dopo l'invio dell'ultimo pacchetto


    NS_LOG_INFO("Enable static global routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();


    NS_LOG_INFO("Setup NetAnim.");

    AnimationInterface* anim;
    if(useNetAnim){
        anim = new AnimationInterface(useRtsCts ? "wireless-task1-rts-on.xml" : "wireless-task1-rts-off.xml");

        anim->UpdateNodeDescription(nodes.Get(0), "SRV-0");
        anim->UpdateNodeColor(nodes.Get(0), 255, 0, 0);

        anim->UpdateNodeDescription(nodes.Get(1), "HOC-1");
        anim->UpdateNodeColor(nodes.Get(1), 0, 0, 255);

        anim->UpdateNodeDescription(nodes.Get(2), "HOC-2");
        anim->UpdateNodeColor(nodes.Get(2), 0, 0, 255);

        anim->UpdateNodeDescription(nodes.Get(3), "CLI-3");
        anim->UpdateNodeColor(nodes.Get(3), 0, 255, 0);

        anim->UpdateNodeDescription(nodes.Get(4), "CLI-4");
        anim->UpdateNodeColor(nodes.Get(4), 0, 255, 0);

        anim->EnablePacketMetadata();
        anim->EnableWifiMacCounters(Seconds(0), Seconds(7));
        anim->EnableWifiPhyCounters(Seconds(0), Seconds(7));
    }


    NS_LOG_INFO("Enable tracing.");

    phy.EnablePcap(useRtsCts ? "task1-on-2.pcap" : "task1-off-2.pcap", device.Get(2), true, true);


    NS_LOG_INFO("Run Simulation.");

    Simulator::Stop(Seconds(7));
    Simulator::Run();
    Simulator::Destroy();


    NS_LOG_INFO("Done.");
    return 0;
}