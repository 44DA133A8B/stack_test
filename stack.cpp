// Alice in Mediocreland (2019)

#include <cstdint>
#include "stack.hpp"

struct StackState
{
	char* current;
	char* end;
};

thread_local StackState tl_stack_state;

DLL void get_thread_stack(Stack& stack)
{
	StackState& state = tl_stack_state;
	stack.current = &state.current;
	stack.end = state.end;
	stack.original = state.current;
}

DLL void initialize_thread_stack(void* buffer, size_t buffer_size)
{
	StackState& state = tl_stack_state;
	state.current = (char*)buffer;
	state.end = (char*)buffer + buffer_size;
}

DLL void shutdown_thread_stack()
{
	tl_stack_state = { };
}
