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
 * 
 * #if defined(__linux__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) 
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
//#include <winsock.h>
#include <winsock2.h>   // Socket,Bind,Recv,etc
#include <Winbase.h>    // FormatMessage
#include <Ws2tcpip.h>
#pragma comment (lib, "WS2_32.lib")
#endif

#if defined __linux__ || __unix__
#include <sys/socket.h> // socket(), bind(), recvfrom(), sockaddr
#include <netinet/in.h> // sockaddr_in, INADDR_ANY, htons()
#include <arpa/inet.h>  // inet_addr(), inet_ntoa()
#include <unistd.h>     // close() 
#include <cerrno>       // errno
// #include <sys/un.h>
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
         * @brief Constructor for udp_server_error.
         * @param _msg
         * @param _err_code_num
         * @param _err_code_str
         */
        explicit udp_server_error( const std::string& _msg,
                                    const int& _err_code_num,
                                    const std::string& _err_code_str ) :
            std::runtime_error( _msg + "\nError Code Num : " + std::to_string( _err_code_num ) + 
                            "\nError Code Str : " + _err_code_str ) { }
    };


    /**
     * @struct ring_buffer
     * @brief Monolithic single-buffer circular ring engine for high-speed UDP ingestion.
     */
    struct ring_buffer {

        public :

        /**
         * @brief Costructor ring_buffer
         * @throw
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
         * @brief Commits the newly received packet size into history and advances the internal circular cursor.
         * @param bytes_len The exact number of bytes returned by the recvfrom call.
         */
        void commit_packet( const std::size_t& bytes_len ) {
            std::unique_lock<std::mutex> lck( mmtx_offsets );
            offset tmp_offset;
            ///< First packet
            if ( mvoffsetss.empty() ) {
                // begin = 0
                tmp_offset.end = bytes_len - 1;
                mvoffsetss.push_back( tmp_offset ); 
                mcv_first_packet.notify_one();
            }
            else {
            ///< Other packets
                offset last_offset = mvoffsetss.back();
                tmp_offset.begin = last_offset.end + 1;
                if ( tmp_offset.begin >= max_num_bytes )
                    tmp_offset.begin = tmp_offset.begin % max_num_bytes;  
                tmp_offset.end = tmp_offset.begin + bytes_len - 1;
                if ( tmp_offset.end >= max_num_bytes )
                    tmp_offset.end = tmp_offset.end % max_num_bytes;
                mvoffsetss.push_back( tmp_offset ); 
            }
        }

        /**
         * @brief Calculates and returns the advanced memory address where the WinAPI should write the next incoming packet.
         * @return uint8_t* Pointer to the exact memory block offset inside the static array.
         */
       //  uint8_t get_data_address( ) {
       //      std::unique_lock<std::mutex> lck( mmtx_data );
       //      //std::size_t memory_offset = write_slot * max_size_udp_rx;
       //      //lck.unlock();
       //      //return &data[memory_offset];
       //  }

        /**
         * @brief Get offset of rung buffer
         */
        uint8_t* get_next_offset( ) {
            offset next_offset;
            std::unique_lock<std::mutex> lck( mmtx_offsets );
            if ( !mvoffsetss.empty() ) {
                next_offset = mvoffsetss.back();   
                next_offset.end = next_offset.end + 1; 
            }
            lck.unlock();

            if ( next_offset.end >= max_num_bytes )
                next_offset.end = next_offset.end % max_num_bytes;

            return &((*mdata)[next_offset.end]);
        }

        /**
         * @brief Data Copy of the last packet received
         * @param last_packet_ [out] The destination buffer where the packet payload will be copied. 
         */
        void get_last_packet( std::vector< uint8_t >& last_packet_ ) {
            waiting_first_packet_received();
            offset last_offset;
            std::unique_lock<std::mutex> lck( mmtx_offsets );
            last_offset = mvoffsetss.back();   
            lck.unlock();
            double dsize = std::abs(static_cast<double>(last_offset.end-last_offset.begin));
            last_packet_.reserve( std::abs( static_cast<int>(dsize)) );
            auto src_begin = mdata->begin() + last_offset.begin;
            auto src_end = mdata->begin() + last_offset.end;
            std::copy(src_begin, src_end, last_packet_.begin());
        }

        /**
         * @brief Set the flag and notify waiting threads when the first packet is received 
         */
        void set_first_packet_received() {
            std::unique_lock<std::mutex> lck_fp( mmtx_first_packet );
            if ( !mfirst_packet ) {
                mfirst_packet = true;
                mcv_first_packet.notify_one(); 
            }
        }

        /**
         * @brief This function IS BLOCKING, until the first packet is received
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
         * @brief Mutex for mdata
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
         * @brief Mutex to serialize access to the packet offsets vector
         */
        std::mutex mmtx_offsets;

        /**
         * @brief Vector storing the offset information for each buffered packet
         */
        std::vector< offset > mvoffsetss;

        /**
         * @brief Mutex to synchronize status and operations on the very first packet 
         */
        std::mutex mmtx_first_packet;

        /**
         * @brief Condition Variable to notify the first packet is received
         */
        std::condition_variable mcv_first_packet;

        /**
         * @brief Flag indicating whether the first packet has been received
         */
        bool mfirst_packet = false;

    };


    /**
     * @class udp_server 
     * @brief Constructor for udp_server
     */
    class udp_server {

        public:

        /**
         * @brief Constructor for udp_server
         * @param _lport local port
         * @throw std::runtime_error If the thread was not created
         */
        udp_server( const unsigned short& _lport ) :
                mlocal_port(_lport),
                merror_code_num(0),
                merror_code_str(""),
                msocket(0) {
            /*if ( _lport > 65535 ) {
                mlocal_port = 0 ;
                throw udp_server_error( "Called \"uudp_server::dp_server_base()\" local port : " + std::to_string( _port ) + " > 65535");
            }*/
        }

        /**
         * @brief Destructor for udp_server
         */
        virtual ~udp_server( ) { }

        /**
         * @brief This function is BLOCKING
         * @throw udp_server_error 
         */
        void start( ) {
        #if defined _WIN64 || _WIN32
            WSAData data;
            ret = WSAStartup(MAKEWORD(2, 2), &data);
            if (ret != EXIT_SUCCESS) {
                set_err_sys();
                throw udp_server_error("Called \"udp_server::start_server() WSAStartup() != EXIT_SUCCESS\"", get_error_code_num(), get_error_code_str() );
            }
        #endif
            open_socket();
            bind_socket();
            receive_from();
        }

        /**
         * @brief
         */
        void stop( ) {
            close_socket();
        }
		
        /**
         * @brief 
         */
        /*sockaddr_in get_info_client() {
            sockaddr_in ret;
            std::unique_lock<std::mutex> lck(mmtx_data);
            ret = msclient;
            lck.unlock();
            return ret;
        }*/

        /**
         * @brief
         */
        void set_local_port( unsigned short _local_port ) { mlocal_port = _local_port; }

        /**
         * @brief
         */
        unsigned short get_local_port() { return mlocal_port; }


        void get_last_packet( std::vector< uint8_t >& last_packet_ ) {
            mring_buffer.get_last_packet( last_packet_ );
        }

        protected:

        /**
         * @brief
         */
        unsigned short mlocal_port;

        /**
         * @brief 
         */
        std::mutex mmtx_error;

        /**
         * @brief 
         */
        int merror_code_num;

        /**
         * @brief 
         */
        std::string merror_code_str;

        /**
         * @brief 
         */
        std::mutex mmtx_data;

        /**
         * @brief 
         */
        ring_buffer mring_buffer;

    #if defined _WIN64 || _WIN32
        /**
         * @brief 
         */
        SOCKET msocket;
    #endif

    #if defined __linux__ || __unix__
        /**
         * @brief 
         */
        int msocket;
    #endif

        /**
         * @brief
         * @throw udp_server_error Throw if the socket is NOT VALID
         */
        void open_socket( ) {
            bool error_socket = false;
        #if defined _WIN64 || _WIN32
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
         * @brief
         * @throw udp_server_error Throw
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
         * @brief
         */
		void close_socket( ) {
            bool err_close = false;
            std::unique_lock<std::mutex> lck(mmtx_data);

        #if defined _WIN64 || _WIN32
            closesocket(msocket);
            if ( WSACleanup() != EXIT_SUCCESS) {
                set_err_sys();
                throw udp_server_error ("Called \"udp_server::close_socket() WSACleanup() != EXIT_SUCCESS\"", get_error_code_num(), get_error_code_str() );
            }
        #endif

        #if defined __linux__ || __unix__
            if ( close(msocket) != 0 ) {
                err_close = true;
            }
        #endif

            if ( err_close ) {
                set_err_sys();
                throw udp_server_error ("Called \"udp_server::close_socket()\"", get_error_code_num(), get_error_code_str() );
            }

            lck.unlock();
        }

        /**
         * @brief 
         */
        void set_error_code_str( const char* _err_msg ) {
            std::unique_lock<std::mutex> lck(mmtx_error);
            merror_code_str = _err_msg;
            lck.unlock();
        }

        /**
         * @brief 
         */
        void get_error( int& err_code_, std::string& err_msg_ ) {
            std::unique_lock<std::mutex> lck(mmtx_error);
            err_code_ = merror_code_num;
            err_msg_ = merror_code_str;
            lck.unlock();
        }

        /**
         * @brief 
         */
        int get_error_code_num ( void ) {
            int ret = EXIT_SUCCESS;
            std::unique_lock<std::mutex> lck(mmtx_error);
            ret = merror_code_num;
            lck.unlock();
            return ret;
        }

        /**
         * @brief 
         */
        std::string get_error_code_str ( ) {
            std::string ret;
            std::unique_lock<std::mutex> lck(mmtx_error);
            ret = merror_code_str;
            lck.unlock();
            return ret;
        }

        /**
         * @brief 
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
         * @brief THIS Function is BLOCKING
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

            // std::unique_lock<std::mutex> lck(mmtx_data);
            uint8_t* write_ptr = mring_buffer.get_next_offset();
            // lck.unlock();

            // Opzione B: std::vector (dinamico)
            //unsigned char buffer[max_size_udp_rx];
            //unsigned char* write_ptr = &buffer[0];

            while ( run ) {
                std::memset(&client, 0, sizeof(client));

            #if defined _WIN64 || _WIN32
                int num_bytes_rx = recvfrom(msocket,
                                    (char*)write_ptr,
                                    MAXSIZE_UDP_RX,
                                    0,
                                    reinterpret_cast<SOCKADDR*>(&msclient), &size_client );
                                    // (SOCKADDR*)(&msclient), &size);

                // ret : 0 = close socket
                if ( num_bytes_rx == SOCKET_ERROR || num_bytes_rx < 0 ) {
                    set_err_sys();
                    throw udp_server_error( "Called \"udp_server::receive_from() SOCKET_ERROR\"", get_error_code_num(), get_error_code_str() );
                    break;
                }
            #endif

            #if defined __linux__ || __unix__
                size_client = sizeof(client);
                errno = 0;
                // std::unique_lock<std::mutex> lck(mmtx_data);
                ssize_t num_bytes_rx = recvfrom(msocket,
                                    (unsigned char*)write_ptr,
                                    max_size_udp_rx,
                                    0,
                                    //reinterpret_cast<sockaddr*>(&client), 
                                    (sockaddr*)&client, 
                                    &size_client );

                if ( num_bytes_rx < 0 ) {
                    set_err_sys();
                    throw udp_server_error( "Called \"udp_server::recvfrom() < 0\"", get_error_code_num(), get_error_code_str() );
                    break;
                }
            #endif

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
