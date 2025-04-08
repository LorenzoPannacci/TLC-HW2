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

NS_LOG_COMPONENT_DEFINE("HW2_Task2_Team_46");

int
main(int argc, char* argv[])
{
    // Gestione parametri
    bool useRtsCts = false;
    bool verbose = false;
    bool useNetAnim = false;
    std::string ssidName = "TLC2022";

    CommandLine cmd(__FILE__);
    cmd.AddValue("useRtsCts", "Se vero viene forzato l'utilizzo dell'handshake RTS/CTS da parte della rete", useRtsCts);
    cmd.AddValue("verbose", "Se vero viene abilitato l'uso dei logs per il server e per i clients della UDP Echo Application", verbose);
    cmd.AddValue("useNetAnim", "Se vero vengono generati tutti i file relativi per NetAnim", useNetAnim);
    cmd.AddValue("ssid", "Nome della SSID della rete.", ssidName);
    cmd.Parse(argc, argv);

    // Abilità LOG
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

    uint32_t nStaNodes = 5;
    NodeContainer staNodes;
    staNodes.Create(nStaNodes);

    uint32_t nApNodes = 1;
    NodeContainer apNodes;
    apNodes.Create(nApNodes);


    NS_LOG_INFO("Create and install channels.");

    YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
    YansWifiPhyHelper phy;
    phy.SetChannel(channel.Create());

    WifiMacHelper mac;
    Ssid ssid = Ssid(ssidName);

    WifiHelper wifi;
    wifi.SetStandard(WifiStandard(WIFI_STANDARD_80211g));
    wifi.SetRemoteStationManager("ns3::AarfWifiManager");
    
    // setup STA
    mac.SetType("ns3::StaWifiMac",
                "Ssid", SsidValue(ssid),
                "QosSupported", BooleanValue(false));
    NetDeviceContainer staDevice = wifi.Install(phy, mac, staNodes);

    // setup AP
    mac.SetType("ns3::ApWifiMac",
                "Ssid", SsidValue(ssid),
                "QosSupported", BooleanValue(false));
    NetDeviceContainer apDevice = wifi.Install(phy, mac, apNodes);


    NS_LOG_INFO("Setup mobility.");

    // Mobility
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
    mobility.Install(staNodes);
    
    mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
    mobility.Install(apNodes);


    NS_LOG_INFO("Internet stack.");

    InternetStackHelper stack;
    stack.Install(staNodes);
    stack.Install(apNodes);


    NS_LOG_INFO("Assign IPv4 Addresses.");

    Ipv4AddressHelper address;
    address.SetBase("192.168.1.0", "/24");
    Ipv4InterfaceContainer staInterface = address.Assign(staDevice);
    Ipv4InterfaceContainer apInterface = address.Assign(apDevice);


    NS_LOG_INFO("Create applications.");
    // n0 (UDP ECHO Server)
    uint16_t portEcho = 21;
    UdpEchoServerHelper echoServer(portEcho);
    ApplicationContainer echoServerApp = echoServer.Install(staNodes.Get(0));

    echoServerApp.Start(Seconds(0.0));
    echoServerApp.Stop(Seconds(7.0)); // finisce 3 secondi dopo l'invio dell'ultimo pacchetto

    // UDP ECHO Client
    uint32_t packetSize = 512;
    uint32_t maxPacketCount = 2;
    Time interPacketInterval3 = Seconds(2);
    Time interPacketInterval4 = Seconds(3);

    // n3
    UdpEchoClientHelper n3Client(staInterface.GetAddress(0), portEcho);
    n3Client.SetAttribute("PacketSize", UintegerValue(packetSize));
    n3Client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    n3Client.SetAttribute("Interval", TimeValue(interPacketInterval3));
    ApplicationContainer n3App = n3Client.Install(staNodes.Get(3));

    n3App.Start(Seconds(2.0));
    n3App.Stop(Seconds(5.0)); // finisce un secondo dopo l'invio dell'ultimo pacchetto

    // n4
    UdpEchoClientHelper n4Client(staInterface.GetAddress(0), portEcho);
    n4Client.SetAttribute("PacketSize", UintegerValue(packetSize));
    n4Client.SetAttribute("MaxPackets", UintegerValue(maxPacketCount));
    n4Client.SetAttribute("Interval", TimeValue(interPacketInterval4));
    ApplicationContainer n4App = n4Client.Install(staNodes.Get(4));

    n4App.Start(Seconds(1.0));
    n4App.Stop(Seconds(5.0)); // finisce un secondo dopo l'invio dell'ultimo pacchetto


    NS_LOG_INFO("Enable static global routing.");
    Ipv4GlobalRoutingHelper::PopulateRoutingTables(); 


    NS_LOG_INFO("Setup NetAnim.");

    AnimationInterface* anim;
    if(useNetAnim){
        anim = new AnimationInterface(useRtsCts ? "wireless-task2-rts-on.xml" : "wireless-task2-rts-off.xml"); 

        anim->UpdateNodeDescription(staNodes.Get(0), "SRV-0");
        anim->UpdateNodeColor(staNodes.Get(0), 255, 0, 0);

        anim->UpdateNodeDescription(staNodes.Get(1), "STA-1");
        anim->UpdateNodeColor(staNodes.Get(1), 0, 0, 255);

        anim->UpdateNodeDescription(staNodes.Get(2), "STA-2");
        anim->UpdateNodeColor(staNodes.Get(2), 0, 0, 255);

        anim->UpdateNodeDescription(staNodes.Get(3), "CLI-3");
        anim->UpdateNodeColor(staNodes.Get(3), 0, 255, 0);

        anim->UpdateNodeDescription(staNodes.Get(4), "CLI-4");
        anim->UpdateNodeColor(staNodes.Get(4), 0, 255, 0);

        anim->UpdateNodeDescription(apNodes.Get(0), "AP");
        anim->UpdateNodeColor(apNodes.Get(0), 66, 49, 137);

        anim->EnablePacketMetadata();
        anim->EnableWifiMacCounters(Seconds(0), Seconds(7));
        anim->EnableWifiPhyCounters(Seconds(0), Seconds(7));
    }


    NS_LOG_INFO("Enable tracing.");

    phy.EnablePcap(useRtsCts ? "task2-on-4.pcap" : "task2-off-4.pcap", staDevice.Get(4), true, true);
    phy.EnablePcap(useRtsCts ? "task2-on-5.pcap" : "task2-off-5.pcap", apDevice.Get(0), true, true);


    NS_LOG_INFO("Run Simulation.");

    Simulator::Stop(Seconds(7));
    Simulator::Run();
    Simulator::Destroy();


    NS_LOG_INFO("Done.");
    return 0;
}