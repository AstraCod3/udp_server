/**
 * @file failure.cpp
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
#include <iostream>
#include <sstream>
#include <mutex>
#include <thread>
#include <chrono>
#include <vector>
#include <memory>
#include <algorithm> // std::sort
#include <utility> // std::swap

#include "../thread_base/cthread_base.hpp"


namespace ns_failure {
 
     /**
      * @class cfailure
      * @brief Example illustrating how to use ns_base_thread::cthread_base
      */
    class cfailure : public ns_thread_base::cthread_base {
 
    public:
 
        /**
        * @brief cfailure Costructor
        */
        cfailure () { }
        cfailure ( std::string& _name_of_thread ) :
        ns_thread_base::cthread_base(_name_of_thread )
        { }

        /**
        * @brief cfailure Destructor
        */
        virtual ~cfailure () { }
 
    protected:
 
        /**
         * @brief Example of the implementation of "thread_function()" inherited from ns_base_thread::cthread_base
         */
        void thread_function() override {
            std::cout << "Executing thread 10 seconds\n";
            for (int i=10; i>0; --i) {
                std::cout << " ... " << i;
                std::this_thread::sleep_for (std::chrono::seconds(1));
            }
            std::cout << std::endl;
         }
 
     };


    /**
     * @class cmanager_failure  
     * @brief 
     */
    class cmanager_failure {

    public:

        /**
         * @brief 
         */
        cmanager_failure()
        { }
        
        /**
         * @brief 
         */
        virtual ~cmanager_failure ()
        { }

        /**
         * @brief Executes a sequence of commands from the class cthread_base
         * @param ofthr [IN] An object of cfailure
         * @param _combination [IN], A vector of string containing the sequence of commands { "destroy" , "run" , "create" }
         * @return True if there is an exception
         * @return False if there is NOT an exception
         */
        void execute_commands(cfailure& ofthr,std::vector<std::string>& _combination) {
            for ( const auto& it : _combination) {
                if ( it.compare("create") == 0 ) {
                    try {
                        ofthr.create();
                    }
                    catch (const ns_thread_base::thread_lifecycle_error& e) {
                        std::cerr << " Create Exeception lifecycle : " << e.what() <<"\n";
                    }
                    catch (const std::runtime_error& e) {
                        std::cerr << " Create Exeception runtime : " << e.what() << "\n";
                    }
                    catch (const std::exception& e) {
                        std::cerr << " Create Exeception standard failure : " << e.what() << "\n";
                    }
                }

                if ( it.compare("run") == 0 ) {
                    try {
                        ofthr.run();
                    }
                    catch (const ns_thread_base::thread_lifecycle_error& e) {
                        std::cerr << " Run Exeception lifecycle : " << e.what() <<"\n";
                    }
                    catch (const std::runtime_error& e) {
                        std::cerr << " Run Exeception runtime : " << e.what() << "\n";
                    }
                    catch (const std::exception& e) {
                        std::cerr << " Run Exeception failure : " << e.what() << "\n";
                    }
                }

                if ( it.compare("destroy") == 0 ) {
                    try {
                        ofthr.destroy();
                    }
                    catch (const ns_thread_base::thread_lifecycle_error& e) {
                        std::cerr << " Destroy Exeception lifecycle : " << e.what() <<"\n";
                    }
                    catch (const std::runtime_error& e) {
                        std::cerr << " Destroy Exeception runtime : " << e.what() << "\n";
                    }
                    catch (const std::exception& e) {
                        std::cerr << " Destroy Exeception failure : " << e.what() << "\n";
                    }
                }
            }
        }
     
        /**
         * @brief Tests the failure of a sequence of commands by throwing an exception.
         * @return True If exepected exception is verified
         * @return False If Exepected exception is NOT verified
         */
        void test_failed_sequence_of_commands() {
            int start = 2;
            int end = 5;
            for (int c = start; c < end; c++) {
                cfailure ofthr;
                ofthr.set_name("ThreadTest");
                switch (c) {
                    case (0): {
                        std::cout << "Case (0) : destroy , run , create\n";
                        std::vector<std::string> combination = { "destroy" , "run" , "create"};
                        execute_commands(ofthr,combination);
                        break;
                    }
                    case (1) : {
                        std::cout << "Case (1) : destroy , create , run\n";
                        std::vector<std::string> combination = { "destroy" , "create" , "run" };
                        execute_commands(ofthr,combination);
                        break;
                    }
                    case (2) : {
                        std::cout << "Case (2) : run , create , destroy\n";
                        std::vector<std::string> combination = { "run" , "create" , "destroy" };
                        execute_commands(ofthr,combination);
                        break;
                    }
                    case (3) : {
                        std::cout << "Case (3) : run , destroy , create\n";
                        std::vector<std::string> combination = { "run" , "destroy" , "create" };
                        execute_commands(ofthr,combination);
                        break;
                    }
                    case (4) : {
                        std::cout << "Case (4) : create , destroy , run\n";
                        std::vector<std::string> combination = { "create" , "destroy" , "run" };
                        execute_commands(ofthr,combination);
                        break;
                    }
                    default: {
                        std::cout << "\tDefault\n";
                        break;
                    }
               }
           }
        }

    };


    /**
     * @brief 
     */
    void main_failure();
    void main_failure() {
        std::cout << "\n";
        std::cout << "\n BEGIN -- Test Failure\n";
        std::cout << "\n";

        cmanager_failure ofail;
        ofail.test_failed_sequence_of_commands();

        std::cout << "\n END -- Test Failure\n";
        std::cout << "\n";
    }
} // ns_failure

