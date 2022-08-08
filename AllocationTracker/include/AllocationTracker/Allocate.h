#include <typeinfo>
#include <unordered_map>
#include <thread>
#include <assert.h>
#include <iostream>
#include <mutex>

#if defined(__clang__) || defined(__GNUC__)
#define CIN_ALLOCATOR_FORCE_INLINE	inline __attribute__((always_inline))
#elif defined(_MSC_VER)
#define CIN_ALLOCATOR_FORCE_INLINE	__forceinline
#else
#define CIN_ALLOCATOR_FORCE_INLINE	inline
#endif

#define CIN_ALLOCATOR_SHARED_STATE 0 /* Can free allocated memory from all threads */ 
#define CIN_ALLOCATOR_USE_NOTHROW_NEW 1 /* Disable exceptions */
#if CIN_ALLOCATOR_USE_NOTHROW_NEW 
#define CIN_ALLOCATOR_THROW_ATTRIBUTE (std::nothrow)
#else
#define CIN_ALLOCATOR_THROW_ATTRIBUTE
#endif

/*
* When the compiler sees a _new_ expression, it first deduces the appropriate overloaded
* new operator (::operator new(...)), depending on the number and type of passed arguments, then,
* the function is invoked, returning a pointer to raw block of memory.
*
* new expression:
* int* integer = new int;
* new operator:
* void* storagePointer = ::operator new(sizeof (int));
* ... and using new expression:
* int* integer = new (storagePointer) int;
* Note that _new_ expression performs no allocation.
*/

typedef void (*MemoryDumpCallbackFunction_t)(const void* allocation, const char* typeName, const std::size_t allocationSize, const char* filename, const std::size_t fileLine);
static MemoryDumpCallbackFunction_t s_DumpCallbackFunction{ [](const void* allocation, const char* typeName, const std::size_t allocationSize, const char* filename, const std::size_t fileLine)
{
		/* To prevent IOStream from going nuts */
		static std::mutex mutex;
		std::lock_guard<std::mutex> lock(mutex);
		{
			std::cout << "[WARNING]\n";
			std::cout << "[Thread : " << std::this_thread::get_id() << "] Found unfreed memory at 0x" << allocation << " (" << allocationSize << " Bytes) of type " << typeName << " [" << filename << " | " << fileLine << "]\n";
			std::cout << '\n';
		}
	} };

static void SetMemoryDumpCallback(MemoryDumpCallbackFunction_t callback) noexcept
{
	static std::mutex mutex;
	std::lock_guard<std::mutex> lock(mutex);
	{
		s_DumpCallbackFunction = callback;
	}
}
#if CIN_ALLOCATOR_SHARED_STATE
class ThreadAllocatorData final
#else
thread_local class ThreadAllocatorData final
#endif
{
public:
	struct AllocationData
	{
		explicit AllocationData(const char* name, const std::size_t size, const char* file, const std::size_t line) noexcept
			:
			Name(name),
			Size(size),
			File(file),
			Line(line)
		{}

		const char* Name;
		std::size_t Size;
		const char* File;
		std::size_t Line;
	};

#if CIN_ALLOCATOR_SHARED_STATE
	static inline std::unordered_map<const void*, AllocationData> Allocations;
	static inline std::mutex s_AllocationRegistryMutex;
#else
	std::unordered_map<const void*, AllocationData> Allocations;
#endif
public:
	~ThreadAllocatorData() noexcept
	{
		DumpThreadMemory();
	}

	CIN_ALLOCATOR_FORCE_INLINE void DumpThreadMemory()
	{
#if CIN_ALLOCATOR_SHARED_STATE
		std::lock_guard<std::mutex> lock(s_AllocationRegistryMutex);
#endif
		if (!Allocations.empty())
		{
			for (const auto& [address, allocation] : Allocations)
				s_DumpCallbackFunction(address, allocation.Name, allocation.Size, allocation.File, allocation.Line);
		}
	}

	CIN_ALLOCATOR_FORCE_INLINE void Register(const void* address, const char* name, const std::size_t size, const char* file, const std::size_t line) noexcept
	{
#if CIN_ALLOCATOR_SHARED_STATE
		std::lock_guard<std::mutex> lock(s_AllocationRegistryMutex);
#endif
		Allocations.emplace(address, std::move(AllocationData{ name, size, file, line }));
	}

	CIN_ALLOCATOR_FORCE_INLINE void Unregister(const void* address) noexcept
	{
#if CIN_ALLOCATOR_SHARED_STATE
		std::lock_guard<std::mutex> lock(s_AllocationRegistryMutex);
#endif
		assert(Allocations.find(address) != Allocations.end());
		Allocations.erase(address);
	}
} static t_ThreadAllocatorData;

static void CinDumpThreadMemory()
{
	t_ThreadAllocatorData.DumpThreadMemory();
}

struct AllocationProxy final
{
	const char* File;
	const uint32_t Line;

	explicit constexpr AllocationProxy(const char* file, const uint32_t line) noexcept
		:
		File(file),
		Line(line)
	{}

	~AllocationProxy() noexcept = default;

	template <typename T>
	[[nodiscard]] T* operator << (T* const allocation) const noexcept
	{
		/* Forward the allocation to variable */
		t_ThreadAllocatorData.Register(allocation, typeid(T).name(), sizeof(T), File, Line);
		return allocation;
	}
};

struct DeallocationProxy final
{
	explicit constexpr DeallocationProxy() noexcept = default;
	~DeallocationProxy() noexcept = default;

	template<typename T>
	void operator << (T* const address) noexcept
	{
		t_ThreadAllocatorData.Unregister(address);
		delete address;
	}
};

struct DeallocationProxyArray final
{
	explicit constexpr DeallocationProxyArray() noexcept = default;
	~DeallocationProxyArray() noexcept = default;

	template<typename T>
	void operator << (T* const address) noexcept
	{
		t_ThreadAllocatorData.Unregister(address);
		delete[] address;
	}
};

#if (_DEBUG)
/* If exceptions are disabled, return value should be checked (no different from standard new) */
#define cinew AllocationProxy(__FILE__, __LINE__) << new CIN_ALLOCATOR_THROW_ATTRIBUTE
#define cindel DeallocationProxy() << 
#define cindelarr DeallocationProxyArray() << 
#else
/* If exceptions are disabled, return value should be checked (no different from standard new) */
#define cinew new CIN_ALLOCATOR_THROW_ATTRIBUTE
#define cindel delete
#define cindelarr delete[]
#endif