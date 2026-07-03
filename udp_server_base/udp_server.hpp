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

#ifndef BASEUDPSERVER_HEADER
#define BASEUDPSERVER_HEADER

#include <iostream>
#include <string>
#include <array>
#include <vector>
#include <cstdlib>
#include <sstream>
#include <mutex>

#if defined _WIN64 || _WIN32
#include <winsock2.h>   // Socket,Bind,Recv,etc
#include <Winbase.h>    // FormatMessage
#pragma comment (lib, "WS2_32.lib")
#endif

#if defined __linux__ || __unix__
#endif


/**
 * @namespace ns_udp_server 
 * @brief Gather all class, struct, enum etc to handling thread
 */
namespace ns_udp_server {


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
         * @param message The detailed error message.
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
     * @struct udp_packet
     * @brief Monolithic single-buffer circular ring engine for high-speed UDP ingestion.
     */
    struct packet {

        public :

        /**
         * @brief Constructor that pre-allocates size tracking memory to prevent runtime allocations.
         */
        packet() {
            // Pre-allocate the vector size immediately to avoid reallocation data races with the main thread
            // packet_size.resize(max_packets, 0);
        }

        /**
         * @brief Commits the newly received packet size into history and advances the internal circular cursor.
         * @param bytes_len The exact number of bytes returned by the recvfrom call.
         */
        void commit_packet(std::size_t bytes_len) {
            offset last_offset;
            std::unique_lock<std::mutex> lck( mmtx_offset );
            last_offset = packet_offsets.back();   
            
            last_offset.begin = last_offset.end + 1;
            if ( last_offset.begin > max_bytes )
                last_offset.begin = last_offset.begin % max_bytes ;  
            
            last_offset.end = last_offset.begin + bytes_len;
            if ( last_offset.end > max_bytes )
                last_offset.end = last_offset.end % max_bytes ;  

            packet_offsets.push_back( last_offset ); 
            lck.unlock();
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
         * @brief
         */
        std::size_t get_offset_last_packet( ) {
            std::unique_lock<std::mutex> lck( mmtx_offset );
            offset last_offset = packet_offsets.back();   
            lck.unlock();
            return last_offset.end;
        }

        private : 
        
        static constexpr std::size_t max_size_udp_rx = 65535; ///< Max theoretical UDP payload size.
        static constexpr std::size_t max_packets = 10; ///< Total slots available in the ring buffer.
        // static constexpr std::size_t max_bytes = max_size_udp_rx * max_packets;
        static constexpr std::size_t max_bytes = max_size_udp_rx ;
        
        std::mutex mmtx_data;
        std::array< uint8_t, 100 > data; ///< Flat contiguous memory block allocating ~655 Megabytes on the heap.

        std::mutex mmtx_offset; ///< Chronological history tracking the real size of each received packet. 

        /**
         * @brief Chronological history tracking the real size of each received packet.
         */
        struct offset {
            offset() : begin(0),end(0) { }
            virtual ~offset() { }
            std::size_t begin = 0;  ///< Circular cursor index for the next write operation (0 to 9999).
            std::size_t end = 0;   ///< Monotonically increasing counter of all packets received.
        };

        std::vector< offset > packet_offsets; ///< Chronological history tracking the real size of each received packet. 

    };


    /**
     * @class udp_server_base 
     * @brief Constructor for udp_server_base
     */
	class udp_server_base {

        public:
 
        /**
         * @brief Constructor for udp_server_base
         * @param _lport local port
         * @throw std::runtime_error If the thread was not created
         */
		udp_server_base( unsigned short& _lport ) :
                mlocal_port(_lport),
                merror_code_num(0),
                merror_code_str(""),
                msocket(0) {
            if ( _lport > 65535 ) {
                mlocal_port = 0 ;
                throw udp_server_error( "Called \"uudp_server_base::dp_server_base()\" local port : " + std::to_string( _port ) + " > 65535");
            }
        }

        /**
         * @brief Destructor for udp_server_base
         */
		virtual ~udp_server_base( ) { }

        /**
         * @brief
         */
		virtual receive_base( ) = 0;

        /**
         * @brief This function is BLOCKING
         * @throw udp_server_error 
         */
        void start_server( ) {
        #if defined _WIN64 || _WIN32
            WSAData data;
            ret = WSAStartup(MAKEWORD(2, 2), &data);
            if (ret != EXIT_SUCCESS) {
                set_err_sys();
                throw udp_server_error("Called \"udp_server_base::start_server() WSAStartup() != EXIT_SUCCESS\"", get_error_code_num(), get_error_code_str() );
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
            if ( close_socket() != EXIT_SUCCESS ) {
                set_err_sys();
                throw udp_server_error ("Called \"udp_server_base::stop() close_socket() != EXIT_SUCCESS\"", get_error_code_num(), get_error_code_str() );
            }
        #if defined _WIN64 || _WIN32
            if ( WSACleanup() != EXIT_SUCCESS) {
                set_err_sys();
                throw udp_server_error ("Called \"udp_server_base::stop() WSACleanup() != EXIT_SUCCESS\"", get_error_code_num(), get_error_code_str() );
            }
        #endif
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
        packet mpackets;

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
            msocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if ( msocket == INVALID_SOCKET ) {
                set_err_sys();
                throw udp_server_error( "Exception \"udp_server::open_socket()\"", get_err_code(), get_err_msg() );
            }
        }

        /**
         * @brief
         * @throw udp_server_error Throw
         */
		void bind_socket( ) {
            sockaddr_in add;
            add.sin_family = AF_INET;
            add.sin_addr.s_addr = htonl(INADDR_ANY);
            add.sin_port = htons(mlocal_port);
            if ( bind(msocket, (SOCKADDR*)(&add), sizeof(add) != EXIT_SUCCESS ) {
                set_err_sys();
                throw udp_server_error("Exception \"udp_server::bind_socket()\" != EXIT_SUCCESS", get_err_code(), get_err_msg() );
            }
        }

        /**
         * @brief
         */
		int close_socket( ) {
            int ret = EXIT_SUCCESS;
            std::unique_lock<std::mutex> lck(mmtx_data);
            ret = closesocket(msocket);
            lck.unlock();
            return ret;
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
        void get_err( int& err_code_, std::string& err_msg_ ) {
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
            lck.unlock();
        #endif
        }

        /**
         * @brief THIS Function is BLOCKING
         */
        void receive_from() {
            /**
             * @brief 
             */
            static struct sockaddr_in msclient;
            static const int size_client = sizeof(msclient);

            bool run = true;
            
            while ( run ) {

                std::memset(&msclient, 0, sizeof(msclient));
		
                uint8_t* write_ptr = mpackets.get_offset_last_packet() + 1;

                std::unique_lock<std::mutex> lck(mmtx_data);

            #if defined _WIN64 || _WIN32
                int num_bytes_rx = recvfrom(msocket,
                                (char*)write_ptr,
                                MAXSIZE_UDP_RX,
                                0,
                                reinterpret_cast<SOCKADDR*>(&msclient), &size_client );
                                //(SOCKADDR*)(&msclient), &size);

                // ret : 0 = close socket
                if ( bytes_rx == SOCKET_ERROR || bytes_rx < 0 ) {
                    set_err_sys();
                    throw udp_server_error( "Called \"udp_server_base::receive_from() SOCKET_ERROR\"", get_error_code_num(), get_error_code_str() );
                }
            #endif

                if ( num_bytes_rx >= 0 ) {
                    mpackets.commit_
                } 

                lck.unlock();
            }
        }

    }; /* class udp_server_base */

} /* namespace ns_udp_server */

#endif
