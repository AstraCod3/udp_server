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

        ns_udp_server::ring_buffer packet_manager; 

        // Simulate initial buffer
        uint8_t* init_packet_ptr = nullptr;
        packet_manager.advance_write_pointer(0, init_packet_ptr);
        EXPECT_EQ( init_packet_ptr, packet_manager.mdata )
            << "The init pointer is not equal mdata[0].";

        // Simulate writing a 256-byte packet (e.g., the decreasing sequence from your script)
        size_t first_packet_size = 256;
        uint8_t* first_packet_ptr = nullptr;
        packet_manager.advance_write_pointer(first_packet_size, first_packet_ptr);
        EXPECT_EQ( first_packet_ptr, init_packet_ptr + first_packet_size )
            << "The first pointer is not equal mdata[0].";

        // Request the memory block for the second packet
        size_t second_packet_size = 128;
        uint8_t* second_packet_ptr = nullptr;
        packet_manager.advance_write_pointer( second_packet_size, second_packet_ptr );
        EXPECT_EQ( second_packet_ptr, init_packet_ptr + first_packet_size + second_packet_size )
            << "The second pointer is not equal mdata[first_size + second_size +1].";

        size_t third_packet_size = 128;
        uint8_t* third_packet_ptr = nullptr;
        packet_manager.advance_write_pointer( third_packet_size , third_packet_ptr );
        EXPECT_EQ( third_packet_ptr , init_packet_ptr + first_packet_size + second_packet_size + third_packet_size )
            << "The third pointer is not equal mdata[first_size + second_size + third_size + 1].";

        EXPECT_EQ( third_packet_ptr , packet_manager.mdata )
            << "The third_packet_ptr pointer is not equal mdata";
    }

    /**
     * @brief Test suite RingBufferTest 
     * Verifies case to verify ring buffer behavior when it reaches its maximum capacity.
     */
    TEST(RingBufferTest, TestBufferWrapAroundOrFullCondition) {
        ns_udp_server::ring_buffer packet_manager;

        uint8_t* base_buffer_ptr = nullptr;
        packet_manager.advance_write_pointer(0, base_buffer_ptr );
        ASSERT_NE(base_buffer_ptr, nullptr);

        size_t maximum_capacity = ns_udp_server::ring_buffer::MAX_NUM_BYTES;
        uint8_t* wrapped_packet_ptr = nullptr;
        packet_manager.advance_write_pointer(maximum_capacity, wrapped_packet_ptr );

        ASSERT_NE(wrapped_packet_ptr , nullptr);
        EXPECT_EQ(wrapped_packet_ptr, base_buffer_ptr + maximum_capacity )
            << "Ring buffer failed to wrap around to the beginning of the memory space.";
    }

    /**
     * @brief Data Integrity (Write Raw Data and Read It Back) 
     */
    TEST(RingBufferTest, TestWriteAndReadDataIntegrity) {
        ns_udp_server::ring_buffer packet_manager;

        // Prepare dummy data to simulate an incoming UDP payload (Decreasing sequence)
        const std::size_t size_payload = 256;
        std::vector<uint8_t> mock_payload;
        for (int i = size_payload ; i > 0; i--) {
            mock_payload.push_back(static_cast<uint8_t>(i));
        }

        // Get the write pointer and copy the payload into the ring buffer
        uint8_t* write_ptr = nullptr;
        packet_manager.advance_write_pointer( 0, write_ptr );
        ASSERT_EQ(write_ptr, (packet_manager.mdata))
            << "The write pointer is not equal mdata[0].";
        std::memcpy(write_ptr, mock_payload.data(), mock_payload.size());
        packet_manager.advance_write_pointer( size_payload, write_ptr );

        // Read the data back from the ring buffer
        std::vector<uint8_t> read;
        packet_manager.get_last_packet_async(read); 
        // std::this_thread::sleep_for(std::chrono::seconds(1));
        ASSERT_EQ(size_payload, read.size() ) << "The size is not equal.";

        // VERIFICATION: Check if the data read from the buffer matches the written payload
        for (size_t i = 0; i < mock_payload.size() - 1; i++) {
            EXPECT_EQ(read[i], mock_payload[i]) << "Data mismatch at buffer index " << i;
        }
    }

    /**
     * @brief Write Wrap-Around Logic
     */
    TEST(RingBufferTest, TestBufferWrapAroundCondition) {
        // Prepare dummy data to simulate an incoming UDP payload (Decreasing sequence)
        const std::size_t size_payload = 25;
        std::vector<uint8_t> mock_payload;
        for (std::size_t i = 0; i < size_payload ; i++) {
            if ( i < 10 ) {
                mock_payload.push_back(static_cast<uint8_t>(0xAA));
            }
            else {
                mock_payload.push_back(static_cast<uint8_t>(0x1F));
            }
        }

        ns_udp_server::ring_buffer packet_manager;

        uint8_t* base_buffer_ptr = nullptr;
        packet_manager.advance_write_pointer(0, base_buffer_ptr);
        ASSERT_NE(base_buffer_ptr, nullptr);

        uint8_t* write_ptr_adv = nullptr;
        uint8_t* write_ptr = nullptr;
        size_t capacity = ns_udp_server::ring_buffer::MAX_NUM_BYTES; 
        packet_manager.advance_write_pointer( capacity - 10, write_ptr_adv );
        write_ptr = base_buffer_ptr + (capacity - 10);
        EXPECT_EQ(write_ptr,write_ptr_adv);

        std::memcpy(write_ptr, mock_payload.data(), size_payload);

        uint8_t* new_write_ptr = nullptr;
        packet_manager.advance_write_pointer(size_payload, new_write_ptr );

        EXPECT_EQ(base_buffer_ptr[capacity - 1], 0xAA);
        EXPECT_EQ(base_buffer_ptr[0], 0x1F);       
    }

}

/**
 * @brief
 * @details
 */
int main(int argc,char* argv[]) {
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

    return EXIT_SUCCESS;
}
