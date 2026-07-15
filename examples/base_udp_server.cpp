/**
 * @file base_udp_server.cpp
 * @brief 
 * @details
 * @author AstraCod3
 * @date June 28, 2026
 * @version 1.0.0
 * 
 * @note
 * @warning
 * @see 
 * @todo
 * @deprecated
 * @def
 */

#include <chrono>
#include <iostream>
#include <thread>
#include <string_view>
#include <algorithm> // std::for_each
#include <iomanip>
#include "../udp_server/udp_server.hpp"

using namespace std::chrono_literals;

namespace ns_base_udp_server {

    /**
     * @brief Orchestrator entry-point modeling execution flow, background tasks, and safe lifecycle termination.
     * @return int Status execution code (0 for success).
     */
    void base_main_udp_server() ;
    void base_main_udp_server() {
        unsigned short lport = 1581;
        std::cout << "local port : " << lport << "\n";
        ns_udp_server::udp_server udpsrv(lport);

        std::thread server_thread([&udpsrv]() {
            std::cout << "[Thread] Server UDP start\n";
            udpsrv.start(); 
        });
        server_thread.detach();

        int wait_seconds = 5;
        std::cout << "waiting for packet for " << wait_seconds << " secs otherwise this process will be terminated\n";
        for(int ws = wait_seconds; ws > -1; ws--) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            std::cout << ws << std::flush;
            if ( ws > 0 )
                std::cout << " .. ";
        }
        std::cout << "\n";
    
        std::vector<uint8_t> buff_rx;

        std::cout << "get last packet ... ... ";
        udpsrv.get_last_packet_sync( buff_rx );
        std::cout << "recevied\n";
        std::cout << "size last packet : " << buff_rx.size() << "\n";

        auto print_hex = [](uint8_t byte) {
            std::cout << "HEX:\n";
            std::cout << std::hex          // Imposta la base esadecimale
                      << std::setw(2)      // Forza la larghezza a 2 caratteri
                      << std::setfill('0') // Aggiunge lo '0' iniziale se necessario
                      << static_cast<int>(byte) // Cast a int per stampare il numero e non il carattere ASCII
                      << " ";              // Spazio di separazione
        };

        std::for_each(buff_rx.begin(), buff_rx.end(), print_hex);

        std::cout << "udp server ... ... ";
        udpsrv.stop();
        std::cout << "stop\n";
            
        std::cout << "waiting udp server thread joinable\n";
        if (server_thread.joinable()) {
            std::cout << "udp server thread ... ... ";
            server_thread.join();
            std::cout << "joined\n";
        }
    }
}

/**
 * @brief
 * @return int 
 */
int main() {
    std::cout << "\n";
    std::cout << " =============================================================\n";
    std::cout << "                  Running Base Udp Server Example...\n";
    std::cout << " =============================================================\n";
    std::cout << "\n";
    int ret = EXIT_SUCCESS;
    ns_base_udp_server::base_main_udp_server();
    std::cout << "\n";
    std::cout << "..done!\n";
    return ret;
}
