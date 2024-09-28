#include <platform/platform.hpp>

#if RYTHE_PLATFORM_LINUX

#include <dlfcn.h>

namespace rsl
{
	dynamic_library platform::load_library(const char* path)
	{
		dynamic_library result;
		result.m_handle = dlopen(path, RTLD_NOW);
		return result;
	}

	void* platform::get_symbol(dynamic_library library, const char* symbolName)
	{
		return dlsym(library.m_handle, symbolName);
	}
} // namespace rsl

#endif
