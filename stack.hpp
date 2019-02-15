// Alice in Mediocreland (2019)

#pragma once

#if STACK_EXPORT
	#define DLL  extern "C" __declspec(dllexport)
#else
	#define DLL  extern "C" __declspec(dllimport)
#endif

struct Stack
{
	char** current;
	char* end;
	char* original;
};

DLL void initialize_thread_stack(void* buffer, size_t buffer_size);
DLL void shutdown_thread_stack();
DLL void get_thread_stack(Stack& stack);

template<typename T>
inline T* stack_alloc(Stack& stack, size_t size, size_t alignment);

template<typename T>
inline T* stack_realloc(Stack& stack, T* memory, size_t prev_size,
	size_t copy_size, size_t new_size, size_t alignment);

inline void reset_thread_stack(Stack& stack);

struct StackScope
{
	inline StackScope();
	inline ~StackScope();

	inline operator Stack&();

private:
	Stack _stack;
};

#pragma region Inline

template< typename T >
constexpr T* align_memory(T* value, size_t alignment);

template<typename T>
inline T* stack_alloc(Stack& stack, size_t size, size_t alignment)
{
	char* const current = *stack.current;
	char* const memory = align_memory(current, alignment);
	char* const memory_end = memory + size * sizeof(T);

	const bool has_enough_space = memory_end <= stack.end;

	*stack.current = has_enough_space ? memory_end : current;

	return has_enough_space ? memory : nullptr;
}

template<typename T>
inline T* stack_realloc(Stack& stack, T* memory, size_t prev_size,
	size_t copy_size, size_t new_size, size_t alignment)
{
	if (memory == nullptr)
		return stack_alloc<T>(stack, new_size, alignment);

	char* const current = *stack.current;
	char* const memory_end = (char*)memory + prev_size;

	if (memory_end == current)
	{
		char* const new_memory = align_memory(memory, alignment);
		char* const new_memory_end = new_memory + new_size * sizeof(T);

		if (new_memory_end > stack.end)
			return nullptr;

		if (new_memory != memory)
			memmove(new_memory, memory, copy_size * sizeof(T));

		*stack.current = new_memory_end;
		return memory;
	}

	char* const new_memory = stack_alloc<T>(stack, new_size, alignment);

	if (new_memory != nullptr)
		memcpy(new_memory, memory, copy_size * sizeof(T));

	return new_memory;
}

inline void reset_thread_stack(Stack& stack)
{
	*stack.current = stack.original;
}

template< typename T >
constexpr T* align_memory(T* value, size_t alignment)
{
	return (T*)(((size_t)value + alignment - 1) / alignment * alignment);
}

inline StackScope::StackScope()
{
	get_thread_stack(_stack);
}

inline StackScope::~StackScope()
{
	reset_thread_stack(_stack);
}

inline StackScope::operator Stack&()
{
	return _stack;
}

#pragma endregion
