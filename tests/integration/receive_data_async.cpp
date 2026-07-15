/**
 * @file receive_data_async.cpp
 * @brief
 * @details
 * @author AstraCod3
 * @date July 01, 2026
 * @version 0.0.1
 * 
 * @note Ensure that this source file header is included in the implementation file.
 * @warning
 * @see
 * @todo
 * @deprecated
 * @def
 */

#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <cstring>

// Cross-platform minimal socket configuration for the test client
#if defined _WIN32 || defined _WIN64
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#endif

#if defined __linux__ || defined __unix__
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define closesocket close
#endif

#include "../../udp_server/udp_server.hpp"

namespace ns_receive_data_async {

    /**
     * @brief Local Port
     */
    const int LOCAL_PORT = 1581;

    /**
     * @brief Number of packets to send
     */
    const int NUM_PACKETS = 5;

    /**
     * @brief Helper function to send a raw UDP packet to the local server.
    * @param[in] message The payload string to be transmitted over UDP.
    */
    void send_packet(const std::string& message) ;
    void send_packet(const std::string& message) {
        bool err_sock = false;
    #if defined _WIN32 || defined _WIN64
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        SOCKET client_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if ( client_fd == INVALID_SOCKET ) {
            err_sock = true;
        }
        int num_bytes_tx = 0;
    #endif

    #if defined __linux__ || defined __unix__
        int client_fd = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (client_fd < 0 ) {
            err_sock = true;
        }
        ssize_t num_bytes_tx = 0;
    #endif
        
        if ( err_sock ) {
            std::cerr << "[TEST] ERROR : send_test_packet error socket\n";
            return;
        }

        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(LOCAL_PORT);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

        ///< Transmit the raw data packet
        num_bytes_tx = ::sendto(client_fd,
                                    message.c_str(),
                                    static_cast<int>(message.size()),
                                    0, 
                                    (sockaddr*)&server_addr,
                                    sizeof(server_addr) );

        if ( num_bytes_tx < 0 ) {
            std::cerr << "[TEST] ERROR : sendto failed\n";
        }

    #if defined _WIN64 || defined _WIN32
        ::closesocket(client_fd);
        WSACleanup();
    #elif defined __linux__ || defined __unix__
        ::close(client_fd);
    #endif

    }

    /**
     * @brief Main of receive data async
     * @return EXIT_SUCCESS on success, or a non-zero value on failure.
     */
    int main_receive_data_async() ;
    int main_receive_data_async() {
        std::cout << "[TEST] Starting integration test for udp_server async ..." << std::endl;
        try {
            // 1. Instantiate the server within the designated custom namespace
            ns_udp_server::udp_server server(LOCAL_PORT);
            // 2. Launch the server loop inside a dedicated background thread to prevent blocking
            std::thread server_thread([&server]() {
                server.start(); 
                std::cout << "[TEST] Udp server started\n";
            });
            // Small delay to ensure the server socket completes binding operations
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            std::vector< uint8_t > last_packet;
            std::string sent_payload;
            std::string str_last_packet;
            for ( int cnt_pcks = 0; cnt_pcks < NUM_PACKETS; cnt_pcks++ ) {
                sent_payload = "Hello_UDP_RingBuffer [" + std::to_string(cnt_pcks) + "]";
                std::cout << "[TEST] --->> Sending packet size : " << sent_payload.size() << std::endl;
                std::cout << "[TEST] --->> Sending packet: \"" << sent_payload << "\"\n";
                send_packet(sent_payload);
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                server.get_last_packet_async(last_packet); 
                size_t bytes_received = last_packet.size();
                str_last_packet.append(last_packet.begin(), last_packet.begin() + bytes_received);
                str_last_packet.erase(
                    std::remove(str_last_packet.begin(), str_last_packet.end(), '\0'),
                    str_last_packet.end()
                );
                std::cout << "[TEST] <<--- Received packet size : " << last_packet.size() << std::endl;
                std::cout << "[TEST] <<--- Packet fetched from server: \"" << str_last_packet << "\"\n";
                assert(str_last_packet == sent_payload && "ERROR: Fetched packet mismatch!");
                std::cout << "[TEST] Verification of get_last_packet(): PASSED\n";
                last_packet.clear();
                sent_payload.clear();
                sent_payload = "";
                str_last_packet.clear();
                str_last_packet = "";
            }
            std::cout << "[TEST] Requesting server stop..." << std::endl;
            server.stop();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            if (server_thread.joinable()) {
                server_thread.join();
            }
            std::cout << "[TEST] Server thread joined cleanly." << std::endl;
            std::cout << "[TEST] INTEGRATION TEST COMPLETED SUCCESSFULLY!" << std::endl;
        } catch (const ns_udp_server::udp_server_error& e) {
            std::cerr << "[TEST FAILED] Server component exception caught: " << e.what() << std::endl;
            return 1;
        } catch (const std::exception& e) {
            std::cerr << "[TEST FAILED] Standard generic exception caught: " << e.what() << std::endl;
            return 1;
        }
        return EXIT_SUCCESS;
    }

} // ns_receive_data_async

/**
 * @brief
 * @return EXIT_SUCCESS on success, or a non-zero value on failure.
 */
int main() {
    std::cout << "\n";
    int ret = EXIT_SUCCESS;
    ret = ns_receive_data_async::main_receive_data_async();
    if ( ret != EXIT_SUCCESS ) {
        return ret;
    }
    return EXIT_SUCCESS;
}
