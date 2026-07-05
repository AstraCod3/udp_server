/**
 * @file test.cpp
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

#include "google_unit_test.hpp"

/**
 * @brief Main to execute example and tests
 */
int main(int argc,char* argv[]) {

    std::cout << "\n";
    std::cout << " =============================================================\n";
    std::cout << "                  Run Unit Tests ...\n";
    std::cout << " =============================================================\n";
    std::cout << "\n";

    /*if (argc > 1) {
        std::cout << "list of arguments:\n";
        for (int i=0; i<argc; i++)
            std::cout << "\t[" << i << "] : " << argv[i] << std::endl;
    }*/

    // try {
        ns_google_unit_tests::main_unit_tests(argc,argv);
    /*}
    catch (const ns_thread_base::thread_lifecycle_error& e) {
        std::cout << "Exeception failure\n";
        std::cout << e.what() <<"\n";
        return EXIT_FAILURE; 
    }
    catch (const std::runtime_error& e) {
        std::cerr << "Exeception runtime failure : " << e.what() << std::endl;
        return EXIT_FAILURE; 
    }
    catch (const std::exception& e) {
        std::cerr << "Exeception standard failure : " << e.what() << std::endl;
        return EXIT_FAILURE; 
    }*/

    std::cout << "..done!\n";

    return EXIT_SUCCESS;
}
