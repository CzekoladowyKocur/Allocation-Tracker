#include "AllocationTracker/Allocate.h"

#define TEST 0 /* 1 or 0 */
struct UserStructure final
{
	int x, y, z;

	UserStructure() noexcept
		:
		x(0),
		y(0),
		z(0)
	{
#if (TEST == 1)
		std::cout << "Default Constructed " << typeid(*this).name() << " from thread " << std::this_thread::get_id() << '\n';
#endif
	}

	explicit UserStructure(const int x, const int y, const int z) noexcept
		:
		x(x),
		y(y),
		z(z)
	{
#if (TEST == 1)
		std::cout << "Constructed " << typeid(*this).name() << " from thread " << std::this_thread::get_id() << '\n';
#endif
	}

	~UserStructure() noexcept
	{
#if (TEST == 1)
		std::cout << "Destructed " << typeid(*this).name() << " from thread " << std::this_thread::get_id() << '\n';
#endif
	}
};

#if (TEST == 0)

void LeakSomeMemory()
{
	if (int* integer{ cinew int })
		std::cout << "Oops\n";
	else
		std::abort(); /* new failed */

	if (int* integer{ cinew int() })
		std::cout << "Oops\n";
	else
		std::abort(); /* new failed */

	if (int* integer{ cinew int(1) })
		cindel(integer);
	else
		std::abort(); /* new failed */

	if (char* cstring{ cinew char[1000]{'\0'} })
		std::cout << "Oops\n";
	else
		std::abort(); /* new failed */

	if (UserStructure * userStructure{ cinew UserStructure })
		cindel(userStructure);
	else
		std::abort(); /* new failed */

	if (UserStructure * userStructure{ cinew UserStructure() })
		std::cout << "Oops\n";
	else
		std::abort(); /* new failed */

	if (UserStructure * userStructure{ cinew UserStructure(1, 2, 3) })
		cindel(userStructure);
	else
		std::abort(); /* new failed */

	if (UserStructure * userStructureArray{ cinew UserStructure[1000] })
		std::cout << "Oops\n";
	else
		std::abort(); /* new failed */
}

int main()
{
	std::thread
		worker1{ []()
			{
				for (uint32_t i {0}; i < 2; ++i)
					LeakSomeMemory();
			} },
		worker2{ []()
			{
				for (uint32_t i {0}; i < 2; ++i)
					LeakSomeMemory();
			} },
		worker3{ []()
			{
				for (uint32_t i {0}; i < 3; ++i)
					LeakSomeMemory();
			} };

	worker1.join();
	worker2.join();
	worker3.join();

	/* We leaked */
	CinDumpThreadMemory();
	/* Bunchs of no-ops just to make sure iostream wont get clogged */
	uint32_t sum{ 0 };
	for (uint32_t i{ 0 }; i < 1000000; ++i)
	{
		auto value{ rand() };
		sum += value;
	}

	std::cout << "Costum dump function set\n";
	SetMemoryDumpCallback([](const void* allocation, const char* typeName, const std::size_t allocationSize, const char* filename, const std::size_t fileLine)
		{
			(void)allocation;
			(void)allocationSize;
			(void)filename;
			(void)fileLine;

			std::cout << "My dump: " << typeName << " was left unfreed!\n";
		});

	if (int* integer{ cinew int })
		cindel(integer);
	else
		std::abort(); /* new failed */

	if (int* integer{ cinew int })
		rand();
	else
		std::abort(); /* new failed */

	if (int* integer{ cinew int() })
		cindel(integer);
	else
		std::abort(); /* new failed */

	std::cout << "Program exit. . ., dump function will be invoked \n";
	/* automatically dump on thread destruction */
	return 0;
}
#else
void BigAllocationFunction() noexcept
{
	if (UserStructure * userStructure{ cinew UserStructure })
		cindel(userStructure);
	else
		std::abort(); /* new failed */

	if (UserStructure * userStructure{ cinew UserStructure() })
		cindel(userStructure);
	else
		std::abort(); /* new failed */

	if (UserStructure * userStructure{ cinew UserStructure(1, 2, 3) })
		cindel(userStructure);
	else
		std::abort(); /* new failed */

	if (UserStructure * userStructureArray{ cinew UserStructure[1000] })
		cindelarr(userStructureArray);
	else
		std::abort(); /* new failed */
}

void ThreadTest() noexcept
{
	std::thread
		worker1{ []()
			{
				for (uint32_t i {0}; i < 10; ++i)
					BigAllocationFunction();
			} },
		worker2{ []()
			{
				for (uint32_t i {0}; i < 20; ++i)
					BigAllocationFunction();
			} },
		worker3{ []()
			{
				for (uint32_t i {0}; i < 30; ++i)
					BigAllocationFunction();
			} };

	worker1.join();
	worker2.join();
	worker3.join();
}

#ifdef _MSC_VER
/* Use microsofts's c runtime library to detect any unfreed memory if on windows */
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>
#endif
int main()
{
#ifdef _MSC_VER
	_CrtMemState memoryState; /* Take a snapshot from main to avoid allocations during static object initialization */
	_CrtMemCheckpoint(&memoryState);
#endif
	ThreadTest();

#ifdef _MSC_VER
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
	_CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
	_CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);

	_CrtMemCheckpoint(&memoryState);
#endif
	return 0;
}
#endif