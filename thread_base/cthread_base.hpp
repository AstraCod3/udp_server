/**
 * @file cthread_base.hpp
 * @brief This source file contains the declaration of the ns_base_thread namespace
 * @details
 * @author AstraCod3
 * @date June 28, 2026
 * @version 1.0.0
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
#ifndef CTHREAD_BASE_H_
#define CTHREAD_BASE_H_

#include <thread>               // std::thread
#include <mutex>                // std::mutex
#include <condition_variable>   // std::condition_variable
#include <utility>              // std::pair
#include <map>                  // std::map
#include <list>                 // std::list
#include <vector>               // std::vector
#include <string>               // std::string
#include <cstring>              // std::memcpy
#include <stdexcept>            // std::runtime_error

#if defined(__posix) || defined(__linux__) || defined(__uinux__)
    #include <unistd.h>
    #include <sys/syscall.h>
    #include <sys/types.h>      // gettid()
#endif

#if defined(_WIN64) || defined(_WIN32)
#include <codecvt>
#include <locale>
#include <windows.h>
#endif

/**
 * @namespace ns_thread_base
 * @brief Gather all class, struct, enum etc to handling thread
 */
namespace ns_thread_base {

    /**
     * @class thread_lifecycle_error
     * @brief Custom exception thrown when a thread operation violates the lifecycle rules.
     * @details Inherits from std::runtime_error to maintain compatibility with standard 
     *          exception handling while preventing exception pollution.
     */
    class thread_lifecycle_error : public std::runtime_error {
    public:
        /**
         * @brief Constructor for thread_lifecycle_error.
         * @param message The detailed error message.
         */
        explicit thread_lifecycle_error(const std::string& message) 
            : std::runtime_error(message) { }
    };

    /**
     * @enum THR_STATUS
     * @brief The possible statuses are:
     * - INIT: Thread is in the initialization status.
     * - CREATE: Thread is in the creation status.
     * - READY: Thread is in the reading status.
     * - RUN: Thread is currently running.
     * - EXIT: Thread has exited.
     */
    enum class THR_STATUS : unsigned char{
        INIT,
        CREATE,
        READY,
        RUN,
        EXIT,
        DESTROY
    };

    /**
     * @class cthread_base
     * @brief This class create, init, and handling the base_thread_func
     */
    class cthread_base {

    public:

        /**
         * @brief Costructor cthread_base
         */
        cthread_base() :
            m_thr_name("") {
            init();
        }

        /**
         * @brief Costructor cthread_base
         */
        cthread_base(const std::string& _thr_name) :
            m_thr_name(_thr_name) {
            init();
        }

        /**
         * @brief Destructor cthread_base
         */
        virtual ~cthread_base() {
            if (is_destroyed() == false) {
                destroy();
            }
        }

        /**
         * @brief Spawns the underlying worker thread. This function is BLOCKING.
         * @details Allocates and initializes the internal std::thread executing 
         *          the base_thread_func(). The calling thread will safely block 
         *          until the thread signals that it has reached the READY status.
         * 
         * @throw std::bad_alloc If memory allocation for the std::thread object fails.
         * @throw thread_lifecycle_error If called while the thread already exists in an invalid lifecycle state:
         *        - "Called 'create()'. Current status CREATE, wait to READY status before call 'run()'."
         *        - "Called 'create()' Thread already exists. Current status READY, call 'run()'."
         *        - "Called 'create()' Thread already exists. Current status RUN."
         *        - "Called 'create()' Thread already exists. Current status EXIT."
         *        - "Called 'create()' Thread already exists. Wait 'destroy()' before call 'create()'."
         */
        void create() {

            auto alloc_ptr_thr = [this]() {
                std::unique_lock<std::mutex> lck_ready( mmtx_ready );
                std::unique_lock<std::mutex> lck_pthr( mmtx_pthr );
                mptr_thr = new std::thread( &cthread_base::base_thread_func, this );
                lck_pthr.unlock();
                mcv_ready.wait(lck_ready, [this] { return is_ready(); });

            };

            switch ( get_status() ) {
                case THR_STATUS::INIT : {
                    if ( is_thread_ptr_null() ) {
                        alloc_ptr_thr();
                    }
                    else {
                        throw thread_lifecycle_error("Called \"create()\" Thread already exists. Get Thread status : " + get_status_string());
                    }
                break;
                }

                case THR_STATUS::CREATE : {
                    if ( !is_thread_ptr_null() ) {
                        throw thread_lifecycle_error("Called \"create()\". Current status CREATE, wait to READY status before call \"run()\". Get Thread status : " + get_status_string());
                    }
                break;
                }

                case THR_STATUS::READY : {
                    if ( !is_thread_ptr_null() ) {
                        throw thread_lifecycle_error("Called \"create()\" Thread already exists. Current status READY, call \"run()\". Get Thread status : " + get_status_string());
                    }
                break;
                }

                case THR_STATUS::RUN : {
                    if ( !is_thread_ptr_null() ) {
                        throw thread_lifecycle_error("Called \"create()\" Thread already exists. Current status RUN. Get Thread status : " + get_status_string());
                    }
                break;
                }

                case THR_STATUS::EXIT : {
                    if ( !is_thread_ptr_null() ) {
                        throw thread_lifecycle_error("Called \"create()\" Thread already exists. Current status EXIT. Get Thread status : " + get_status_string());
                    }
                break;
                }

                case THR_STATUS::DESTROY : {
                    if ( is_thread_ptr_null() ) {
                        init();
                        alloc_ptr_thr();
                    }
                    else {
                        throw thread_lifecycle_error("Called \"create()\" Thread already exists. Wait \"destroy()\" before call \"create()\". Get Thread status : " + get_status_string());
                    }
                break;
                }
                default : {
                    throw thread_lifecycle_error("Called \"create()\". Unknown Thread status ");
                break;
                }
            }
        }

        /**
         * @brief Runs the thread.
         *        The function is !!!NOT BLOCKING!!!.
         *        This function must be called when we want the thread to run.
         *        It executes the "thread_function()" in the thread.
         * @throw thread_lifecycle_error Throw if the thread is NOT created
         */
        void run() {
            if ( !is_thread_ptr_null() ) {
                bool run = true;
                bool exit = false;
                command_data tmp_cmd;

                std::unique_lock<std::mutex> lck_cmd( mmtx_cmd );
                tmp_cmd.brun_bexit = std::make_pair( run, exit );
                mcmd.push_back( tmp_cmd );
                lck_cmd.unlock();

                std::unique_lock<std::mutex> lck_cv( mmtx_cv_thr );
                mcv_thr.notify_one();
                lck_cv.unlock();
            }
            else {
                throw thread_lifecycle_error("Called \"run()\". Thread is NOT created, call \"create()\" before run. " + get_status_string() );
            }
        }

        /**
         * @brief Destroys the thread.
         *        This function is !!!BLOCKING!!!
         *        It must be called to destroy the internal object.
         *        Deletes the 'mptr_thr' pointer and sets it to nullptr.
         * 
         * @throw thread_lifecycle_error Throw if the thread was not created.
         */
        void destroy() {
            if ( !is_thread_ptr_null() ) {
                
                exit();
                
                if( mptr_thr->joinable() )
                    mptr_thr->join();

                std::unique_lock<std::mutex> lck_pthr( mmtx_pthr );
                delete mptr_thr;
                mptr_thr = nullptr;
                lck_pthr.unlock();

                set_status( THR_STATUS::DESTROY );
            }
            else {
                throw thread_lifecycle_error("Called \"destroy()\". Thread is NOT created, call \"create()\" before destroyed. " + get_status_string() );
            }
        }

        /**
         * @brief Return the status of the thread in enum format THR_STATUS
         * @return Returns enum THR_STATUS
         */
        THR_STATUS get_status() const noexcept {
            return mstatus.load();
        }

        /**
         * @brief Return the status of the thread in string format.
         * @return Returns thread status type string
         */
        std::string get_status_string() const noexcept {
            std::string ret{""};
            switch ( get_status() ) {
                case THR_STATUS::INIT:      ret = "THREAD STATUS INIT";    break;
                case THR_STATUS::CREATE:    ret = "THREAD STATUS CREATE";  break;
                case THR_STATUS::READY:     ret = "THREAD STATUS READY";   break;
                case THR_STATUS::RUN:       ret = "THREAD STATUS RUN";     break;
                case THR_STATUS::EXIT:      ret = "THREAD STATUS EXIT";    break;
                case THR_STATUS::DESTROY:   ret = "THREAD STATUS DESTROY"; break;
                default:                    ret = "UNKWON THREAD STATUS";  break;
            }
            return ret;
        }

        /**
         * @brief Returns whether the thread is in a ready status.
         * @return TRUE The thread is ready status.
         * @return FALSE The thread is NOT ready status.
         */
        bool is_ready() const noexcept {
            return mstatus.load() == THR_STATUS::READY;
        }
        
        /**
         * @brief Returns whether the thread is in a running status.
         * @return TRUE The hread is running status.
         * @return FALSE The hread is NOT running status.
         */
        bool is_running() const noexcept {
            return mstatus.load() == THR_STATUS::RUN;
        }

        /**
         * @brief  Returns whether the thread has exited.
         * @return TRUE The thread has exited
         * @return FALSE The thread has NOT exited
         */
        bool is_exited() const noexcept {
            return mstatus.load() == THR_STATUS::EXIT;
        }

        /**
         * @brief  Returns whether the thread has exited.
         * @return TRUE The thread has exited
         * @return FALSE The thread has NOT exited
         */
        bool is_destroyed() const noexcept {
            return mstatus.load() == THR_STATUS::DESTROY;
        }

        /**
         * @brief Returns thread id
         * @return Returns thread id
         */
        long int get_tid() const noexcept {
            return m_thr_id.load(std::memory_order_relaxed);
        }

        /**
         * @brief Set Name of thread
         * @param _thr_name [in] name of thread
         * @throw thread_lifecycle_error Throw if the thread is NOT created
         */
        void set_name(const std::string& _thr_name) {
            if (get_status() ==  THR_STATUS::INIT) {
                std::unique_lock<std::mutex> lck(mmtx_thr_name);
                m_thr_name = _thr_name;
            }
            else {
                throw thread_lifecycle_error("Called \"set_thread_name()\". Set name of thread before call \"create()\". Get Thread status : " + get_status_string());
            }
        }

        /**
         * @brief Get name of thread
         * @return Return string with name of thread otherwise its returns an empty string
         */
        std::string get_name() noexcept {
            std::string ret = "";
            std::unique_lock<std::mutex> lck(mmtx_thr_name);
            if ( !m_thr_name.empty() )
                ret = m_thr_name;
            lck.unlock();
            return ret;
        }

        /**
         * @brief Gets the total number of completed loop iterations.
         * @details This read operation is fully thread-safe and lock-free, 
         *          utilizing an atomic load instruction.
         * @return The absolute number of completed loops.
         */
        unsigned long long get_loop_cnt() const noexcept {
            return mloop_cnt.load(std::memory_order_relaxed);
        }

    private:

        /**
         * @brief Mutex for handling the critical section of m_thr_name
         */
        std::mutex mmtx_thr_name;

         /**
         * @brief Name of thread
         */
        std::string m_thr_name;

        /**
         * @brief Mutex for handling the critical section of thread pointer
         *      In a multithreaded context, 
         *      functions create and destroy may be called simultaneously
         */
        std::mutex mmtx_pthr;

        /**
         * @brief Pointer of thread
         */
        std::thread* mptr_thr;

        /**
         * @brief Mutex for handling the critical section of mcommand
         */
        std::mutex mmtx_cmd;

        /**
         * @brief Commands for managing the thread base_thread_func()
         */
        struct command_data {
            std::pair< bool, bool > brun_bexit;
        };

        /**
         * @brief List Command for the thread base_thread_func().
         *        commands : run or exit.
         */
        std::list< command_data > mcmd;

        /**
         * @brief Counter for the number of times cfuntionpointer::call_thread is called
         */
        std::atomic< unsigned long long > mloop_cnt;

        /**
         * @brief Mutex for handling critical section of Condition Variable mcv_thr
         */
        std::mutex mmtx_cv_thr;

        /**
         * @brief Condition Variable for thread base_thread_func()
         */
        std::condition_variable mcv_thr;

        /**
         * @brief Mutex status used in create() and base_thread_func()
         */
        std::mutex mmtx_ready;

        /**
         * @brief Condition Variable status used in create() and base_thread_func() 
         */
        std::condition_variable mcv_ready;

        /**
         * @brief Thread-safe lifecycle status.
         */
        std::atomic<THR_STATUS> mstatus{THR_STATUS::INIT};

        /**
         * @brief Stores The Thread Id assigned by operative system
         * @note std::thread::id mthrid ???
         * @todo - Implement other OperativeSystem
         *          #if defined(__FreeBSD__)
         *              type for FreeBSD m_tid;
         *          #endif
         *          
         *          #if defined(__NetBSD__)
         *             type for __NetBSD__ m_tid
         *          #endif
         *          
         *          #if defined(__OpenBSD__)
         *             type for __OpenBSD__ m_tid
         *          #endif
         */
        #if (__linux__) || defined(_WIN64) || defined(_WIN32)
        std::atomic<long int> m_thr_id;
        #endif

        /**
         * @brief Sets name of thread for Operative System.
         *        Must be call within base_thread_func().
         *        Any names longer than 15 characters will only keep the first 15 characters; the others will be truncated."
         * @todo - Implement FAILED
         *       - Implement other OperativeSystem
         *          #if defined(__FreeBSD__)
         *          ...
         *          #endif
         *          
         *          #if defined(__NetBSD__)
         *          ...
         *          #endif
         *          
         *          #if defined(__OpenBSD__)
         *          ...
         *          #endif
         */
        void setting_name() {
            std::unique_lock<std::mutex> lck(mmtx_thr_name);
            if ( !m_thr_name.empty() ) {
                #if defined(__linux__) || defined(__posix) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
                // pthread_setname_np has different signatures on different platforms
                // On Linux, it takes (pthread_t, const char*)
                // Max 16 chars including null terminator
                pthread_setname_np(pthread_self(),m_thr_name.substr(0, 15).c_str());
                #endif

                #if defined(_WIN64) || defined(_WIN32)
                std::wstring wThreadName(m_thr_name.begin(), m_thr_name.end());
                HRESULT hr = SetThreadDescription(GetCurrentThread(), wThreadName.c_str());
                /* @todo
                    if (FAILED(hr)) {
                    ret = EXIT_FAILURE;
                }*/
                #endif
            }
        }

        /**
         * @brief Set the thread Id assigned by operative system into m_tid
         * @note 
         *
         *   m_tid = std::this_thread::get_id();
         *
         *   pthread_id_np_t   tid;
         *   pthread_t         self;
         *   self = pthread_self();
         *   pthread_getunique_np(&self, &tid);
         *
         *   m_thr_id.store(syscall(SYS_gettid), std::memory_order_relaxed);
         *   m_thr_id.store(syscall(SYS_gettid), std::memory_order_relaxed);
         *
         * @todo - Implement other Operative System
         *          #endif
         */
        void set_thr_id() {

            #if defined(__linux__)
                m_thr_id.store(gettid(), std::memory_order_relaxed);
            #elif defined(__FreeBSD__)
                long tid;
                thr_self(&tid);
                m_thr_id.store((int)tid, std::memory_order_relaxed);
            #elif defined(__NetBSD__)
                m_thr_id.store(_lwp_self(), std::memory_order_relaxed);
            #elif defined(__OpenBSD__)
                m_thr_id.store(getthrid(), std::memory_order_relaxed);
            #elif defined(_WIN64) || defined(_WIN32)
                m_thr_id.store( GetCurrentThreadId(), std::memory_order_relaxed);
                // m_thr_id.store( syscall(SYS_gettid), std::memory_order_relaxed);
            #else
                m_thr_id.store( std::this_thread::get_id(), std::memory_order_relaxed);
                // m_thr_id.store( getpid(), std::memory_order_relaxed);
            #endif

        }

        /**
         * @brief Returns the first element of the list "mcmd"
         * @param run_ [out] TRUE thread will be running status. FALSE thread in ready status.
         * @param exit_ [out] TRUE thread will be exiting status (run_ must be true). FALSE thread will be running or ready status.
         */
        void get_command(bool& run_,bool& exit_) noexcept {
            std::unique_lock<std::mutex> lck_cmd( mmtx_cmd );
            if( mcmd.size() > 0 ) {
                command_data tmp = mcmd.front();
                mcmd.pop_front();
                run_ = tmp.brun_bexit.first;
                exit_ = tmp.brun_bexit.second;
            }
            else {
                // first time set thread status ready
                run_ = false;
                exit_ = false;
            }
        }

        /**
         * @brief Set the status of thread
         * @param _s [in] The status of thread
         */
        void set_status(enum THR_STATUS _s) {
            mstatus.store(_s, std::memory_order_relaxed);
        }

        /**
         * @brief Check if Thread pointer is null
         * @return true : if thread pointer is NULL
         * @return false : if thread pointer is NOT NULL
         */
        bool is_thread_ptr_null() noexcept {
            bool ret = false;
            std::unique_lock<std::mutex> lck_pthr( mmtx_pthr );
            if ( mptr_thr == nullptr )
                ret = true;
            lck_pthr.unlock();
            return ret;
        }

        /**
         * @brief The thread 
         */
        void base_thread_func() {
            set_status(THR_STATUS::CREATE);
            bool brun = false;
            bool bexit = false;
            set_thr_id();
            setting_name();

            // reset loop counter
            mloop_cnt.store(0, std::memory_order_relaxed);

            while (!bexit) {
                get_command(brun, bexit);
                std::unique_lock<std::mutex> lck(mmtx_cv_thr);
                while (!brun) {

                    // acquire status lock (released by create()'s wait call)
                    std::unique_lock<std::mutex> lck_ready(mmtx_ready);
                    set_status(THR_STATUS::READY);
                    // send notify to create() case THR_STATUS::INIT or THR_STATUS::DESTROY
                    mcv_ready.notify_one(); 
                    lck_ready.unlock();

                    mcv_thr.wait(lck);
                    get_command(brun, bexit);
                }
                lck.unlock();

                if(bexit) {
                    break;
                } // !brun loop

                set_status(THR_STATUS::RUN);

                //////////////////////////////////////
                // do somethings start ...
                thread_function();
                // do somethings ... end
                //////////////////////////////////////

                // increase loop counter
                mloop_cnt.fetch_add(1, std::memory_order_relaxed);

                brun = false;
            } // !bexit loop
            set_status(THR_STATUS::EXIT);
        }

        /**
         * @brief Initialize thread, called from cthread_base Costructuren, and "destroy()"
         */
        void init() {

            m_thr_id.store( -1, std::memory_order_relaxed );

            std::unique_lock<std::mutex> lck_pthr( mmtx_pthr );
            mptr_thr = nullptr;
            lck_pthr.unlock();

            std::unique_lock<std::mutex> lck_cmd( mmtx_cmd );
            mcmd.clear();
            lck_cmd.unlock();

            set_status(THR_STATUS::INIT);
        }

        /**
         * @brief Exit thread, called from "destroy()"
         */
        void exit() {
            bool run = true;
            bool exit = true;
            command_data tmp_cmd;

            std::unique_lock<std::mutex> lck_cmd(mmtx_cmd);
            tmp_cmd.brun_bexit = std::make_pair(run,exit);
            mcmd.push_back(tmp_cmd);
            lck_cmd.unlock();

            std::unique_lock<std::mutex> lckcv(mmtx_cv_thr);
            mcv_thr.notify_one();
            lckcv.unlock();
        }

        protected:

        /**
         * @brief Pure virtual function that must be implemented by derived classes.
         *        This function will define the main logic that the thread executes when
         *        it starts running. Each derived class must provide its own implementation
         *        of this functionThe true thread function implemented by class derived 
         */
        virtual void thread_function() = 0;

    }; // class cthread_base
} // namespace ns_manage_thread

#endif // CTHREAD_BASE_H_
