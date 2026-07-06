#include <iostream>
#include <thread>
#include <chrono>
#include <cassert>
#include <cstring>

// Cross-platform minimal socket configuration for the test client
#ifdef _WIN32
    #include <winsock2.h>
    #pragma comment(lib, "ws2_32.lib")
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define SOCKET int
    #define INVALID_SOCKET -1
    #define closesocket close
#endif

#include "../../udp_server/udp_server.hpp"

const int TEST_PORT = 1581;

/**
 * @brief Helper function to send a raw UDP packet to the local server.
 * @param[in] message The payload string to be transmitted over UDP.
 */
void send_test_packet(const std::string& message) ;
void send_test_packet(const std::string& message) {
#ifdef _WIN32
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif

    SOCKET client_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_fd != INVALID_SOCKET) {
        sockaddr_in server_addr{};
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(TEST_PORT);
        inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
        //inet_pton(AF_INET, "192.168.1.5", &server_addr.sin_addr);

        // Transmit the raw data packet
        sendto(client_fd, message.c_str(), message.length(), 0, 
               (struct sockaddr*)&server_addr, sizeof(server_addr));
        
        closesocket(client_fd);
    }

#ifdef _WIN32
    WSACleanup();
#endif
}

int main() {
    std::cout << "[TEST] Starting integration test for udp_server..." << std::endl;

    try {
        // 1. Instantiate the server within the designated custom namespace
        ns_udp_server::udp_server server(TEST_PORT);

        // 2. Launch the server loop inside a dedicated background thread to prevent blocking
        std::thread server_thread([&server]() {
            server.start(); 
            std::cout << "[TEST] Udp server started\n";
        });

        // Small delay to ensure the server socket completes binding operations
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 3. Dispatch a test payload via the mock UDP client
        std::string sent_payload = "Hello_UDP_RingBuffer";
        std::cout << "[TEST] Sending packet size : " << sent_payload.size() << std::endl;
        std::cout << "[TEST] Sending packet: " << sent_payload << std::endl;
        send_test_packet(sent_payload);

        // Small delay allowing the internal ring buffer to safely process and queue incoming data
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // 4. Validate the get_last_packet() retrieval mechanism
        // Note: Adjust return type handling if get_last_packet returns a cpacket object instead of a string
        std::vector< uint8_t > last_packet;
        server.get_last_packet(last_packet); 


        // Loop through each byte and cast it to char to print the letter
        //for (uint8_t byte : last_packet) {
        //    std::cout << static_cast<char>(byte) << " ";
        //}
        //std::cout<<"\n";

        std::string str_last_packet(last_packet.begin(), last_packet.end());
        std::cout << "[TEST] Received packet size : " << last_packet.size() << std::endl;
        std::cout << "[TEST] Packet fetched from server: " << str_last_packet << std::endl;

        // Assertive verification of the integration pipeline
        assert(str_last_packet == sent_payload && "ERROR: Fetched packet mismatch!");
        std::cout << "[TEST] Verification of get_last_packet(): PASSED" << std::endl;

        // 5. Validate the graceful teardown and stopping mechanism
        std::cout << "[TEST] Requesting server stop..." << std::endl;
        server.stop();

        // Synchronize and wait for the server execution thread to terminate cleanly
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

    return 0;
}

