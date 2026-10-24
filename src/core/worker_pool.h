#ifndef WORKER_POOL_H
#define WORKER_POOL_H
#include <algorithm>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

// A few threads that run jobs in the order they came (plan faster-loading: the start-up asset parsing). The jobs touch
// no GL. Destroying the pool runs the jobs still queued, then joins the threads.
class WorkerPool {
  private:
	std::vector<std::thread> threads;
	std::deque<std::function<void()>> jobs;
	std::mutex lock;
	std::condition_variable wake;
	bool stopping = false;

	void work() {
		for (;;) {
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> held(lock);
				wake.wait(held, [this] { return stopping || !jobs.empty(); });
				if (jobs.empty())
					return;
				job = std::move(jobs.front());
				jobs.pop_front();
			}
			job();
		}
	}

  public:
	// size 0: one per core, at least one.
	explicit WorkerPool(unsigned size = 0) {
		if (size == 0)
			size = std::max(1u, std::thread::hardware_concurrency());
		for (unsigned i = 0; i < size; i++)
			threads.emplace_back([this] { work(); });
	}
	~WorkerPool() {
		{
			std::scoped_lock<std::mutex> held(lock);
			stopping = true;
		}
		wake.notify_all();
		for (std::thread& t : threads)
			t.join();
	}
	WorkerPool(const WorkerPool&) = delete;
	WorkerPool& operator=(const WorkerPool&) = delete;

	// The job's result, once a thread has run it.
	template <typename F> std::future<std::invoke_result_t<F>> Run(F job) {
		auto task = std::make_shared<std::packaged_task<std::invoke_result_t<F>()>>(std::move(job));
		std::future<std::invoke_result_t<F>> result = task->get_future();
		{
			std::scoped_lock<std::mutex> held(lock);
			jobs.emplace_back([task] { (*task)(); });
		}
		wake.notify_one();
		return result;
	}
};

#endif
