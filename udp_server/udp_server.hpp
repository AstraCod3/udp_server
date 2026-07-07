/**
 * @file
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

#ifndef UDPSERVER_HEADER
#define UDPSERVER_HEADER

#include <stdio.h>
#include <string.h>
#include <iostream>
#include <string>
#include <cstring> // std::strerror e memset
#include <array>
#include <vector>
#include <cstdlib>
#include <cmath> // std::abs
#include <sstream>
#include <mutex>
#include <condition_variable>

#if defined _WIN64 || _WIN32
#include <winsock2.h> // Socket,Bind,Recv,etc
#include <Winbase.h> // FormatMessage
#include <Ws2tcpip.h>
#pragma comment (lib, "WS2_32.lib")
#endif

#if defined __linux__ || __unix__
#include <sys/socket.h> // socket(), bind(), recvfrom(), sockaddr
#include <netinet/in.h> // sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>  // inet_addr(), inet_ntoa()
#include <unistd.h> // close()
#include <cerrno> // errno
#include <unistd.h>
#endif

/**
 * @namespace ns_udp_server
 * @brief Gather all class, struct, to handling udp server
 */
namespace ns_udp_server {

    /**
     * @brief Max theoretical UDP payload size.
     */
    const std::size_t max_size_udp_rx = 65535;

    /**
     * @enum server_status
     * @brief Represents the operational lifecycle states of the UDP server.
     * 
     * This scoped enumeration provides thread-safe visibility into the current.
     * execution phase of the network engine.
     */
    enum class SERVER_STATUS {
        /** @brief The server is instantiated and undergoing initial resource configuration. */
        INIT,
        /** @brief The server socket initialization has succeeded and the system is ready to launch. */
        START,
        /** @brief The main execution loop is active, processing and queuing incoming UDP packets. */
        RUNNING,
        /** @brief A shutdown signal has been triggered; the loop will unblock and terminate execution safely. */
        STOP
    };

    /**
     * @class udp_server_error
     * @brief Custom exception thrown when a operation is failed.
     * @details Inherits from std::runtime_error to maintain compatibility with standard
     *          exception handling while preventing exception pollution.
     */
    class udp_server_error : public std::runtime_error {

        public:

        /**
         * @brief Constructor for udp_server_error.
         * @param _msg [in] The detailed of error message.
         */
        explicit udp_server_error( const std::string& _msg ) :
            std::runtime_error( _msg ) { }


        /**
         * @brief Constructs a new udp_server_error exception with detailed diagnostic context.
         *
         * Combines the primary failure message, the platform-specific numeric error code,
         *          and the human-readable system error string into a single formatted output. This string
         *          is propagated directly to the underlying `std::runtime_error` base class.
         *
         * @param _msg [in] Contextual description of where and why the exception was triggered.
         * @param _err_code_num [in] The numeric error code captured from the operating system (e.g., errno or GetLastError()).
         * @param _err_code_str [in] The translated string description corresponding to the numeric system error.
         */
        explicit udp_server_error( const std::string& _msg,
                                    const int& _err_code_num,
                                    const std::string& _err_code_str ) :
            std::runtime_error( _msg + "\nError Code Num : " + std::to_string( _err_code_num ) + "\nError Code Str : " + _err_code_str ) { }
    };


    /**
     * @struct ring_buffer
     * @brief Monolithic single-buffer circular ring engine for high-speed UDP ingestion.
     */
    struct ring_buffer {

        public :
        /**
         * @brief Constructs the circular ring buffer and allocates the contiguous memory pool.
         * 
         * Instantiates the buffer state and attempts a large heap allocation for the 
         * underlying storage array.
         * 
         * @throws udp_server_error Wraps standard memory failures (`std::bad_alloc`) into the 
         *                          custom module exception if the block allocation fails.
         */
        ring_buffer() : mfirst_packet(false) {
            try {
                // If this massive allocation fails, C++ automatically throws std::bad_alloc
                mdata = std::make_unique<std::array<uint8_t, max_num_bytes>>();
            }
            catch (const std::bad_alloc& e) {
                // std::cerr << "Allocation failed! Error details: " << e.what() << std::endl;
                throw udp_server_error( "Called \"ring_buffer::ring_buffer()\" Allocation failed! Error details : " + std::string(e.what()) );
            }
        }

        /**
         * @brief Calculates the safe memory address for the next network data ingestion.
         * 
         * @note Thread-safe. Maintains mathematical continuity from the last known 'end' index
         *       even if the metadata queue becomes temporarily empty.
         * 
         * @return uint8_t* Base memory address inside the contiguous data pool, 
         *                  or nullptr if an overflow condition occurs.
         */
        uint8_t* get_next_offset() {
            std::unique_lock<std::mutex> lck(mmtx_offsets);
              
            // FIX: Maintain linear continuity from m_last_end_index, regardless of whether moffsets is empty or not
            size_t next_write_index = (m_last_end_index + 1) % max_num_bytes;
              
            // Anti-collision check: Ensure we do not overwrite unread packets still owned by the user
            if (!moffsets.empty()) {
                size_t oldest_unread_index = moffsets.front().begin;
                  
                size_t free_space = (oldest_unread_index > next_write_index) 
                                    ? (oldest_unread_index - next_write_index) 
                                    : (max_num_bytes - next_write_index + oldest_unread_index);
                  
                if (free_space < max_size_udp_rx ) {
                    return nullptr; // Buffer Overflow protection: drop the frame
                }
            }
              
            return &((*mdata)[next_write_index]);
        }

        /**
         * @brief Commits the newly received packet size into history and updates the permanent cursor.
         * 
         * @param bytes_len The exact number of bytes returned by the underlying system `recvfrom` call.
         */
        void commit_packet(std::size_t bytes_len) {
            std::unique_lock<std::mutex> lck(mmtx_offsets);
            offset tmp_offset;
              
            // Continuous calculation based on the permanent tracker
            tmp_offset.begin = (m_last_end_index + 1) % max_num_bytes;
            tmp_offset.end = (tmp_offset.begin + bytes_len - 1) % max_num_bytes;
              
            // Update the permanent tracking state for the next cycle
            m_last_end_index = tmp_offset.end;
              
            moffsets.push_back(tmp_offset);
              
            // If it's the first packet in the current batch, notify the waiting consumer thread
            if (moffsets.size() == 1) {
                mcv_first_packet.notify_one();
            }
        }

        /**
         * @brief Extracts and copies the oldest unread network packet from the circular buffer.
         * 
         * @note Thread-safe and optimized. The internal metadata mutex (`mmtx_offsets`) is 
         *       held only during the index resolution phase. It is explicitly released *before* 
         *       the physical memory transfer occurs, ensuring the network ingestion thread 
         *       is never stalled by application-level memory operations.
         * 
         * @param[out] output_buffer Vector or string where the packet payload will be copied.
         * @return bool True if a packet was successfully extracted; false if the buffer is empty.
         */
        bool get_last_packet(std::vector<uint8_t>& output_buffer) {
            offset target_offset;
            
            // Critical Section: Acquire lock to read and mutate metadata structures only
            {
                std::unique_lock<std::mutex> lck(mmtx_offsets);
                
                if (moffsets.empty()) {
                    return false; // No packets available to process
                }
                
                // Retrieve the geometric boundaries of the oldest unread packet (FIFO order)
                target_offset = moffsets.front();
            } // Lock is implicitly released here via RAII
            
            // Out of Lock Execution: Perform the actual memory copy (De-deferred Copy).
            // The ingestion thread can now safely write to other regions of mdata in parallel.
            size_t packet_size = 0;
            if (target_offset.end >= target_offset.begin) {
                // Scenario A: The packet data is linear and contiguous
                packet_size = (target_offset.end - target_offset.begin) + 1;
                output_buffer.resize(packet_size);
                std::memcpy(output_buffer.data(), &((*mdata)[target_offset.begin]), packet_size);
            } else {
                // Scenario B: The packet data wrapped around the circular memory ceiling
                size_t first_part = max_num_bytes - target_offset.begin;
                size_t second_part = target_offset.end + 1;
                packet_size = first_part + second_part;
                
                output_buffer.resize(packet_size);
                std::memcpy(output_buffer.data(), &((*mdata)[target_offset.begin]), first_part);
                std::memcpy(output_buffer.data() + first_part, &((*mdata)[0]), second_part);
            }
            
            return true;
        }

        /**
         * @brief Set the flag and notify waiting threads when the first packet is received.
         */
        void set_first_packet_received() {
            std::unique_lock<std::mutex> lck_fp( mmtx_first_packet );
            if ( !mfirst_packet ) {
                mfirst_packet = true;
                mcv_first_packet.notify_one(); 
            }
        }

        /**
         * @brief Blocks the calling thread until the very first UDP packet is successfully received.
         * 
         * This function uses a condition variable to efficiently put the calling thread to sleep,
         * ensuring zero CPU waste while waiting for network data ingestion to begin. If the first
         * packet has already arrived, it returns immediately without blocking.
         *
         * @note This function is **BLOCKING** only until the internal `mfirst_packet` flag evaluates to true.
         */
        void waiting_first_packet_received() {
            std::unique_lock<std::mutex> lck_fp( mmtx_first_packet );
            if ( !mfirst_packet ) {
                mcv_first_packet.wait(lck_fp, [this] { return mfirst_packet; });
            }
        }


        private :

        /**
         * @brief Total slots available in the ring buffer.
         */
        static constexpr std::size_t max_packets = 10;

        /**
         * @brief Total bytes of ring buffer.
         */
        static constexpr std::size_t max_num_bytes = max_size_udp_rx * max_packets;
        
        /**
         * @brief Mutex for mdata.
         */
        std::mutex mmtx_data;

        /**
         * @brief Flat contiguous memory block allocating on the heap.
         */
        std::unique_ptr< std::array<uint8_t, max_num_bytes> > mdata;

        /**
         * @brief Chronological history tracking the real size of each received packet.
         */
        struct offset {
            offset() : begin(0),end(0) { }
            virtual ~offset() { }
            std::size_t begin = 0;
            std::size_t end = 0;
        };

        /**
         * @brief Mutex to serialize access to the packet offsets vector.
         */
        std::mutex mmtx_offsets;

        /**
         * @brief Vector storing the offset information for each buffered packet.
         */
        std::vector< offset > moffsets;

        /**
         * @brief Mutex to synchronize status and operations on the very first packet.
         */
        std::mutex mmtx_first_packet;

        /**
         * @brief Condition Variable to notify the first packet is received.
         */
        std::condition_variable mcv_first_packet;

        /**
         * @brief Flag indicating whether the first packet has been received.
         */
        bool mfirst_packet = false;

        /**
         * @brief Permanent tracker of the absolute last written byte index.
         *        Initialized so that the very first packet starts exactly at index 0.
         */
        size_t m_last_end_index = max_num_bytes - 1;
    };


    /**
     * @class udp_server
     * @brief A cross-platform, single-header UDP server with automated packet management.
     * 
     * The `udp_server` class provides an efficient, zero-configuration solution for 
     * real-time systems running on both Windows (Winsock) and Linux/Unix environments. 
     * It handles low-level socket abstraction under a unified, thread-safe API.
     * 
     * @details Internal architecture highlights:
     * - **Automated Packet Management:** Integrates an internal, thread-safe ring buffer 
     *   to automatically queue and synchronize incoming data under the hood.
     * - **Thread Safety:** State changes and error reporting are fully synchronized via 
     *   atomic operations (`std::atomic`) and mutexes.
     * - **Blocking Synchronizations:** Implements conditional blocking mechanisms for safe 
     *   startup tracking, raw data ingestion, and orderly system shutdowns.
     * 
     * @note Being part of a header-only module, this class requires no external compilation 
     *       or linking. Simply include the header file to deploy it.
     */
    class udp_server {

        public:

        /**
         * @brief Constructs the UDP server and initializes its core states.
         * 
         * Binds the server instance to the specified local port and places
         * the operational state into `SERVER_STATUS::INIT`.
         * 
         * @param _lport The local port number on which the server will listen for incoming traffic.
         */
        explicit udp_server( unsigned short _lport ) :
                mlocal_port(_lport),
                merror_code_num(0),
                merror_code_str(""),
                msocket(0) {
            m_status.store(SERVER_STATUS::INIT);
        }

        /**
         * @brief Deleted copy constructor.
         * 
         * A UDP server manages unique, non-shareable system resources (sockets, threads, 
         * and internal buffers). Duplicating the instance is strictly prohibited to 
         * prevent hardware conflicts and undefined behavior.
         */
        udp_server( const udp_server& ) = delete;

        /**
         * @brief Deleted copy assignment operator.
         * 
         * Copying server states or resources via assignment is disabled to enforce 
         * strict resource ownership and avoid race conditions or double-close errors.
         */
        udp_server& operator=( const udp_server& ) = delete;

        /**
         * @brief Destructor
         */
        virtual ~udp_server( ) { }

        /**
         * @brief Initializes the network subsystem and starts the primary UDP reception loop.
         *
         * @note This function is **BLOCKING**. It will block the calling thread indefinitely
         *       inside the internal `receive_from()` loop until the server is explicitly stopped.
         *
         * @throws udp_server_error If Winsock initialization fails, if the socket cannot be
         *                          opened, or if binding to the local port fails.
         */
        void start( ) {
            m_status.store(SERVER_STATUS::START);
            open_socket();
            bind_socket();
            receive_from();
        }

        /**
         * @brief Gracefully stops the UDP server by updating its status and unblocking the receive thread.
         */
        void stop() {
            m_status.store(SERVER_STATUS::STOP);

        #if defined(_WIN64) || defined(_WIN32)
            SOCKET wake_sock = socket(AF_INET, SOCK_DGRAM, 0);
                if (wake_sock != INVALID_SOCKET) {
        #elif defined(__linux__) || defined(__unix__)
                int wake_sock = socket(AF_INET, SOCK_DGRAM, 0);
                if (wake_sock >= 0) {
        #endif
                    sockaddr_in loopback_addr{};
                    loopback_addr.sin_family = AF_INET;
                    loopback_addr.sin_port = htons(mlocal_port);
                    inet_pton(AF_INET, "127.0.0.1", &loopback_addr.sin_addr);

                    // Transmit 0 bytes payload. This safely wakes up recvfrom() without carrying data.
                    sendto(wake_sock, nullptr, 0, 0, reinterpret_cast<struct sockaddr*>(&loopback_addr), sizeof(loopback_addr));

                #if defined(_WIN64) || defined(_WIN32)
                    closesocket(wake_sock);
                #elif defined(__linux__) || defined(__unix__)
                    close(wake_sock);
                #endif
                }

                bool err_close = false;

            #if defined(_WIN64) || defined(_WIN32)
                // On Windows, msocket might be checked against INVALID_SOCKET
                if (msocket != INVALID_SOCKET) {
                    if (closesocket(msocket) == SOCKET_ERROR) {
                        err_close = true;
                    }
                    msocket = INVALID_SOCKET;
                }
            #elif defined(__linux__) || defined(__unix__)
                if (msocket != -1) {
                    if (close(msocket) != 0) {
                        err_close = true;
                    }
                    msocket = -1;
                }
            #endif


            // If an error occurred during shutdown, we log it instead of throwing an exception
            if (err_close) {
                set_err_sys();
                throw udp_server_error("Exception \"udp_server::stop()\"", get_error_code_num(), get_error_code_str() );
            }
        }

        /**
         * @brief Set local port
         * @param _local_port [in] Local port
         */
        void set_local_port( unsigned short _local_port ) { mlocal_port = _local_port; }

        /**
         * @brief Get local port
         * @return Local Port
         */
        unsigned short get_local_port() { return mlocal_port; }

        /**
         * @brief Data Copy of the last packet received
         * @param last_packet_ [out] The destination buffer where the packet payload will be copied.
         */
        void get_last_packet( std::vector< uint8_t >& last_packet_ ) {
            mring_buffer.get_last_packet( last_packet_ );
        }

        /**
         * @brief Retrieves the current server status as an enum value.
         *
         * @note This function is fully thread-safe and relies on relaxed memory ordering
         *       to guarantee zero performance overhead during high-frequency polling.
         *
         * @return SERVER_STATUS The current active state of the server.
         */
        [[nodiscard]] SERVER_STATUS get_status_enum() const noexcept {
            return m_status.load(std::memory_order_relaxed);
        }

        /**
         * @brief Retrieves the current server status as a string representation.
         * 
         * @note Thread-safe. Fetches the atomic state and converts it into a non-allocating
         *       string view wrapper, making it ideal for low-latency logging and debugging.
         * 
         * @return std::string_view A text view matching the status name ("INIT", "RUNNING", etc.).
         */
        [[nodiscard]] std::string_view get_status_str() const noexcept {
            // Fetch the atomic value into a local snapshot before evaluating the switch
            const SERVER_STATUS current = m_status.load(std::memory_order_relaxed);
            switch (current) {
                case SERVER_STATUS::INIT:    return "INIT";
                case SERVER_STATUS::START:   return "START";
                case SERVER_STATUS::RUNNING: return "RUNNING";
                case SERVER_STATUS::STOP:    return "STOP";
                default:                     return "UNKNOWN_STATUS";
            }
        }

        protected:

        /**
         * @brief Local Port.
         */
        unsigned short mlocal_port;

        /**
         * @brief Mutex Error.
         */
        std::mutex mmtx_error;

        /**
         * @brief Error code number.
         */
        int merror_code_num;

        /**
         * @brief Error code string.
         */
        std::string merror_code_str;

        /**
         * @brief Mutex of ring buffer.
         */
        ring_buffer mring_buffer;

    #if defined _WIN64 || _WIN32
        /**
         * @brief Socket File Descriptor.
         */
        SOCKET msocket;
    #endif

    #if defined __linux__ || __unix__
        /**
         * @brief Socket File Descriptor.
         */
        int msocket;
    #endif

        /**
         * @brief Atomic variable tracking the absolute current runtime status of the server.
         *
         * This variable is accessed concurrently across multiple execution threads
         * (e.g., the worker loop thread and the control thread calling stop()).
         * Using std::atomic guarantees strict sequential consistency and prevents
         * data races without the heavy overhead of standard mutexes.
         */
        std::atomic<SERVER_STATUS> m_status{ SERVER_STATUS::INIT };

        /**
         * @brief Initializes the network subsystem and creates a cross-platform UDP socket.
         * 
         * On Windows platforms, this function automatically handles the initialization of 
         * the Winsock subsystem (WSAStartup) before attempting to allocate resources. It then
         * requests a Datagram socket (`SOCK_DGRAM`) configured for the IPv4 family (`AF_INET`)
         * using Winsock on Windows and POSIX interfaces on Linux/Unix.
         *
         * @throws udp_server_error **[Windows & Linux]** Thrown if the operating system fails
         *                          to allocate an operational socket descriptor. On Windows,
         *                          this is also triggered if the network subsystem (Winsock)
         *                          fails to initialize.
         */
        void open_socket( ) {
            bool error_socket = false;

        #if defined _WIN64 || _WIN32
            WSAData data;
            ret = WSAStartup(MAKEWORD(2, 2), &data);
            if (ret != EXIT_SUCCESS) {
                set_err_sys();
                throw udp_server_error("Called \"udp_server::start_server() WSAStartup() != EXIT_SUCCESS\"", get_error_code_num(), get_error_code_str() );
            }
            msocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if ( msocket == INVALID_SOCKET )
                error_socket = true;
        #endif

        #if defined __linux__ || __unix__
            msocket = socket(AF_INET, SOCK_DGRAM, 0);
            if ( msocket < 0 )
                error_socket = true;
        #endif

            if ( error_socket ) {
                set_err_sys();
                throw udp_server_error( "Exception \"udp_server::open_socket()\"", get_error_code_num(), get_error_code_str() );
            }
        }

        /**
         * @brief Binds the UDP socket to the local port and configures the address structure.
         *
         * Automatically handles cross-platform differences between Windows (Winsock)
         * and Linux/Unix sockets. It configures the system to accept IPv4 incoming
         * traffic on any available local network interface (INADDR_ANY) using the designated port.
         *
         * @throws udp_server_error If the operating system fails to bind the socket interface,
         *                          embedding the platform-specific error code and string message.
         */
        void bind_socket( ) {
            bool err_bind = false;

        #if defined _WIN64 || _WIN32
            sockaddr_in addr;
            ///< AF_INET = IPV4
            addr.sin_family = AF_INET;
            addr.sin_port = htons(mlocal_port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if ( bind(msocket, (SOCKADDR*)(&addr), sizeof(addr)) != EXIT_SUCCESS ) {
                err_bind = true;
            }
        #endif

        #if defined __linux__ || __unix__
           // struct sockaddr_in  addr;
           sockaddr_in addr;
           memset(&addr, 0, sizeof(addr));
           addr.sin_family = AF_INET;
           addr.sin_port = htons(mlocal_port);
           addr.sin_addr.s_addr = htonl(INADDR_ANY);
           if (bind(msocket, (struct sockaddr *) &addr, sizeof(addr)) < 0 ) {
               err_bind = true;
           }
        #endif

            if ( err_bind ) {
                set_err_sys();
                throw udp_server_error("Exception \"udp_server::bind_socket()\" != EXIT_SUCCESS", get_error_code_num(), get_error_code_str() );
            }
        }

        /**
         * @brief Sets a custom string message describing the system error.
         * @param _err_msg Pointer to a null-terminated string containing the error description.
         */
        void set_error_code_str( const char* _err_msg ) {
            std::unique_lock<std::mutex> lck(mmtx_error);
            merror_code_str = _err_msg;
            lck.unlock();
        }

        /**
         * @brief Retrieves both the error code and its descriptive message simultaneously.
         * @param[out] err_code_ Reference where the internal numeric error code will be stored.
         * @param[out] err_msg_ Reference where the internal error string message will be stored.
         */
        void get_error( int& err_code_, std::string& err_msg_ ) {
            std::unique_lock<std::mutex> lck(mmtx_error);
            err_code_ = merror_code_num;
            err_msg_ = merror_code_str;
            lck.unlock();
        }

        /**
         * @brief Gets the current internal numeric error code.
         * @return int The numeric system error code (returns EXIT_SUCCESS if no error occurred).
         */
        int get_error_code_num ( void ) {
            int ret = EXIT_SUCCESS;
            std::unique_lock<std::mutex> lck(mmtx_error);
            ret = merror_code_num;
            lck.unlock();
            return ret;
        }

        /**
         * @brief Gets the current descriptive error string.
         * @return std::string A copy of the string containing the error description.
         */
        std::string get_error_code_str ( ) {
            std::string ret;
            std::unique_lock<std::mutex> lck(mmtx_error);
            ret = merror_code_str;
            lck.unlock();
            return ret;
        }

        /**
         * @brief Captures the last system-level socket error and translates it into internal states.
         * 
         * Queries the underlying operating system APIs to extract the precise failure reason.
         * On Windows, it reads from Winsock via GetLastError() and converts the code into a 
         * human-readable string using FormatMessage(). On Linux/Unix, it evaluates standard errno values.
         */
        void set_err_sys ( ) {
        #if defined _WIN64 || _WIN32
            LPVOID lpBuffer;
            const DWORD err = GetLastError();

            FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                            NULL, // lpsource
                            err, // message id
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // languageid
                            //(wchar_t*)lpBuffer,
                            //(LPWSTR*)(&lpBuffer),
                            reinterpret_cast<LPTSTR>(&lpBuffer),
                            0,
                            NULL);

            //
            // concatonate string to DisplayBuffer
            // const std::wstring DisplayBuffer = L" failed with error " + std::to_wstring(err) + L"\n" + static_cast<wchar_t*>(lpBuffer);
            // Display the error message and exit the process
            // MessageBoxExW(NULL, DisplayBuffer.c_str(), L"Error", MB_ICONERROR | MB_OK, static_cast<WORD>(err));
            //

            std::unique_lock<std::mutex> lck(mmtx_error);

            const std::wstring tempwstr = static_cast<wchar_t*>(lpBuffer);
            std::string str;
            for (std::wstring::const_iterator it = tempwstr.begin(); it != tempwstr.end(); ++it)
                merror_code_str.push_back(static_cast<char>(*it));

            //
            // std::stringstream ss;
            // ss << static_cast<char*>(tempwstr.c_str());
            // merror_code_str = ss.str();
            // merror_code_str = reinterpret_cast<char*>(tempwstr.c_str());
            //

            merror_code_num = err;
            lck.unlock();

            if (NULL != lpBuffer) {
                LocalFree(lpBuffer);
                lpBuffer = NULL;
            }
        #endif

        #if defined __linux__ || __unix__
            std::unique_lock<std::mutex> lck(mmtx_error);
            merror_code_num = errno;
            merror_code_str = std::strerror(errno);
            lck.unlock();
        #endif
        }

        /**
         * @brief Core packet reception loop that processes incoming UDP traffic.
         * 
         * This function handles the low-latency network data ingestion. It continuously
         * queries the underlying socket and writes data blocks directly into the internal, 
         * automated thread-safe ring buffer.
         * 
         * @note This function is **BLOCKING**. It updates the server operational state
         *       to `SERVER_STATUS::RUNNING` immediately before entering its primary execution loop.
         * 
         * @warning This execution loop runs indefinitely until the underlying socket is closed 
         *          or an explicit shutdown/stop state condition unblocks it.
         */
        void receive_from() {
            bool first_packet_received = false;
            bool run = true;

        #if defined _WIN64 || _WIN32
            static struct sockaddr_in client;
            static const int size_client = sizeof(client);
        #endif

        #if defined __linux__ || __unix__
            struct sockaddr_in client;
            socklen_t size_client;
        #endif

            uint8_t* write_ptr = mring_buffer.get_next_offset();
            m_status.store(SERVER_STATUS::RUNNING);

            while ( run ) {

                std::memset(&client, 0, sizeof(client));

            #if defined _WIN64 || _WIN32
                int num_bytes_rx = recvfrom(msocket,
                                    (char*)write_ptr,
                                    max_size_udp_rx,
                                    0,
                                    reinterpret_cast<SOCKADDR*>(&msclient), &size_client );

                if ( num_bytes_rx == SOCKET_ERROR || num_bytes_rx < 0 ) {
                    set_err_sys();
                    throw udp_server_error( "Called \"udp_server::receive_from() SOCKET_ERROR\"", get_error_code_num(), get_error_code_str() );
                    break;
                }
            #endif

            #if defined __linux__ || __unix__
                size_client = sizeof(client);
                errno = 0;
                ssize_t num_bytes_rx = recvfrom(msocket,
                                    (unsigned char*)write_ptr,
                                    max_size_udp_rx,
                                    0,
                                    (sockaddr*)&client,
                                    &size_client );

                if ( num_bytes_rx < 0 ) {
                    set_err_sys();
                    throw udp_server_error( "Called \"udp_server::recvfrom() < 0\"", get_error_code_num(), get_error_code_str() );
                    break;
                }
            #endif

                SERVER_STATUS current_state = m_status.load();

                // 1. Check if a shutdown signal was issued while we were blocked in the kernel
                if ( current_state == SERVER_STATUS::STOP )
                    break; // Break the while loop instantly, ignoring any dummy or partial data


                if ( num_bytes_rx >= 0 ) {
                    if ( !first_packet_received ) {
                        first_packet_received = true;
                        mring_buffer.set_first_packet_received();
                    }
                    mring_buffer.commit_packet( num_bytes_rx );
                    write_ptr = mring_buffer.get_next_offset();
                }

            } // while ( run )
        }

    }; // class udp_server

} // namespace ns_udp_server

#endif
