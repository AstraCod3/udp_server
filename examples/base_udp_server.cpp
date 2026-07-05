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
#include <condition_variable>
#include <string_view>

#include "../udp_server/udp_server.hpp"

//using std::chrono_literals;

namespace ns_base_udp_server {

    /**
     * @brief Orchestrator entry-point modeling execution flow, background tasks, and safe lifecycle termination.
     * @return int Status execution code (0 for success).
     */
    void base_main_udp_server() ;
    void base_main_udp_server() {
        std::cout << "\n";
        std::cout << "Running base_main_udp_server start ...\n";
        std::cout << "\n";
        
        unsigned short lport = 1581;
        std::cout << " local port : " << lport << "\n";
        ns_udp_server::udp_server udpsrv(lport);

        std::thread server_thread([&]() {
            std::cout << "[Thread] Server UDP start\n";
            udpsrv.start(); 
            std::cout << "\n";
        });

    
        int pack_counter = 0;
        const int pack_received = 10;
        std::vector<uint8_t> buff_rx;

        buff_rx.reserve( ns_udp_server::max_size_udp_rx );

        //auto print_hex = [](unsigned char byte) {
        auto print_hex = [](uint8_t byte) {
            std::cout << std::hex          // Imposta la base esadecimale
                      << std::setw(2)      // Forza la larghezza a 2 caratteri
                      << std::setfill('0') // Aggiunge lo '0' iniziale se necessario
                      << static_cast<int>(byte) // Cast a int per stampare il numero e non il carattere ASCII
                      << " ";              // Spazio di separazione
        };

        while ( pack_counter < pack_received ) {
            udpsrv.get_last_packet( buff_rx );
            pack_counter++;

//          std::string_view ascii_view(reinterpret_cast<const char*>(buff_rx.data()), buff_rx.size());
//          std::cout << " received packet : " << pack_counter << std::endl;

            std::cout << "size last packet : " << buff_rx.size() << "\n";
            std::cout << "HEX:\n";
            std::for_each(buff_rx.begin(), buff_rx.end(), print_hex);


            buff_rx.clear(); 
            // std::cout << " received packet : " << pack_counter << std::endl;
        }

        /*
           std::cout << " waiting 500ms\n";
           std::this_thread::sleep_for(500ms);
         */

        udpsrv.stop();
        
        if (server_thread.joinable()) {
            server_thread.join();
        }

        std::cout << "\n";
        std::cout << "...done!!\n";
        std::cout << "\n";

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
    std::cout << "...done!\n";
    return ret;
}
