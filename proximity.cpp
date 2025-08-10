#include <iostream>
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <pcap.h>
#include <unistd.h>

struct T {
    std::string mac;
    std::string name;
};

// REQUIREMENTS:
// apt install libpcap-dev
// apt install build-essential
// apt install iw bluetooth bluez
// BLE: apt install bluez-tools gatttool
// MONITOR MODE:
// ip link set wlan0 down
// iw wlan0 set monitor control
// ip link set wlan0 up
// ip link set wlan0mon up

// COMPILE:
// g++ NearProximityResearchComponent.cpp -o np_research -lpcap

std::vector<T> scan() {
    std::vector<T> v;
    system("iwlist wlan0 scan | grep 'Address' > wifi.log");
    std::ifstream w("wifi.log");
    std::string l;
    while (getline(w, l)) {
        std::size_t p = l.find("Address:");
        if (p != std::string::npos) {
            T t;
            t.mac = l.substr(p + 9);
            t.name = "WiFi_Device";
            v.push_back(t);
        }
    }
    w.close();
    system("hcitoo1 scan > bt.log"); // Assuming typo, should be hcitool
    std::ifstream b("bt.log");
    while (getline(b, l)) {
        std::size_t p = l.find("Address:");
        if (p != std::string::npos) {
            T t;
            t.mac = l.substr(p + 9);
            t.name = "BT_Device";
            v.push_back(t);
        }
    }
    b.close();
    return v;
}

bool mac_found(const std::vector<T>& devs, const std::string& tgt) {
    for (const auto& d : devs)
        if (d.mac == tgt)
            return true;
    return false;
}

std::string encode_payload(const std::string& cmd) {
    std::string out = "";
    for (char c : cmd)
        if (c >= 32 && c <= 126)
            out += c;
        else
            out += '.';
    return out;
}

void beacon(const std::string& i, const std::string& s) {
    char e[PCAP_ERRBUF_SIZE];
    pcap_t* h = pcap_open_live(i.c_str(), BUFSIZ, 1, 1000, e);
    if (!h) {
        std::cerr << "pcap fail: " << e << "\n";
        return;
    }
    u_char fl[] = {
        0x80, 0x00, 0x00, 0x00, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x0c, 0x29, 0xaa, 0xbb, 0xcc, 0x00, 0x0c,
        0x29, 0xaa, 0xbb, 0xcc, 0x00
    };
    std::vector<u_char> pkt(fl, fl + sizeof(fl));
    pkt.insert(pkt.end(), s.begin(), s.end());
    int r = pcap_sendpacket(h, &pkt[0], pkt.size());
    if (r == 0)
        std::cerr << "[+] Beacon FAIL\n";
    else
        std::cout << "[+1] Beacon SENT: " << s << "\n";
    pcap_close(h);
}

void sniff_probes(const std::string& i) {
    char e[PCAP_ERRBUF_SIZE];
    pcap_t* h = pcap_open_live(i.c_str(), BUFSIZ, 1, 1000, e);
    if (!h) {
        std::cerr << "pcap fail: " << e << "\n";
        return;
    }
    while (true) {
        struct pcap_pkthdr* hdr;
        const u_char* pkt;
        int r = pcap_next_ex(h, &hdr, &pkt);
        if (r == 1 && pkt[0] == 0x40) {
            char mac[18];
            snprintf(mac, sizeof(mac), "%02X:%02X:%02X:%02X:%02X:%02X",
                     pkt[10], pkt[11], pkt[12], pkt[13], pkt[14], pkt[15]);
            std::ofstream log("probes.txt", std::ios::app);
            log << "[!] Probe from: " << mac << "\n";
            log.close();
        }
    }
    pcap_close(h);
}

void ble_query(const std::string& m) {
    std::string c = "gatttool -b " + m + " --char-read -a 0x0025";
    system(c.c_str());
}

int main() {
    std::string iface = "wlan0mon";
    std::string sig = "ZETA_SIG";
    std::cout << "[ NEAR PROXIMITY RESEARCH COMPONENT ]\n"
              << "1. Scan + Log Nearby\n"
              << "2. Inject Encoded Beacon\n"
              << "3. Loop Scan + Inject on Target MAC\n"
              << "4. Passive Probe Logger\n"
              << "5. BLE GATT Manual Query\n"
              << "6. Exit\n"
              << ">> ) ";
    int c;
    std::cin >> c;

    if (c == 1) {
        auto d = scan();
        std::ofstream f("log.txt");
        for (const auto& x : d) {
            f << x.mac << " " << x.name << "\n";
        }
        f.close();
        std::cout << "[+1] Logged.\n";
    }
    else if (c == 2) {
        std::string msg;
        std::cout << "Payload: ";
        std::cin.ignore();
        std::getline(std::cin, msg);
        std::string p = encode_payload(msg);
        beacon(iface, p);
    }
    else if (c == 3) {
        std::string m;
        std::cout << "Target MAC: ";
        std::cin.ignore();
        std::getline(std::cin, m);
        auto d = scan();
        while (mac_found(d, m)) {
            std::string p = encode_payload(sig);
            beacon(iface, p);
            d = scan();
        }
    }
    else if (c == 4) {
        sniff_probes(iface);
    }
    else if (c == 5) {
        std::string m;
        std::cout << "MAC Address: ";
        std::cin.ignore();
        std::getline(std::cin, m);
        ble_query(m);
    }
    else if (c == 6) {
        return 0;
    }

    return 0;
}