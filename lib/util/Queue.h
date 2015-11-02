#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Perevod
{
	template <typename T> class Queue
	{
		std::queue<T> queue;
		mutable std::mutex mutex;
		std::condition_variable	cond;
	public:
		int limit;

		Queue();
		bool push(T data);
		T pop();
		T try_pop();
		bool empty() const;
	};
}

#include "detail/Queue.h"
