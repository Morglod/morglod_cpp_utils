#pragma once

#if _HAS_CXX20

#include <semaphore>

using binary_semaphore = std::binary_semaphore;

#else

#include <mutex>

class binary_semaphore {
	const size_t desired;
	size_t avail;
	std::mutex m;
	std::condition_variable cv;
public:
	explicit binary_semaphore(const size_t& desired = 1);
	binary_semaphore(const binary_semaphore& s);

	void acquire();
	void release();
};

#endif