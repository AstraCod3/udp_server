#ifndef UNITTESTUDPSERVER
#define UNITTESTUDPSERVER

#include <chrono> // sleep

#include "../UdpServer/BaseUdpServer.h"
//#include "../UdpServer/BaseThread.h"

namespace NsUnitTest {
	void PrintBuffHex(unsigned char* _b, size_t _l) {
		for (int i = 0; i < _l; i++)
			std::cout << "Buffer[" << i << "]" << std::hex << *(_b + i) << std::endl;
	}

	void PrintBuffChar(unsigned char* _b, size_t _l) {
		for (int i = 0; i < _l; i++)
			std::cout << "Buffer[" << i << "]" << static_cast<char>(*(_b + i)) << std::endl;
	}

	void PrintBuffString(unsigned char* _b, size_t _l) {
		std::string str;
		for (int i = 0; i < _l; i++)
			str += _b[i];
		std::cout << "Buffer : " << str << std::endl;
	}

	std::string GetString(unsigned char* _b, size_t _l) {
		std::string str;
		for (int i = 0; i < _l; i++)
			str += _b[i];
		return str;
	}



	class TBaseUdpServerUdp : public NsUdpServer::BaseUdpServer {

	public:
		TBaseUdpServerUdp() { }
		TBaseUdpServerUdp(unsigned short& _ulp) : BaseUdpServer(_ulp) { }
		virtual ~TBaseUdpServerUdp() { }
		void TGetErr(int& ret, std::string& err_msg) { GetErr(ret, err_msg); }
		void TGetDataRecv(unsigned char* data_recv, size_t& len) { GetDataRecv(data_recv, len); }
	};

	int TestStartAndRx(void) {
		int ret = EXIT_SUCCESS;
		std::string err_msg;
		unsigned short ulport = 9084;
		TBaseUdpServerUdp obaseUdp(ulport);
		std::cout << "set local port : " << ulport << "\n";
		std::cout << "call BaseStartServer() ret -> ";
		ret = obaseUdp.BaseStartServer();
		if (ret != EXIT_SUCCESS) {
			obaseUdp.TGetErr(ret, err_msg);
			std::cout << "err : " << ret << " - err_msg " << err_msg << "\n";
			return EXIT_FAILURE;
		}
		std::cout << ret << "\n";
		unsigned char* data_recv = nullptr;
		data_recv = new (std::nothrow) unsigned char[MAXSIZE_UDP_RX];

		bool run = true;
		while (run) {
			size_t len = 0;
			std::cout << "wait data receive ...\n";
			ret = obaseUdp.BaseStartReceivedFrom();
			if (ret < 0) {
				obaseUdp.TGetErr(ret, err_msg);
				std::cout << "err : " << ret << "\nerr_msg : " << err_msg << "\n";
				break;
			}
			std::memset((unsigned char*)data_recv, 0, MAXSIZE_UDP_RX);
			obaseUdp.TGetDataRecv(data_recv, len);
			std::cout << "\n lenght : " << len << "\n";

			if (len > 0) {
				std::cout << "\n data received ...\n";
				//udpSrv.PrintBuffHex(data_recv, len);
				PrintBuffChar(data_recv, len);
				PrintBuffString(data_recv, len);
				std::string exit_str = GetString(data_recv, len);
				if (exit_str == "quit\n") {
					std::cout << "\n received quit \n";
					run = false;
				}
			}
			else {
				std::cout << "\n buffer empty ...\n";
				run = false;
			}


			// std::cout << "data_recv : " << std::string((char*)(data_recv) )<< "\n";
			std::cout << "\n";
//			std::cout << "sin_family : " << obaseUdp.GetInfoClient().sin_family << "\n";
//			std::cout << "sin_port : " << obaseUdp.GetInfoClient().sin_port << "\n";
//			//std::cout << "sin_addr : " << inet_ntoa (udpSrv.GetInfoClient().sin_addr) << "\n";
//			std::cout << "sin_zero : " << obaseUdp.GetInfoClient().sin_zero << "\n";
//			std::cout << "\n";
		}

		ret = obaseUdp.BaseStopServer();
		std::cout << "\n ret : " << ret << "\n";

		std::cout << "\n deleting buffer \n";
		delete[]data_recv;
		data_recv = nullptr;
		std::cout << "\n ...done!\n";
		return ret;
	}
	


/*	using namespace std::chrono_literals;
	
	enum command_thread {
		continue_thread = 10,
		//stop_thread = 11,
		exit_thread = 11
	};

	class TBaseThread : public NsThread::BaseThread {
	public:
		TBaseThread() : 
			counter(0),
			stop_thread(10) { }
		virtual ~TBaseThread() { }
		int TestCreateThread(const char* _thread_name) {
			int ret = EXIT_SUCCESS;
			ret = CreateThread(_thread_name);
			return ret;
		}

		void PrintStatusThread() {
			switch ( GetThreadStatus() ) {
				case NsThread::THREAD_STATUS::INIT : std::cout << "Thread status INIT\n"; break;
				case NsThread::THREAD_STATUS::CREATE : std::cout << "Thread status CREATE\n"; break;
				case NsThread::THREAD_STATUS::WAIT : std::cout << "Thread status WAIT\n"; break;
				case NsThread::THREAD_STATUS::RUN :	 std::cout << "Thread status RUN\n"; break;
				case NsThread::THREAD_STATUS::EXIT : std::cout << "Thread status EXIT\n"; break;
				default: std::cout << "Thread status NOT defined\n"; break;
			}
		}

	private:
		int counter;
		int stop_thread;
	protected:
		void thread_func() override {
			std::cout << "thread func\n" << std::flush;
			//const auto start = std::chrono::high_resolution_clock::now();
			std::this_thread::sleep_for(1000ms);
			//const auto end = std::chrono::high_resolution_clock::now();
			//const std::chrono::duration<double, std::milli> elapsed = end - start;

			std::cout << "counter : " << counter << std::endl;
			switch (counter) {
			case command_thread::continue_thread :
				std::cout << "continue thread\n" << std::endl;
				counter++;
				RunThread();
				break;
			//case command_thread::stop_thread:
			//	std::cout << "stop thread\n" << std::endl;
			//	break;
			case command_thread::exit_thread:
				std::cout << "exit thread\n" << std::endl;
				ExitThread();
				break;
			default:
				break;
			}
			RunThread();
		}
	};
*/
} /* namespace nsUnitTestUdp */

#endif