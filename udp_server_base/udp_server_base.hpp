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

#define MAXSIZE_UDP_RX 65535

/**
 * @namespace ns_thread_base
 * @brief Gather all class, struct, enum etc to handling thread
 */
namespace ns_udp_server_base {

    /**
     * @class udp_server_error 
     * @brief Custom exception thrown when a operation is failed.
     * @details Inherits from std::runtime_error to maintain compatibility with standard 
     *          exception handling while preventing exception pollution.
     */
    class udp_server_error : public std::runtime_error {
        public:

        /**
         * @brief Constructor for udp_server_error .
         * @param message The detailed error message.
         */
        explicit udp_server_error( const std::string& message )
            : std::runtime_error(message) { }

        /**
         * @brief Constructor for udp_server_error .
         * @param _msg
         * @param _err_code_num
         * @param _err_code_str
         */
        explicit udp_server_error(  const std::string& _msg,
                                    const int& _err_code_num,
                                    const std::string& _err_code_str ) {
            std::string execpetion_msg = _msg + 
                                "\nError Code Num : " + std::to_string( _err_code_num ) +
                                "\nError Code Str : " + _err_code_str ;
            std::runtime_error( execpetion_msg );
        }
    };


    /**
     * @struct udp_packet
     * @brief 
     */
    struct udp_packet {

        std::vector<uint8_t> data;
        
        /**
         * @brief Indica quanti byte sono stati EFFETTIVAMENTE ricevuti nell'ultimo pacchetto
         */
        std::size_t valid_size = 0;

        /**
         * @brief Costruttore che inizializza il buffer alla dimensione massima
         */
        udp_packet() :
            data(65535, 0), size(0) { }

        /**
         * @brief
         */
        udp_packet(std::vector<uint8_t>&& _buffer, size_t _bytes_len) :
            data( std::move(buffer) ), size( _bytes_len ) { }
    };


    /**
     * @class udp_server_base 
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
                musize_data_rx(0),
                merror_code_num(0),
                mSocket(0) {
            if ( _lport > 65535 ) {
                throw udp_server_error ("Called \"udp_server_base()\" local port > 65535");
            }

            std::memset(mdata_rx, 0, MAXSIZE_UDP_RX);
            mdata_rx[0] = '\0';

            musize_data_rx = 0;

            std::memset(&msclient, 0, sizeof(msclient));
        }

        /**
         * @brief Destructor for udp_server_base
         */
		virtual ~udp_server_base( );

        /**
         * @brief
         * @throw 
         */
        int start_server( ) {
        #if defined _WIN64 || _WIN32
            WSAData data;
            ret = WSAStartup(MAKEWORD(2, 2), &data);
            if (ret != EXIT_SUCCESS) {
                set_err_sys();
                throw udp_server_error ("Called \"udp_server_base::start_server() WSAStartup() != EXIT_SUCCESS\"", get_error_code_num(), get_error_code_str() );
            }
        #endif
            open_socket();
            bind_socket();
        }

        /**
         * @brief
         */
		void stop() {
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
         * @brief THIS Function is BLOCKING
         */
        void receive_from() {
        #if defined _WIN64 || _WIN32
            int ret = EXIT_SUCCESS;
            std::unique_lock<std::mutex> lck(mmtx_data);
            int size = sizeof(msclient);

            ret = recvfrom(mSocket,
                (char*)(&mdata_rx[0]),
                MAXSIZE_UDP_RX,
                0,
                reinterpret_cast<SOCKADDR*>(&msclient), &size);
                //(SOCKADDR*)(&msclient), &size);

            // ret : 0 = close socket
            if (ret > 0)
                musize_data_rx = ret;

            if (ret == 0) {
                set_err_sys();
                lck.unlock();
                return merror_code_num;
            }

            lck.unlock();
        #endif
        }

        /**
         * @brief Get the size of the dara received
         * @return The byte of received data
         */
		size_t get_rx_data_size ( ) {
            size_t ret_size;
            std::unique_lock<std::mutex> lck(mmtx_data);
            ret_size = musize_data_rx;
            lck.unlock();
            return ret_size;
        }

        /**
         * @brief 
         */
        sockaddr_in get_info_client() {
            sockaddr_in ret;
            std::unique_lock<std::mutex> lck(mmtx_data);
            ret = msclient;
            lck.unlock();
            return ret;
        }

        /**
         * @brief
         */
		void set_local_port(unsigned short _local_port) {
            mlocal_port = _local_port;
        }

        /**
         * @brief
         */
		unsigned short get_local_port() { return mlocal_port; }


        protected:

        /**
         * @brief
         */
		void PrintBuffHex(unsigned char* _b, size_t _l);

        /**
         * @brief
         */
		void PrintBuffString(unsigned char* _b, size_t _l);

        /**
         * @brief
         */
		void PrintBuffChar(unsigned char* _b, size_t _l);

        /**
         * @brief
         */
		void GetErr(int& _err_code, std::string& err_msg_);

        /**
         * @brief
         */
		std::string GetErrMsg();


        private:

        /**
         * @brief
         */
		unsigned short mlocal_port;

        /**
         * @brief
         */
		std::mutex mmtx_data;

        /**
         * @brief 
         */
        std::vector< udp_packet> mdata_rx;

        /**
         * @brief 
         */
		size_t musize_data_rx;

        /**
         * @brief 
         */
		struct sockaddr_in msclient;
		
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

    #if defined _WIN64 || _WIN32
		//WSAData mWSAData;

        /**
         * @brief 
         */
		SOCKET mSocket;
    #endif

    #if defined __linux__ || __unix__
        /**
         * @brief 
         */
		int mSocket;
    #endif

        /**
         * @brief
         * @throw udp_server_error Throw if the socket is NOT VALID
         */
        void open_socket( ) {
            mSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
            if ( mSocket == INVALID_SOCKET ) {
                set_err_sys();
                throw udp_server_error ("Called \"open_socket()\"
                        \nError Code : " + std::to_string( get_err_code() ) +
                        "\nError Str : " + get_err_msg() );
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
            if ( bind(mSocket, (SOCKADDR*)(&add), sizeof(add) != EXIT_SUCCESS );
                set_err_sys();
                throw udp_server_error( "Called \"open_socket()\"
                        \nError Code : " + std::to_string( get_err_code() ) +
                        "\nError Str : " + get_err_msg() );
            }
        }

        /**
         * @brief
         */
		int close_socket( ) {
            int ret = EXIT_SUCCESS;
            std::unique_lock<std::mutex> lck(mmtx_data);
            ret = closesocket(mSocket);
            lck.unlock();
            return ret;
        }

        /**
         * @brief 
         */
        void GetDataRecv( unsigned char* data_recv_, size_t& len_ ) {
            len_ = 0;
            std::unique_lock<std::mutex> lck(mmtx_data);
            if (mdata_rx[0] != '\0') {
                len_ = musize_data_rx;
                std::memset(data_recv_, 0, len_);
                std::memcpy(data_recv_, mdata_rx, len_);
                // clean buffer > lenght
                if ( len_ < (MAXSIZE_UDP_RX-1) )
                    data_recv_[len_ + 1] = '\0';
            }
            lck.unlock();
        }

        /**
         * @brief 
         */
        void set_error_str( const char* _err_msg ) {
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

} /* namespace nsBaseUdpServer */

#endif
