/**
 * @file udp_server.hpp
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

#include <array>
#include <condition_variable>
#include <cstdlib>
#include <cstdint>
#include <cstring> // std::strerror e memset
#include <iostream>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <stdio.h>
#include <string.h>
#include <string>
#include <thread>
#include <vector>

#if defined _WIN64 || _WIN32
//#include <windows.h>
#include <winsock2.h> // Socket,Bind,Recv,etc
#include <Winbase.h> // FormatMessage
#include <Ws2tcpip.h>
#pragma comment (lib, "WS2_32.lib")
#endif

#if defined __linux__ || __unix__
#include <arpa/inet.h>  // inet_addr(), inet_ntoa()
#include <cstdint>
#include <cstddef>
#include <fcntl.h>
#include <netinet/in.h> // sockaddr_in, INADDR_ANY, htons()
#include <sys/mman.h>
#include <sys/socket.h> // socket(), bind(), recvfrom(), sockaddr
#include <unistd.h> // close()
#include <cerrno> // errno
#endif

/**
 * @namespace ns_udp_server
 * @brief Gather all class, struct, to handling udp server
 */
namespace ns_udp_server {

    /**
     * @brief Max theoretical UDP payload size.
     */
    const std::size_t MAX_SIZE_UDP_RX = 65536;

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
         * @brief Constructs a new udp_server_error exception with detailed diagnostic context.
         *
         * Combines the primary failure message, the platform-specific numeric error code,
         *  and the human-readable system error string into a single formatted output.
         * This string is propagated directly to the underlying `std::runtime_error` base class.
         */
        explicit udp_server_error( const std::string& _msg ) :
            std::runtime_error( _msg + get_error_code_sys() + get_error_str_sys() ) { }
       
        /**
         * @brief Captures the last system-level socket error and translates it into internal states.
         * 
         * Queries the underlying operating system APIs to extract the precise failure reason.
         *  On Windows, it reads from Winsock via GetLastError() and converts the code into a
         *  human-readable string using FormatMessage(). On Linux/Unix, it evaluates standard errno values.
         *
         * @return Error string code
         */
        static std::string get_error_code_sys( ) {
            std::string ret_str_num;
        #if defined _WIN64 || _WIN32
            ret_str_num = std::to_string( GetLastError() );
        #endif
        #if defined __linux__ || __unix__
            ret_str_num = std::to_string( errno );
        #endif
            ret_str_num = "\nError Code Num : " + ret_str_num;
            return ret_str_num;
        }

        /**
         * @brief Captures the last system-level socket error and translates it into message.
         * @return Error string message
         */
        static std::string get_error_str_sys() {
            std::string error_code_str;

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

            const std::wstring tempwstr = static_cast<wchar_t*>(lpBuffer);
            std::string str;
            for (std::wstring::const_iterator it = tempwstr.begin(); it != tempwstr.end(); ++it)
                error_code_str.push_back(static_cast<char>(*it));

            if (NULL != lpBuffer) {
                LocalFree(lpBuffer);
                lpBuffer = NULL;
            }
        #endif

        #if defined __linux__ || __unix__
            error_code_str = std::strerror(errno);
        #endif

            return error_code_str;
        }
    };


    /**
     * @class ring_buffer
     * @brief High-performance, Zero-Copy circular ring buffer using virtual memory mirroring.
     * @note The buffer capacity (max_num_bytes) must be a power of 2 to allow fast bitwise masking.
     */
    class ring_buffer {

        public:

        /**
         * @brief Total slots available in the ring buffer.
         */
        static constexpr std::size_t MAX_PACKETS = 4;

        /**
         * @brief Total bytes of the ring buffer.
         * @note Must be a power of 2
         */
        static constexpr std::size_t MAX_NUM_BYTES = MAX_SIZE_UDP_RX * MAX_PACKETS;

        /**
         * @brief Threshold od index, & a bit operation instead of %
         */
        static constexpr std::size_t THRESHOLD_INDEX = MAX_NUM_BYTES - 1;

        /**
         * @brief Construct a new mirrored ring buffer object.
         * @details Reserves a contiguous virtual address space equal to twice the capacity 
         *          and maps both halves to the same underlying physical memory page.
         * @throws udp_server_error If any OS-level allocation or mapping step fails.
         */
        ring_buffer() :
            mview1(nullptr),
            mview2(nullptr),
            mdata(nullptr),
            mdata_base(nullptr) {
        #if defined _WIN64 || _WIN32
            for (int retry = 0; retry < 5; ++retry) {
                /**
                 * @brief 1. Probe the virtual address space to find a free region equal to twice the capacity
                 */
                uint8_t* target_address = reinterpret_cast<uint8_t*>(::VirtualAlloc(
                                            nullptr, 2 * MAX_NUM_BYTES, MEM_RESERVE, PAGE_NOACCESS));

                if (!target_address)
                    continue;

                /**
                 * @brief 2. Instantly release the reservation so MapViewOfFileEx can map directly onto these addresses
                 */
                ::VirtualFree(target_address, 0, MEM_RELEASE);

                /**
                 * @brief 3. Create a shared memory-backed file mapping object using the Windows paging file
                 */
                mh_map = ::CreateFileMappingW( INVALID_HANDLE_VALUE,
                                                nullptr, PAGE_READWRITE,
                                                0, static_cast<DWORD>(MAX_NUM_BYTES),
                                                nullptr );

                if (!mh_map || mh_map == INVALID_HANDLE_VALUE) {
                    mh_map = INVALID_HANDLE_VALUE;
                    continue;
                }

                /**
                 * @brief 4. Map the first half of the virtual address space to the physical memory
                 */
                mview1 = reinterpret_cast<uint8_t*>(::MapViewOfFileEx(
                                                        mh_map, FILE_MAP_ALL_ACCESS,
                                                        0, // dwFileOffsetHigh
                                                        0, // dwFileOffsetLow
                                                        MAX_NUM_BYTES,
                                                        target_address));

                /**
                 * @brief 5. Map the second half of the virtual address space to the EXACT SAME physical memory
                 */
                mview2 = reinterpret_cast<uint8_t*>(::MapViewOfFileEx(
                                                        mh_map, FILE_MAP_ALL_ACCESS,
                                                        0, // dwFileOffsetHigh
                                                        0, // dwFileOffsetLow
                                                        MAX_NUM_BYTES,
                                                        target_address + MAX_NUM_BYTES));

                /**
                 * @brief If both mappings succeeded, initialize pointers and return
                 */
                if ( mview1 && mview2 ) {
                    mdata = mview1;
                    mdata_base = mview1;
                    return; 
                }

                /**
                 * @brief Cleanup failed attempt resources before retrying
                 */
                cleanup();
            }
            throw udp_server_error("Called \"ring_buffer::ring_buffer()\" Allocation failed! MapViewOfFileEx mirroring failed permanently.");

            
        #endif

        #if defined __linux__ || __unix__

            /**
             * @brief Allocates a mirrored virtual memory ring buffer on Unix-like operating systems.
             * 
             * This function creates a virtual circular buffer by mapping the same physical
             * memory page(s) twice into adjacent virtual address regions. This allows contiguous
             * linear memory access even when data wraps around the end of the buffer boundaries,
             * avoiding manual wrap-around checks.
             * 
             * @return void* A pointer to the start of the ring buffer (`view1`) on success, 
             *               or `nullptr` if any system call fails.
             */

            /**
             * @brief 1. Create an anonymous RAM-backed file descriptor.
             * 
             * `memfd_create` creates a file that resides entirely in RAM, shared across descriptors
             * but not visible within the standard filesystem. `MFD_CLOEXEC` ensures the descriptor
             * is closed automatically if the process spawns an `exec` call.
             */
            int fd = ::memfd_create("ring_buffer", MFD_CLOEXEC);
            if (fd == -1) {
                throw udp_server_error("Called \"ring_buffer::ring_buffer() memfd_create == -1\"");
            }

            /**
             * @brief Resize the backing physical file to the exact single buffer capacity.
             */
            if ( ::ftruncate(fd, MAX_NUM_BYTES) == -1 ) {
                ::close(fd);
                throw udp_server_error("Called \"ring_buffer::ring_buffer() ftruncate == -1\"");
            }

            /**
             * @brief 2. Probe and reserve a contiguous virtual address space equal to TWICE the capacity.
             * 
             * `MAP_ANONYMOUS | MAP_PRIVATE` requests a placeholder virtual region. `PROT_NONE` ensures
             * no physical memory or swap space is allocated to this range yet. It simply marks the address
             * space as unavailable for other allocations.
             */
            void* target_address = ::mmap(nullptr, 2 * MAX_NUM_BYTES, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
            if ( target_address == MAP_FAILED ) {
                ::close(fd);
                throw udp_server_error("Called \"ring_buffer::ring_buffer() target_address == MAP_FAILED\"");
            }

            /**
             * @brief 3. Map the first half of the reserved virtual space to the physical backing file.
             * 
             * `MAP_FIXED` forces the kernel to instantly replace the `PROT_NONE` placeholder at `target_address`
             * with this shared mapping, eliminating any time-of-check-to-time-of-use (TOCTOU) race conditions.
             */
            mview1 = reinterpret_cast<uint8_t*> ( ::mmap(target_address,
                                                    MAX_NUM_BYTES,
                                                    PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0)
                                                );
            if ( mview1 == MAP_FAILED ) {
                ::munmap(target_address, 2 * MAX_NUM_BYTES);
                ::close(fd);
                throw udp_server_error("Called \"ring_buffer::ring_buffer() view1 == MAP_FAILED\"");
            }

            /**
             * @brief 4. Map the second half of the reserved virtual space to the EXACT SAME physical memory.
             * 
             * The offset within the file descriptor is 0, identical to the first mapping. This establishes
             * the mirroring mechanism: `target_address` and `target_address + MAX_NUM_BYTES` point to the
             * same physical backing bytes.
             */
            uint8_t* second_half_address = reinterpret_cast<uint8_t*>(target_address) + MAX_NUM_BYTES;
            mview2 = reinterpret_cast<uint8_t*>( ::mmap(second_half_address, MAX_NUM_BYTES, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, 0) );
            if ( mview2 == MAP_FAILED ) {
                ::munmap(target_address, 2 * MAX_NUM_BYTES);
                ::close(fd);
                throw udp_server_error("Called \"ring_buffer::ring_buffer() view2 == MAP_FAILED\"");
            }

            /**
             * @brief Close the file descriptor.
             * 
             * Active memory mappings remain valid even after closing the descriptor that created them,
             * as the kernel maintains an internal reference to the underlying
             * file resource until `munmap` is called.
             */
            ::close(fd);

            mdata = mview1;
            mdata_base = mview1;

        #endif
        }

        /**
         * @brief Disable copy semantics to prevent accidental duplication and double-free of OS resources.
         */
        ring_buffer(const ring_buffer&) = delete;
        ring_buffer& operator=(const ring_buffer&) = delete;

        /**
         * @brief Destroy the mirrored ring buffer object.
         * @details RAII compliant. Automatically unmaps views and releases all Windows OS kernel handles.
         */
        ~ring_buffer() {
            cleanup();
        }

        /**
         * @brief Get the raw base virtual pointer (index 0) of the buffer.
         * @return uint8_t* Pointer to the buffer start.
         */
        uint8_t* data() const { return mdata; }

        /**
         * @brief Wrap and advance the write pointer circularly after a successful write operation.
         * @details Employs a branchless bitwise masking trick to roll back the virtual pointer 
         *          to the first half of the memory space instantly if it crosses the threshold.
         * @param _bytes_len [in] The exact number of bytes returned by the underlying system `recvfrom` call.
         * @param next_ptr_ [out] Update adress to write next data
         */
        void advance_write_pointer( const std::size_t& _bytes_len, uint8_t*& next_ptr_ ) {
            moffsets.update_offsets(_bytes_len);
            mdata = mdata + _bytes_len;
            next_ptr_ = mdata;
        }

        /**
         * @brief This Extracts and copies the oldest unread network packet from the circular buffer.
         * @note This function is **BLOCKING** only until the internal `mreceived_packet` flag evaluates to true.
         * @param output_buffer_ [out] Vector or string where the packet payload will be copied.
         */
        void get_last_packet_async( std::vector<uint8_t>& output_buffer_ ) {
            std::thread thr = std::thread([&]() {
                get_last_packet( output_buffer_ );
            });
            if ( thr.joinable() ) {
                thr.join();
            }
        }

        /**
         * @brief This Extracts and copies the oldest unread network packet from the circular buffer.
         * @note This function is **BLOCKING** only until the internal `mreceived_packet` flag evaluates to true.
         * @param output_buffer_ [out] Vector or string where the packet payload will be copied.
         */
        void get_last_packet_sync( std::vector<uint8_t>& output_buffer_ ) {
            ///< Waiting unti the packet has been received
            waiting_packet_received() ;
            std::thread thr = std::thread([&]() {
                get_last_packet( output_buffer_ );
            });
            if ( thr.joinable() ) {
                thr.join();
            }
        }

        /**
         * @brief Set the flag and notify waiting threads when the packet is received.
         */
        void notify_packet_received() {
            std::unique_lock<std::mutex> lck_fp( mmtx_packet_received );
            mpacket_received = true;
            mcv_packet_received.notify_one();
        }

        /**
         * @brief Blocks the calling thread until the very first UDP packet is successfully received.
         *
         * This function uses a condition variable to efficiently put the calling thread to sleep,
         * ensuring zero CPU waste while waiting for network data ingestion to begin. If the first
         * packet has already arrived, it returns immediately without blocking.
         *
         * @note This function is **BLOCKING** only until the internal `mpacket_received` flag evaluates to true.
         */
        void waiting_packet_received() {
            std::unique_lock<std::mutex> lck_fp( mmtx_packet_received );
            mcv_packet_received.wait(lck_fp, [this] { return mpacket_received; });
            mpacket_received = false;
        }

        private :

        /**
         * @brief Chronological history tracking the real size of each received packet.
         */
        struct offset {

            public:

            /**
             * @brief Costructor
             */
            offset() : mbegin(0), mend(0) { }

            /**
             * @brief Destructor
             */
            virtual ~offset() { }

            /**
             * @brief Copy Costructor
             */
            offset(const offset& other) {
                // Lock the source object's mutex before reading its data members
                std::scoped_lock lock(other.mmtx_offset);
                
                // Safely copy the values while the source object is protected
                this->mbegin = other.mbegin;
                this->mend = other.mend;
                
                // Note: this->mmtx_offset is automatically initialized in its default state.
                // We DO NOT lock 'this->mmtx_offset' because the current object is still 
                // local to this thread and not yet accessible by other threads.
            }

            /**
             * @brief
             */
            offset& operator=(const offset& other) {
                // 1. MANDATORY: Protect against self-assignment to prevent deadlocks on the same mutex
                if (this == &other) {
                    return *this;
                }
                // 2. Lock both distinct mutexes simultaneously to avoid deadlocks (C++17 RAII)
                std::scoped_lock lock(this->mmtx_offset, other.mmtx_offset);
                // 3. Perform the actual data copy safely
                this->mbegin = other.mbegin;
                this->mend = other.mend;
                // 4. Return reference to the current object
                return *this; 
            }

            /*
             * @brief
             */
            offset& operator+=(const offset& other) {
                // 1. Check for self-increment (e.g., offsetA += offsetA)
                if (this == &other) {
                    // If it is the same object, lock ONLY this mutex to prevent deadlock
                    std::scoped_lock lock(this->mmtx_offset);
                    
                    // Correct behavior: self-increment doubles the internal values
                    this->mbegin += this->mbegin + 1;
                    this->mend += this->mend + 1;
                } else {
                    // 2. If objects are different, lock BOTH safely (C++17 RAII)
                    std::scoped_lock lock(this->mmtx_offset, other.mmtx_offset);
                    
                    // 3. Perform the safe cross-object addition
                    this->mbegin += other.mbegin + 1;
                    this->mend += other.mend +1;
                }

                // 4. Return reference to the current object
                return *this; 
            }

            /**
             * @brief
             * @return TRUE
             * @return FALSE
            */
            bool is_zero( ) {
                bool ret = false;
                std::unique_lock<std::mutex> lck( mmtx_offset );
                if ( mbegin == 0 && mend == 0 ) {
                    ret = true;
                }
                else {
                    ret = false;
                }
                lck.unlock();
                return ret;
            }

            /**
             * @brief
             */
            std::size_t get_begin() {
                std::size_t ret = 0;
                std::unique_lock<std::mutex> lck( mmtx_offset );
                ret = mbegin;
                lck.unlock();
                return ret;
            }

            /**
             * @brief
             */
            std::size_t get_end() {
                std::size_t ret = 0;
                std::unique_lock<std::mutex> lck( mmtx_offset );
                ret = mend ;
                lck.unlock();
                return ret;
            }

            /**
             * @brief Update offsets
             * @param [in] Number of bytes
             */
            void update_offsets( const std::size_t& _len ) {
                std::unique_lock<std::mutex> lck( mmtx_offset );
                if ( _len > 0 ) {
                    if ( mbegin == 0 && mend == 0 ) {
                        // first packet 
                        mbegin = 0;
                        mend = ( _len - 1 ) & THRESHOLD_INDEX;
                    }
                    else {
                        // other packets
                        mbegin = ( mend + 1 );
                        mend = ( mbegin + _len - 1) & THRESHOLD_INDEX;
                    }
                }
            }


            private:

            /**
             * @brief Mutex to serialize access to the packet offsets vector.
             */
            mutable std::mutex mmtx_offset;

            /**
             * @brief
             */
            std::size_t mbegin = 0;

            /**
             * @brief
             */
            std::size_t mend = 0;

        }; // struct offset

        /**
         * @brief Windows : Unmaps both virtual views from the process address space and closes
         *         the file mapping handle to release physical resources back to the OS.
         *
         * @brief Unix/Linux : Unmaps the entire contiguous virtual address space (both views at once)
         *         using a single system call.
         */
        void cleanup() {

        #if defined _WIN64 || _WIN32
            if ( mview2 ) {
                ::UnmapViewOfFile(mview2);
            }
            if ( mview1 ) {
                ::UnmapViewOfFile(mview1);
            }
            if (mh_map && (mh_map != INVALID_HANDLE_VALUE) ) {
                ::CloseHandle(mh_map);
            }
            if ( mdata ) {
                ::VirtualFree(mdata, 0, MEM_RELEASE);
            }
        #endif

        #if defined __linux__ || __unix__
            // mview1 is equal to the start of the target_address blocks
            if (mview1) {
                // Unmap both halves at once by specifying twice the capacity
                ::munmap(mview1, 2 * MAX_NUM_BYTES);
            }
            // Note: The file descriptor (fd) was already closed during initialization,
            // so no need to clean it up here.
        #endif

            mview1 = nullptr;
            mview2 = nullptr;
            mdata = nullptr;
            mdata_base = nullptr;
        }

        /**
         * @brief This Extracts and copies the oldest unread network packet from the circular buffer.
         * @note This function is **BLOCKING** only until the internal `mfirst_packet` flag evaluates to true.
         * @param[out] output_buffer Vector or string where the packet payload will be copied.
         */
        void get_last_packet(std::vector<uint8_t>& output_buffer_) {
            offset target_offset = moffsets;
            // Out of Lock Execution: Perform the actual memory copy (De-deferred Copy).
            // The ingestion thread can now safely write to other regions of mdata in parallel.
            size_t packet_size = 0;
            if (target_offset.get_end() >= target_offset.get_begin() ) {
                // Scenario A: The packet data is linear and contiguous
                packet_size = (target_offset.get_end() - target_offset.get_begin()) + 1;
                output_buffer_.resize(packet_size);
                std::memcpy(output_buffer_.data(), (mdata_base+target_offset.get_begin()), packet_size);
            } else {
                // Scenario B: The packet data wrapped around the circular memory ceiling
                size_t first_part = MAX_NUM_BYTES - target_offset.get_begin();
                size_t second_part = target_offset.get_end() + 1;
                packet_size = first_part + second_part;
                
                output_buffer_.resize(packet_size);
                std::memcpy(output_buffer_.data(), (mdata_base+target_offset.get_begin()), first_part);
                std::memcpy(output_buffer_.data() + first_part, mdata_base, second_part);
            }
        }

        /**
         * @biref
         */
        uint8_t* mview1 = nullptr;

        /**
         * @biref
         */
        uint8_t* mview2 = nullptr;

        /**
         * @brief Flat contiguous memory block allocating on the heap.
         *          advanced for each packet
         */
        uint8_t* mdata = nullptr;

        /**
         * @brief Flat contiguous memory block allocating on the heap.
         *          base address
         */
        uint8_t* mdata_base = nullptr;

    #if defined _WIN64 || _WIN32
        HANDLE mh_map = INVALID_HANDLE_VALUE;
    #endif

        /**
         * @brief **THREAD-SAFE** Storing the offset information for the last packet packet.
         */
        offset moffsets;

        /**
         * @brief Mutex to synchronize status and operations on the very first packet.
         */
        std::mutex mmtx_packet_received;

        /**
         * @brief Condition Variable to notify the first packet is received.
         */
        std::condition_variable mcv_packet_received;

        /**
         * @brief Flag indicating whether the first packet has been received.
         */
        bool mpacket_received = false;

    }; // class ring_buffer


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
            #if defined _WIN64 || _WIN32
                msocket(INVALID_SOCKET) {
            #endif
            #if defined __linux__ || __unix__
                msocket(-1) {
            #endif
            mstatus.store(SERVER_STATUS::INIT);
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
         * @brief Virtual destructor that enforces a safe and orderly server shutdown.
         *
         * Automatically triggers the `stop()` sequence during object destruction. This guarantees 
         * that the active reception loop is unblocked, ongoing network operations are terminated, 
         * and all underlying system resources (such as socket descriptors) are fully released.
         * 
         * @note Declared `virtual` to ensure robust and safe resource cleanup even if the 
         *       `udp_server` component is extended through inheritance.
         */
        virtual ~udp_server() {
            stop();
        }

        /**
         * @brief Deleted copy assignment operator.
         *
         * Copying server states or resources via assignment is disabled to enforce 
         * strict resource ownership and avoid race conditions or double-close errors.
         */
        udp_server& operator=( const udp_server& ) = delete;

        /**
         * @brief Initializes the network subsystem and starts the primary UDP reception loop.
         *
         * @note This function is **BLOCKING**. It will block the calling thread indefinitely
         *       inside the internal `receive_from()` loop until the server is explicitly stopped.
         *
         * @throws udp_server_error If Winsock initialization fails, if the socket cannot be opened,
         *          or if binding to the local port fails.
         */
        void start() {
            // mstatus.exchange(SERVER_STATUS::STOP, std::memory_order_seq_cst); 
            SERVER_STATUS curr_status = mstatus.load(std::memory_order_relaxed); 
            // if ( (curr_status == SERVER_STATUS::INIT) || (curr_status == SERVER_STATUS::STOP ) ) {
            if ( (curr_status != SERVER_STATUS::START) && (curr_status != SERVER_STATUS::RUNNING) ) {
                mstatus.store(SERVER_STATUS::START);
                open_socket();
                bind_socket();
                receive_from();
            }
            else {
                throw udp_server_error( "Called \"udp_server::start()\" already start");
            }
        }

        /**
         * @brief Gracefully stops the UDP server by updating its status and unblocking the receive thread.
         */
        void stop() {
            SERVER_STATUS old_status = mstatus.exchange(SERVER_STATUS::STOP, std::memory_order_seq_cst);
            switch ( old_status ) {
                case ( SERVER_STATUS::STOP ) :
                case ( SERVER_STATUS::INIT ) : {
                    break;
                }
                case ( SERVER_STATUS::START ) :
                case ( SERVER_STATUS::RUNNING ) : {
                    send_dummy();
                    close_socket();
                    break;
                }
            }
            mstatus.store(SERVER_STATUS::STOP);
        }

        /**
         * @brief THIS FUNCTION IS **NON-BLOCKING**.
         *          Data Copy of the last packet received.
         * @param last_packet_ [out] The destination buffer where the packet payload will be copied.
         */
        void get_last_packet_async( std::vector< uint8_t >& last_packet_ ) {
            mring_buffer.get_last_packet_async( last_packet_ );
        }

        /**
         * @brief THIS FUNCTION IS **BLOCKING**.
         * @brief Data Copy of the last packet received
         * @param last_packet_ [out] The destination buffer where the packet payload will be copied.
         */
        void get_last_packet_sync( std::vector< uint8_t >& last_packet_ ) {
            mring_buffer.get_last_packet_sync( last_packet_ );
        }

        /**
         * @brief Set local port
         * @param _local_port [in] Local port
         */
        void set_local_port( unsigned short _local_port ) {
            mlocal_port = _local_port;
        }

        /**
         * @brief Get local port
         * @return Local Port
         */
        unsigned short get_local_port() {
            return mlocal_port;
        }

        /**
         * @brief Retrieves the current server status as an enum value.
         * @note This function is fully thread-safe and relies on relaxed memory ordering
         *       to guarantee zero performance overhead during high-frequency polling.
         * @return SERVER_STATUS The current active state of the server.
         */
        [[nodiscard]] SERVER_STATUS get_status_enum() const noexcept {
            return mstatus.load(std::memory_order_relaxed);
        }

        /**
         * @brief Retrieves the current server status as a string representation.
         * @note Thread-safe. Fetches the atomic state and converts it into a non-allocating
         *       string view wrapper, making it ideal for low-latency logging and debugging.
         * @return std::string_view A text view matching the status name ("INIT", "RUNNING", etc.).
         */
        [[nodiscard]] std::string_view get_status_str() const noexcept {
            // Fetch the atomic value into a local snapshot before evaluating the switch
            const SERVER_STATUS current = mstatus.load(std::memory_order_relaxed);
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
            int ret = ::WSAStartup(MAKEWORD(2, 2), &data);
            if (ret != EXIT_SUCCESS) {
                throw udp_server_error("Called \"udp_server::start_server() WSAStartup() != EXIT_SUCCESS\"");
            }
            msocket = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if ( msocket == INVALID_SOCKET ) {
                error_socket = true;
            }
        #endif

        #if defined __linux__ || __unix__
            msocket = ::socket(AF_INET, SOCK_DGRAM, 0);
            if ( msocket < 0 ) {
                error_socket = true;
            }
        #endif

            if ( error_socket ) {
                throw udp_server_error( "Exception \"udp_server::open_socket()\"");
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
         *          embedding the platform-specific error code and string message.
         */
        void bind_socket( ) {
            bool err_bind = false;

        #if defined _WIN64 || _WIN32
            sockaddr_in addr;
            ///< AF_INET = IPV4
            addr.sin_family = AF_INET;
            addr.sin_port = htons(mlocal_port);
            addr.sin_addr.s_addr = htonl(INADDR_ANY);
            if ( ::bind(msocket, (SOCKADDR*)(&addr), sizeof(addr)) != 0 ) {
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
           if ( ::bind(msocket, (struct sockaddr *) &addr, sizeof(addr)) < 0 ) {
               err_bind = true;
           }
        #endif

            if ( err_bind ) {
                throw udp_server_error("Exception \"udp_server::bind_socket()\" != EXIT_SUCCESS");
            }
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
            bool run = true;
            struct sockaddr_in client;

        #if defined _WIN64 || _WIN32
            int size_client = sizeof(client);
            int num_bytes_rx = 0; 
        #endif

        #if defined __linux__ || __unix__
            socklen_t size_client = sizeof(client);
            ssize_t num_bytes_rx = 0;
        #endif

            uint8_t* write_ptr = nullptr;
            mring_buffer.advance_write_pointer( 0, write_ptr ) ;
            mstatus.store(SERVER_STATUS::RUNNING);

            while ( run ) {
                std::memset(&client, 0, sizeof(client));

            #if defined __linux__ || __unix__
                ///< reset system error
                errno = 0;
            #endif

                num_bytes_rx = ::recvfrom(msocket,
                                        reinterpret_cast<char*>(write_ptr),
                                        MAX_SIZE_UDP_RX,
                                        0,
                                        reinterpret_cast<sockaddr*>(&client),
                                        &size_client);

                ///< Received dummy packet
                if ( mstatus.load(std::memory_order_relaxed) == SERVER_STATUS::STOP ) {
                    mring_buffer.notify_packet_received();
                    break;
                }

            #if defined _WIN64 || _WIN32
                if ( num_bytes_rx == SOCKET_ERROR ) {
                    run = false;
                }
            #endif

            #if defined __linux__ || __unix__
                if ( num_bytes_rx < 0 ) {
                    ///< Check if the failure was caused by an intentional server shutdown sequence
                    if ( errno == 9 || errno == 4 ) {
                        run = false;
                    }
                }
            #endif

                if ( num_bytes_rx >= 0 ) {
                    mring_buffer.advance_write_pointer( num_bytes_rx, write_ptr ) ;
                    mring_buffer.notify_packet_received();
                }
                else { // num_bytes_rx < 0
                    mring_buffer.notify_packet_received();
                    throw udp_server_error( "Called \"udp_server::receive_from() num byttes rx < 0\"");
                }

            } // while ( run )
        }

        /**
         * @brief Sends a zero-byte dummy datagram to the local port to unblock the server.
         *
         * This function creates a temporary, short-lived UDP socket and transmits an empty
         * payload to the server's own listening port via the local loopback interface (127.0.0.1).
         * It acts as a wake-up signal to force the blocking `recvfrom()` call inside the
         * ingestion loop to return immediately, allowing the thread to evaluate shutdown flags.
         *
         * @note Cross-platform implementation handling Winsock (Windows) and POSIX (Linux/Unix)
         *       socket lifecycles natively. It cleans up its internal resources before returning.
         */
        void send_dummy() {
        #if defined(_WIN64) || defined(_WIN32)
            SOCKET wake_sock = ::socket(AF_INET, SOCK_DGRAM, 0);
            if (wake_sock != INVALID_SOCKET) {
        #elif defined(__linux__) || defined(__unix__)
            int wake_sock = ::socket(AF_INET, SOCK_DGRAM, 0);
            if (wake_sock >= 0) {
        #endif
                char dummy_buffer = 0;
                sockaddr_in loopback_addr{};
                loopback_addr.sin_family = AF_INET;
                loopback_addr.sin_port = htons(mlocal_port);
                ::inet_pton(AF_INET, "127.0.0.1", &loopback_addr.sin_addr);

                // Transmit 0 bytes payload. This safely wakes up recvfrom() without carrying data.
                //sendto(wake_sock, nullptr, 0, 0, reinterpret_cast<struct sockaddr*>(&loopback_addr), sizeof(loopback_addr));
                ::sendto(wake_sock, &dummy_buffer, 0, 0, reinterpret_cast<struct sockaddr*>(&loopback_addr), sizeof(loopback_addr));

            #if defined(_WIN64) || defined(_WIN32)
                ::closesocket(wake_sock);
            #elif defined(__linux__) || defined(__unix__)
                ::close(wake_sock);
            #endif
            }
        }

        /**
         * @brief Closes the main server socket interface and releases OS network resources.
         *
         * Safely terminates the operational socket descriptor and resets the internal identifier
         * (`INVALID_SOCKET` on Windows or `-1` on Linux) to prevent double-close attempts or dangling
         * resource references.
         *
         * @note If the socket system call fails during the shutdown phase, it logs the operating
         *       system's diagnostic state and propagates a structured runtime error.
         *
         * @throws udp_server_error Thrown if the operating system fails to gracefully release
         *                          the allocated socket interface descriptor.
         */
        void close_socket() {
            bool err_close = false;
            #if defined(_WIN64) || defined(_WIN32)
                ///< On Windows, msocket might be checked against INVALID_SOCKET
                if ( msocket != INVALID_SOCKET ) {
                    if (::closesocket(msocket) == SOCKET_ERROR) {
                        err_close = true;
                    }
                    else {
                        msocket = INVALID_SOCKET;
                        WSACleanup(); 
                    }
                }
            #endif

            #if defined(__linux__) || defined(__unix__)
                if ( msocket != -1 ) {
                    if (::close(msocket) != 0) {
                        err_close = true;
                    }
                    msocket = -1;
                }
            #endif

            if (err_close) {
                throw udp_server_error("Exception \"udp_server::stop()\"");
            }
        }

        /**
         * @brief Local Port.
         */
        unsigned short mlocal_port;

        /**
         * @brief Mutex of ring buffer.
         */
        ring_buffer mring_buffer;

        /**
         * @brief Socket File Descriptor.
         */
    #if defined _WIN64 || _WIN32
        SOCKET msocket;
    #endif

    #if defined __linux__ || __unix__
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
        std::atomic<SERVER_STATUS> mstatus{ SERVER_STATUS::INIT };

    }; // class udp_server

} // namespace ns_udp_server

#endif
