// Alice in Mediocreland (2019)

#include <cstdint>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <atomic>

#include "stack.hpp"

void test0_stack_vs_alloca();
void test1_stack_vs_heap();

constexpr size_t TEST0_ALLOCATION_SIZE = 8192;
constexpr size_t TEST0_ALLOCATION_NUM = 10000000;
constexpr size_t TEST1_PASS_NUM = 100000;
constexpr size_t TEST1_BLOCK_NUM = 64;
constexpr size_t TEST1_BLOCK_SIZE = 512;

constexpr size_t DEFAULT_ALIGNMENT = 16;

int main(int argc, const char* argv[])
{
	test0_stack_vs_alloca();
	test1_stack_vs_heap();

	return 0;
}

#pragma region Test0

double test0_stack();
double test0_alloca();

void test0_stack_vs_alloca()
{
	const double alloca_avg_time = test0_alloca();
	std::cout << "test0 (stack vs alloca) alloca time: " << alloca_avg_time << "ns" << std::endl;

	const double stack_avg_time = test0_stack();
	std::cout << "test0 (stack vs alloca) stack time: " << stack_avg_time << "ns" << std::endl;
}

double test0_stack()
{
	constexpr size_t STACK_SIZE = TEST0_ALLOCATION_SIZE + 65536;
	char buffer[STACK_SIZE];
	memset(buffer, 0, STACK_SIZE);

	initialize_thread_stack(buffer, STACK_SIZE);

	std::chrono::high_resolution_clock::time_point begin;
	std::chrono::high_resolution_clock::time_point end;
	std::chrono::duration<uint64_t, std::nano> total_time = { };

	Stack stack;

	for (size_t i = 0; i < TEST0_ALLOCATION_NUM; i++)
	{
		begin = std::chrono::high_resolution_clock::now();
		std::atomic_thread_fence(std::memory_order_acquire);
		{
			get_thread_stack(stack);
			char* memory = stack_alloc<char>(stack, TEST0_ALLOCATION_SIZE, DEFAULT_ALIGNMENT);
			memset(memory, 0xff, TEST0_ALLOCATION_SIZE);
			reset_thread_stack(stack);
		}
		std::atomic_thread_fence(std::memory_order_release);
		end = std::chrono::high_resolution_clock::now();
		total_time += end - begin;
	}

	shutdown_thread_stack();
	return double(total_time.count()) / TEST0_ALLOCATION_NUM;
}

inline void test0_alloca_impl();

double test0_alloca()
{
	std::chrono::high_resolution_clock::time_point begin;
	std::chrono::high_resolution_clock::time_point end;
	std::chrono::duration<uint64_t, std::nano> total_time = { };

	for (size_t i = 0; i < TEST0_ALLOCATION_NUM; i++)
	{
		begin = std::chrono::high_resolution_clock::now();
		std::atomic_thread_fence(std::memory_order_acquire);
		test0_alloca_impl();
		std::atomic_thread_fence(std::memory_order_release);
		end = std::chrono::high_resolution_clock::now();
		total_time += end - begin;
	}

	return double(total_time.count()) / TEST0_ALLOCATION_NUM;
}

inline void test0_alloca_impl()
{
	char* memory = (char*)_alloca(TEST0_ALLOCATION_SIZE + DEFAULT_ALIGNMENT);
	memory = align_memory(memory, DEFAULT_ALIGNMENT);
	memset(memory, 0xff, TEST0_ALLOCATION_SIZE);
}

#pragma endregion

#pragma region Test1

double test1_stack_impl();
double test1_heap_impl();

void test1_stack_vs_heap()
{
	const double heap_avg_time = test1_heap_impl();
	std::cout << "test1 (stack vs heap) heap time: " << heap_avg_time << "ns" << std::endl;

	const double stack_avg_time = test1_stack_impl();
	std::cout << "test1 (stack vs heap) stack time: " << stack_avg_time << "ns" << std::endl;
}

struct Block
{
	size_t size;
	char* data;
	Block* next;
};

Block* create_list(Block*& block_buffer, char*& block_data_buffer)
{
	block_buffer = (Block*)malloc(TEST1_BLOCK_NUM * sizeof(Block));
	memset(block_buffer, 0, TEST1_BLOCK_NUM * sizeof(Block));

	block_data_buffer = (char*)malloc(TEST1_BLOCK_NUM * TEST1_BLOCK_SIZE);
	memset(block_data_buffer, 0, TEST1_BLOCK_NUM * TEST1_BLOCK_SIZE);

	for (size_t i = 0; i < TEST1_BLOCK_NUM; i++)
	{
		Block& block = block_buffer[i];
		block.size = TEST1_BLOCK_SIZE;
		block.data = block_data_buffer + i * TEST1_BLOCK_SIZE;
		block.next = block_buffer + i + 1;
	}
	block_buffer[TEST1_BLOCK_NUM - 1].next = nullptr;

	return block_buffer;
}

double test1_stack_impl()
{
	const size_t stack_size = TEST1_BLOCK_NUM * TEST1_BLOCK_SIZE + 65536;
	char* stack_buffer = (char*)malloc(stack_size);
	initialize_thread_stack(stack_buffer, stack_size);
	memset(stack_buffer, 0, stack_size);

	Block* block_buffer;
	char* block_data_buffer;
	Block* list_item = create_list(block_buffer, block_data_buffer);

	std::chrono::high_resolution_clock::time_point begin;
	std::chrono::high_resolution_clock::time_point end;
	std::chrono::duration<uint64_t, std::nano> total_time = { };
	Stack stack;

	for (size_t i = 0; i < TEST1_PASS_NUM; i++)
	{
		begin = std::chrono::high_resolution_clock::now();

		size_t buffer_size = 0;
		size_t buffer_capacity = 1024;

		get_thread_stack(stack);
		char* buffer = stack_alloc<char>(stack, buffer_capacity, DEFAULT_ALIGNMENT);

		for (Block* block = list_item; block != nullptr; block = block->next)
		{
			const size_t block_size = block->size;
			const size_t new_buffer_size = buffer_size + block_size;

			if (new_buffer_size > buffer_capacity)
			{
				size_t new_buffer_capacity = buffer_capacity + buffer_capacity / 2;
				new_buffer_capacity = std::max(new_buffer_capacity, new_buffer_size);

				buffer = stack_realloc(stack, buffer, buffer_capacity, buffer_size,
					new_buffer_capacity, DEFAULT_ALIGNMENT);

				buffer_capacity = new_buffer_capacity;
			}

			memcpy(buffer + buffer_size, block->data, block_size);
			buffer_size = new_buffer_size;
		}

		reset_thread_stack(stack);

		end = std::chrono::high_resolution_clock::now();
		total_time += end - begin;
	}

	free(block_buffer);
	free(block_data_buffer);

	shutdown_thread_stack();
	free(stack_buffer);

	return double(total_time.count()) / TEST1_PASS_NUM;
}

double test1_heap_impl()
{
	Block* block_buffer;
	char* block_data_buffer;
	Block* list_item = create_list(block_buffer, block_data_buffer);

	std::chrono::high_resolution_clock::time_point begin;
	std::chrono::high_resolution_clock::time_point end;
	std::chrono::duration<uint64_t, std::nano> total_time = { };

	for (size_t i = 0; i < TEST1_PASS_NUM; i++)
	{
		begin = std::chrono::high_resolution_clock::now();

		size_t buffer_size = 0;
		size_t buffer_capacity = 1024;
		char* buffer = (char*)malloc(buffer_capacity);

		for (Block* block = list_item; block != nullptr; block = block->next)
		{
			const size_t block_size = block->size;
			const size_t new_buffer_size = buffer_size + block_size;

			if (new_buffer_size > buffer_capacity)
			{
				size_t new_buffer_capacity = buffer_capacity + buffer_capacity / 2;
				new_buffer_capacity = std::max(new_buffer_capacity, new_buffer_size);

				buffer = (char*)realloc(buffer, new_buffer_capacity);

				buffer_capacity = new_buffer_capacity;
			}

			memcpy(buffer + buffer_size, block->data, block_size);
			buffer_size = new_buffer_size;
		}
		free(buffer);

		end = std::chrono::high_resolution_clock::now();
		total_time += end - begin;
	}

	free(block_buffer);
	free(block_data_buffer);
	return double(total_time.count()) / TEST1_PASS_NUM;
}

#pragma endregion
