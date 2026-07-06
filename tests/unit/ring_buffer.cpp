/**
 * @file ring_buffer.cpp
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

//#include <future>
//#include <chrono>
#include <atomic>
#include <iostream>
#include <cstdint>
#include <cstring>
#include <vector>

#include <gtest/gtest.h>


// Temporary bypass of access modifiers to test internal ring buffer pointers
#define private public
#define protected public
#include "../../udp_server/udp_server.hpp"
#undef private
#undef protected

/**
 * @namespace ns_unit_test_ring_buffer 
 * @brief Contains all the Google unit tests
 * @details This namespace isolates test components, mocks, and test fixtures from the production code.
 */
namespace ns_unit_test_ring_buffer {

    /**
     * @brief Test suite RingBufferTest
     * Verifies sequential writes, committed offsets.
     */
    TEST(RingBufferTest, TestBufferCommitCycle) {
        // 1. Initialize the packet manager
        ns_udp_server::ring_buffer packet_manager; 

        // 2. Request the initial memory block where the next UDP message should be written
        uint8_t* first_packet_ptr = packet_manager.get_next_offset();
        ASSERT_NE(first_packet_ptr, nullptr) << "Initial buffer pointer should not be null.";

        // Simulate writing a 256-byte packet (e.g., the decreasing sequence from your script)
        size_t first_packet_size = 256;

        // 3. Commit the written bytes to advance the internal write markers
        packet_manager.commit_packet(first_packet_size);

        // 4. Request the memory block for the second packet
        uint8_t* second_packet_ptr = packet_manager.get_next_offset();
        ASSERT_NE(second_packet_ptr, nullptr) << "Subsequent buffer pointer should not be null.";

        // VERIFICATION: The second pointer must be positioned exactly after the first packet
        EXPECT_EQ(second_packet_ptr, first_packet_ptr + first_packet_size) 
            << "The next offset pointer did not advance correctly based on the committed bytes.";

        // 5. Test edge case: Write multiple packets to verify stability
        size_t second_packet_size = 128;
        packet_manager.commit_packet(second_packet_size);

        uint8_t* third_packet_ptr = packet_manager.get_next_offset();
        EXPECT_EQ(third_packet_ptr, second_packet_ptr + second_packet_size)
            << "The third offset pointer failed to advance sequentially.";
    }

    /**
     * @brief Test suite RingBufferTest 
     * Verifies case to verify ring buffer behavior when it reaches its maximum capacity.
     */
    TEST(RingBufferTest, TestBufferWrapAroundOrFullCondition) {
        ns_udp_server::ring_buffer packet_manager;
        
        // Get the base memory address of the internal raw buffer
        // (Assuming your buffer variable inside ring_buffer is named 'buffer_data' or 'm_buffer')
        uint8_t* base_buffer_ptr = packet_manager.get_next_offset();
        ASSERT_NE(base_buffer_ptr, nullptr);

        // Fill the buffer to its maximum capacity to force a wrap-around check
        // (Replace 65536 with your actual maximum buffer capacity variable if available, e.g., packet_manager.m_capacity)
        size_t maximum_capacity = ns_udp_server::ring_buffer::max_num_bytes; 
        packet_manager.commit_packet(maximum_capacity);

        // Request a new pointer after filling the entire ring buffer
        uint8_t* wrapped_packet_ptr = packet_manager.get_next_offset();

        // VERIFICATION: Adapt this check depending on your exact implementation:
        // - If it wraps around to the beginning, it should match the base pointer.
        // - If it drops the packet because it is full, it should return nullptr.
        if (wrapped_packet_ptr == nullptr) {
            SUCCEED() << "Buffer is full, correctly rejected further writes.";
        } else {
            EXPECT_EQ(wrapped_packet_ptr, base_buffer_ptr)
                << "Ring buffer failed to wrap around to the beginning of the memory space.";
        }
    }

    /**
     * @brief Data Integrity (Write Raw Data and Read It Back) 
     */
    TEST(RingBufferTest, TestWriteAndReadDataIntegrity) {
        ns_udp_server::ring_buffer packet_manager;

        // 1. Prepare dummy data to simulate an incoming UDP payload (Decreasing sequence)
        std::vector<uint8_t> mock_payload;
        for (int i = 255; i >= 0; --i) {
            mock_payload.push_back(static_cast<uint8_t>(i));
        }

        // 2. Get the write pointer and copy the payload into the ring buffer
        uint8_t* write_ptr = packet_manager.get_next_offset();
        ASSERT_NE(write_ptr, nullptr) << "Failed to get write pointer.";
        
        std::memcpy(write_ptr, mock_payload.data(), mock_payload.size());
        
        // 3. Commit the written bytes
        packet_manager.commit_packet(mock_payload.size());
        packet_manager.set_first_packet_received();

        // 4. Read the data back from the ring buffer
        std::vector<uint8_t> read_ptr;
        packet_manager.get_last_packet(read_ptr); 

        // VERIFICATION: Check if the data read from the buffer matches the written payload
        for (size_t i = 0; i < mock_payload.size(); ++i) {
            // Eseguiamo il cast a int di ENTRAMBI i valori direttamente dentro EXPECT_EQ
            // Questo garantisce che i tipi siano identici e stampabili numericamente da GTest
            EXPECT_EQ(static_cast<int>(read_ptr[i]), static_cast<int>(mock_payload[i]))
                << "Data mismatch at buffer index " << i;
        }
    }

    /**
     * @brief Interleaved Read and Write Operations
     */
    /*TEST(RingBufferTest, TestInterleavedReadAndWrite) {
        ns_udp_server::ring_buffer packet_manager;

        // Step 1: Write Packet A (100 bytes)
        uint8_t* write_ptr_A = packet_manager.get_next_offset();
        packet_manager.commit_packet(100);

        // Step 2: Write Packet B (50 bytes)
        uint8_t* write_ptr_B = packet_manager.get_next_offset();
        EXPECT_EQ(write_ptr_B, write_ptr_A + 100);
        packet_manager.commit_packet(50);

        // Step 3: Read Packet A (Consume 100 bytes)
        uint8_t* read_ptr_1 = packet_manager.get_last_packet();
        EXPECT_EQ(read_ptr_1, write_ptr_A);
        packet_manager.pop_or_consume(100);

        // Step 4: Next read pointer must point to Packet B
        uint8_t* read_ptr_2 = packet_manager.get_read_ptr();
        EXPECT_EQ(read_ptr_2, write_ptr_B) << "Read pointer failed to advance to Packet B.";
    }*/

    /**
     * Wrap-Around Logic
     */
    /*TEST(RingBufferTest, TestBufferWrapAroundCondition) {
        ns_udp_server::ring_buffer packet_manager;
        
        uint8_t* base_buffer_ptr = packet_manager.get_next_offset();
        ASSERT_NE(base_buffer_ptr, nullptr);

        // Fill the buffer completely to trigger wrap-around
        // (Replace 65536 with your actual capacity variable, e.g., packet_manager.m_capacity)
        size_t maximum_capacity = 65536; 
        packet_manager.commit_packet(maximum_capacity);

        uint8_t* wrapped_packet_ptr = packet_manager.get_next_offset();

        if (wrapped_packet_ptr == nullptr) {
            SUCCEED() << "Buffer is full, correctly rejected further writes.";
        } else {
            EXPECT_EQ(wrapped_packet_ptr, base_buffer_ptr)
                << "Ring buffer failed to wrap around to the beginning of the memory space.";
        }
    }*/

}

/**
 * @brief
 * @details
 */
int main(int argc,char* argv[]) {
    std::cout << "\n";
    std::cout << " =============================================================\n";
    std::cout << "                  Running Ring Buffer Google Unit Test...\n";
    std::cout << " =============================================================\n";
    std::cout << "\n";
    try {
        /*
         * @brief Initi Google Test framework with arguments passando i parametri della riga di comando
         */
        ::testing::InitGoogleTest(&argc, argv);
        
        /*
         * @brief Execute all TEST e TEST_F macro return 0 succeded,  otherwise return 1
         */ 
        int ret = RUN_ALL_TESTS();
        if ( ret != 0 ) {
            throw std::runtime_error("!ERROR! Unit test failure\n");
        }
    }
    catch (const ns_udp_server::udp_server_error& e) {
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
    }

    std::cout << "\n";
    std::cout << "..done!\n";
    return EXIT_SUCCESS;
}
