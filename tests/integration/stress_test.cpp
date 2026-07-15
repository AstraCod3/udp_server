/**
 * @file receive_data_sync.cpp
 * @brief
 * @details
 * @author AstraCod3
 * @date July 01, 2026
 * @version 0.0.1
 * 
 * @note
 * @warning
 * @see
 * @todo
 * @deprecated
 * @def
 */

#include <array>
#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>


#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif
#if defined __linux__ || __unix__
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define closesocket close
#endif

#include "../../udp_server/udp_server.hpp"

/**
 * @namespace ns_stress_test
 * @brief
 */
namespace ns_stress_test {

    /**
     * @brief
     */
    const int LOCAL_PORT = 1581;

    /**
     * @brief
     */
    ns_udp_server::udp_server server(LOCAL_PORT);

    /**
     * @brief
     */
    // const int NUM_PACKETS = 1426;
    const int NUM_PACKETS = 14;

    /**
     * @brief
     */
    const int NUM_BYTES_PER_PACKET = 620;

    /**
     * @brief
     */
#ifdef _WIN32
    int num_bytes_tx = 0;
    SOCKET client_fd = INVALID_SOCKET;
#endif

#if defined __linux__ || __unix__
    ssize_t num_bytes_tx = 0;
    int client_fd = -1;
#endif

    /**
     * @brief
     */
    std::array< std::array<uint8_t, NUM_BYTES_PER_PACKET>, NUM_PACKETS > buffer_to_send;

    /**
     * @brief
     */
    std::vector< uint8_t > packet_rx;

    /**
     * @brief
     */
    std::string str_last_packet;

    /**
     * @brief
     */
    std::vector<uint8_t> check_msg;

    /**
     * @brief
     */
    std::mutex mtx_print;

    /**
     * @brief
     */
    int counter_missmatch = 0;

    /**
     * @brief
     */
    void print_msg( const std::string& _msg ) ;
    void print_msg( const std::string& _msg ) {
        std::unique_lock<std::mutex> lck( mtx_print );
        std::cout << _msg << "\n";
    }

    /**
     * @brief Prepare buffer to send
     */
    void prepare_packet_to_send() ;
    void prepare_packet_to_send() {
        unsigned int counter_packets = 0;
        unsigned int counter_bytes = 0;
        print_msg( std::string ("[TEST] Prepare packets ..."));
        for ( counter_packets = 0; counter_packets < NUM_PACKETS; counter_packets++) {
            std::array<uint8_t, NUM_BYTES_PER_PACKET> tmp_packet;
            for ( counter_bytes = 0; counter_bytes < NUM_BYTES_PER_PACKET; counter_bytes++) {
                tmp_packet[ counter_bytes ] = static_cast<uint8_t>( ((counter_bytes + counter_packets) & 255) );
            }
            buffer_to_send[ counter_packets ] = tmp_packet;
        }
        print_msg( std::string ("[TEST] Prepare packets end"));
    }

    /**
     * @brief Close Client Socket
     */
    void close_socket_client () ;
    void close_socket_client () {
    #ifdef _WIN32
        ::closesocket(client_fd);
        WSACleanup();
        client_fd = INVALID_SOCKET;
    #endif

    #if defined __linux__ || __unix__
        ::close(client_fd);
        client_fd = -1;
    #endif

    }

    /**
     * @brief Open Socket Client
     * @return EXIT_SUCCESS on success, or a non-zero value on failure.
     */
    int open_socket_client() ;
    int open_socket_client() {
    #ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        client_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if (client_fd == INVALID_SOCKET) {
            std::cerr << "[ERROR] Invalid client socket!!!\n";
            return EXIT_FAILURE;
        }
    #endif

    #if defined __linux__ || __unix__
        client_fd = socket(AF_INET, SOCK_DGRAM, 0);
        if ( client_fd < 0 ) {
            std::cerr << "[ERROR] Invalid client socket!!!\n";
            return EXIT_FAILURE;
        }
    #endif
        return EXIT_SUCCESS;
    }

    /**
     * @brief Send Packets
     */
    void send_packets() ;
    void send_packets() {
        char* buff_tx = nullptr;
        if ( open_socket_client() != EXIT_SUCCESS ) {
            return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(LOCAL_PORT);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);


        for ( int cnt_tx = 0; cnt_tx < NUM_PACKETS; cnt_tx++ ) {
            buff_tx = reinterpret_cast<char*>( buffer_to_send[ cnt_tx ].data() );

            num_bytes_tx = sendto(client_fd,
                                    buff_tx, // Linux accepts const void*, so unsigned char* converts implicitly
                                    NUM_BYTES_PER_PACKET,
                                    0,
                                    (struct sockaddr*)&server_addr,
                                    sizeof(server_addr));

            if ( num_bytes_tx < 0 ) {
                close_socket_client();
                return;
            }
            /*print_msg( std::string( "[TEST] --->>> Sent packet [" + 
                std::to_string( cnt_tx ) +
                "] data [0]: " + std::to_string(buff_tx[0])));*/
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        close_socket_client();
    }

    /**
     * @brief Check packets received
     */
    void recv_and_check_packets( ) ;
    void recv_and_check_packets() {
        int cnt_rx = 0;
        while (cnt_rx < NUM_PACKETS) {
            /* print_msg(std::string ("[TEST] <<<--- Waiting receive packets ...")); */
            server.get_last_packet_sync(packet_rx);

            /* print_msg(std::string( "[TEST] <<<--- Received packet : " + std::to_string( cnt_rx ))); */
            check_msg.assign(buffer_to_send[cnt_rx].begin(), buffer_to_send[cnt_rx].end());
            // assert(packet_rx.size() == check_msg.size() && "ERROR: packets size mismatch!");
            auto it_check = check_msg.begin();
            auto it_rx = packet_rx.begin();
            for ( size_t cnt = 0; cnt < packet_rx.size(); cnt++ ) {
                if ( *it_rx != *it_check ) {
                    print_msg( std::string("[TEST] ERROR: MISSMATCHED\n Counter Packet : " + std::to_string(cnt_rx) +
                            "\n N.Element : " + std::to_string( cnt ) + 
                            "\n received " + std::to_string(*it_rx) + 
                            "\n expected " + std::to_string(*it_check) ));
                    counter_missmatch++;
                    break;
                }
                ++it_check;
                ++it_rx;
            }

            /*if (checked) {
                print_msg(std::string("[TEST] <<<--- Verification of get_last_packet(): PASSED"));
            }*/

            cnt_rx++;
            check_msg.clear();
        }
    }

    /**
     * @brief main of stress test
     */
    int main_stress_test() ;
    int main_stress_test() {
        int ret = EXIT_SUCCESS;
        std::cout << "[TEST] Starting integration test for udp_server..." << std::endl;
        try {
            std::thread server_thread([&]() {
                print_msg( std::string( "[TEST] Udp Server thread BEGIN") );
                server.start(); 
                print_msg( std::string( "[TEST] Udp Server thread END") );
            });

            prepare_packet_to_send();

            while (server.get_status_enum() != ns_udp_server::SERVER_STATUS::RUNNING) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
            std::this_thread::sleep_for(std::chrono::seconds(1));

            std::thread received_thread([&]() {
                print_msg( std::string( "[TEST] Receive thread BEGIN") );
                recv_and_check_packets();
                print_msg( std::string( "[TEST] Recevie thread END") );
            });

            std::this_thread::sleep_for(std::chrono::seconds(3));
            std::thread send_thread([&]() {
                print_msg( std::string( "[TEST] Send thread BEGIN") );
                
                send_packets();
                print_msg( std::string( "[TEST] Send thread END") );
            });
            std::this_thread::sleep_for(std::chrono::seconds(7));

            if (send_thread.joinable()) {
                print_msg( std::string( "[TEST] Send thread JOINABLE") );
                send_thread.join();
                print_msg( std::string( "[TEST] Send thread JOIN") );
            }

            // 5. Validate the graceful teardown and stopping mechanism
            print_msg(std::string("[TEST] Requesting server stop..."));
            server.stop();

            // Synchronize and wait for the server execution thread to terminate cleanly
            if (received_thread.joinable()) {
                print_msg( std::string( "[TEST] Receive thread JOINABLE") );
                received_thread.join();
                print_msg( std::string( "[TEST] Recevie thread JOIN") );
            }
            std::this_thread::sleep_for(std::chrono::seconds(3));

            // Synchronize and wait for the server execution thread to terminate cleanly
            if (server_thread.joinable()) {
                print_msg( std::string( "[TEST] Server thread JOINABLE") );
                server_thread.join();
                print_msg( std::string( "[TEST] Server thread JOIN") );
            }
            if ( counter_missmatch == 0 ) {
                print_msg( std::string("[TEST] INTEGRATION TEST COMPLETED SUCCESSFULLY!" ));
            }
            else {
                print_msg( std::string("[TEST] INTEGRATION TEST Counter missmatched : " + std::to_string( counter_missmatch)));
                ret = EXIT_FAILURE;
            }
        } catch (const ns_udp_server::udp_server_error& e) {
            std::cerr << "[TEST FAILED] Server component exception caught: " << e.what() << std::endl;
            ret = EXIT_FAILURE;
        } catch (const std::exception& e) {
            std::cerr << "[TEST FAILED] Standard generic exception caught: " << e.what() << std::endl;
            ret = EXIT_FAILURE;
        }
        return ret;
    }

} // ns_stress_test

/**
 * @brief
 */
int main() {
    std::cout << "\n";
    int ret = EXIT_SUCCESS;
    ret = ns_stress_test::main_stress_test();
    if ( ret == EXIT_FAILURE )
        return ret;
    return ret;
}

