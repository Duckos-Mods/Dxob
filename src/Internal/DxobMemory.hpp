#pragma once
#include "DxobTypes.hpp"
#include <malloc.h>

namespace Dxob
{
	template<typename T>
	T* talloc(std::size_t size) { return static_cast<T*>(malloc(size * sizeof(T))); }

	template<typename T>
	T* talloc(std::size_t size, std::size_t alignment) { return static_cast<T*>(_aligned_malloc(size * sizeof(T), alignment)); }

	template<typename T>
	T* tralloc(T* ptr, std::size_t size) { return static_cast<T*>(realloc(ptr, size * sizeof(T))); }

	template<typename T>
	void tfree(T* ptr) { free(ptr); }

	template<typename T>
	void tfree(T* ptr, Dxob::u64 alignment) { _aligned_free(ptr); }

	template<typename T>
	T* tcalloc (Dxob::u64 size) { return (T*)calloc(size, sizeof(T)); }

	template<typename T>
	T* tcalloc(Dxob::u64 size, Dxob::u64 alignment) 
	{ 
		T* ptr = (T*)_aligned_malloc(size * sizeof(T), alignment);
		memset(ptr, 0, size * sizeof(T));
		return ptr;
	}
}