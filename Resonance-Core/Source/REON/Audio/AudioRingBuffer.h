#pragma once

#include <iostream>
#include <vector>
#include <atomic>
#include <thread>
#include <cstdlib>
#include <immintrin.h>
#include <memory>
#include <new>

#ifdef __clang__
#define ENABLE_AVX __attribute__((target("avx")))
#else
#define ENABLE_AVX
#endif

namespace REON::AUDIO {
	class AudioRingBuffer {
	public:
		static constexpr size_t capacity = 4096 * 4;  // Fixed buffer size of 4096 samples

		AudioRingBuffer() : head(0), tail(0) {
			// Align buffer to 32-byte boundary
			buffer = static_cast<float*>(_aligned_malloc(capacity * sizeof(float), 32));
			if (!buffer) {
				throw std::bad_alloc();
			}
			//_aligned_free(buffer);
			//buffer = nullptr;
			// std::memset(buffer, 0, capacity * sizeof(float));
		}

		~AudioRingBuffer() {
			if (buffer) {
				if (reinterpret_cast<uintptr_t>(buffer) % 32 != 0) {
					throw std::runtime_error("Buffer is not 32-byte aligned!");
				}
				else {
					_aligned_free(buffer);
					buffer = nullptr;
				}
			}
		}

		AudioRingBuffer(const AudioRingBuffer&) = delete;
		AudioRingBuffer& operator=(const AudioRingBuffer&) = delete;

		ENABLE_AVX
		// SIMD-based bulk push of float data into the buffer
		void push_bulk(const float* data, size_t num_samples) {
			size_t next_head = (head.load(std::memory_order_relaxed) + num_samples) & (capacity - 1);

			// If the buffer will overflow, overwrite the oldest data
			if (next_head == tail.load(std::memory_order_acquire)) {
				// Move the tail forward to make space for new data
				tail.store((tail.load(std::memory_order_relaxed) + num_samples) & (capacity - 1), std::memory_order_release);
			}

			size_t i = 0;
			// Process data in bulk (SIMD)
			for (; i + 8 <= num_samples; i += 8) {  // 8 floats per SIMD operation (32 bytes)
				__m256 simd_data = _mm256_load_ps(&data[i]);  // Load 8 floats into SIMD register
				size_t write_index = (head.load(std::memory_order_relaxed) + i) & (capacity - 1);
				_mm256_store_ps(&buffer[write_index], simd_data);  // Store SIMD register back to buffer
			}

			// Process any remaining data that doesn't fit in a full SIMD chunk
			for (; i < num_samples; ++i) {
				size_t write_index = (head.load(std::memory_order_relaxed) + i) & (capacity - 1);
				buffer[write_index] = data[i];
			}

			// Update head atomically
			head.store(next_head, std::memory_order_release);
		}

		ENABLE_AVX
		void peek_latest(float* output, size_t num_samples) const {
			if (num_samples > capacity) {
				throw std::runtime_error("Requested more samples than the buffer can hold.");
			}

			size_t start_index = (head + capacity - num_samples) & (capacity - 1);

			// No wrap-around: single contiguous copy
			if (start_index + num_samples <= capacity) {
				size_t i = 0;
				// Use SIMD for aligned copying
				for (; i + 8 <= num_samples; i += 8) {
					__m256 simd_data = _mm256_load_ps(&buffer[start_index + i]);
					_mm256_store_ps(&output[i], simd_data);
				}
				// Copy remaining elements
				for (; i < num_samples; ++i) {
					output[i] = buffer[start_index + i];
				}
			}
			else {
				// Wrap-around: split into two segments
				size_t first_part_size = capacity - start_index;
				size_t i = 0;

				// SIMD for first segment
				for (; i + 8 <= first_part_size; i += 8) {
					__m256 simd_data = _mm256_load_ps(&buffer[start_index + i]);
					_mm256_store_ps(&output[i], simd_data);
				}
				// Remaining for first segment
				for (; i < first_part_size; ++i) {
					output[i] = buffer[start_index + i];
				}

				// Second segment (from the beginning of the buffer)
				size_t j = 0;
				for (; j + 8 <= (num_samples - first_part_size); j += 8) {
					__m256 simd_data = _mm256_load_ps(&buffer[j]);
					_mm256_store_ps(&output[first_part_size + j], simd_data);
				}
				// Remaining for second segment
				for (; j < (num_samples - first_part_size); ++j) {
					output[first_part_size + j] = buffer[j];
				}
			}
		}

		// Returns the number of elements in the buffer
		size_t size() const {
			return (head.load(std::memory_order_acquire) + capacity - tail.load(std::memory_order_acquire)) & (capacity - 1);
		}

		// Returns the number of available spaces in the buffer
		size_t available_space() const {
			return (tail.load(std::memory_order_acquire) + capacity - head.load(std::memory_order_acquire) - 1) & (capacity - 1);
		}

	private:
		float* buffer;  // Aligned buffer (32-byte boundary)
		std::atomic<size_t> head;  // Atomic read pointer
		std::atomic<size_t> tail;  // Atomic write pointer
	};
}