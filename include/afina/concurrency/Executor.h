#ifndef AFINA_CONCURRENCY_EXECUTOR_H
#define AFINA_CONCURRENCY_EXECUTOR_H

#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <chrono>
#include <algorithm>
#include <iostream>

namespace Afina {
namespace Concurrency {

/**
 * # Thread pool
 */
class Executor {
    enum class State {
        // Threadpool is fully operational, tasks could be added and get executed
        kRun,

        // Threadpool is on the way to be shutdown, no ned task could be added, but existing will be
        // completed as requested
        kStopping,

        // Threadppol is stopped
        kStopped
    };

public:

    Executor(std::string name, size_t low_watermark = 2, size_t hight_watermark = 8, size_t max_queue_size = 128, size_t idle_time = 2000): _low_watermark(low_watermark), _hight_watermark(hight_watermark), _max_queue_size(max_queue_size), _idle_time(idle_time)
    {
        if(_low_watermark < 0)
        {
            _low_watermark = 0;
        }
        
        if(_hight_watermark < _low_watermark)
        {
            _hight_watermark = _low_watermark;
        }
    }

    ~Executor() {
        Stop(true);
    };

    /**
     * Main function that all pool threads are running. It polls internal task queue and execute tasks
     */
    friend void perform(Executor *executor) { 
        while(true)
        {
            std::unique_lock<std::mutex> _lock(executor->mutex);
            executor->free_workers++;

			auto to_wait = std::chrono::system_clock::now() + executor->_idle_time;
            //while it is no tasks
            while(executor->tasks.empty() && executor->state == Executor::State::kRun)
            {
                auto status = executor->empty_condition.wait_until(lock, to_wait);

                //if everything ok
                if(status == std::cv_status::timeout && executor->threads.size() > executor->_low_watermark)
                {
                    DeleteThread(executor);
                    executor->free_workers--;
                    return;
                }
            }

            //if it is no tasks and stop of the pool
            if(executor->state != Executor::State::kRun && executor->tasks.empty())
            {
                DeleteThread(executor);
                if(executor->threads.empty())
                {
                    executor->stop_condition.notify_one();
                }
                return;
            }

            auto task = std::move(executor->tasks.front());
            executor->tasks.pop_front();
            executor->free_workers--;
            _lock.unlock();
            task();
            _lock.lock();
        }
    };

    friend void DeleteThread(Executor *executor) {
        auto thread_id = std::this_thread::get_id();
        auto iter = std::find_if(executor->threads.begin(), executor->threads.end(), [=](std::thread &t) { return (t.get_id() == thread_id); });
        if(iter != executor->threads.end())
        {
            iter->detach();
            executor->threads.erase(iter);
        }
    };

    void Start() {
	    std::unique_lock<std::mutex> lock(mutex);
	    for(size_t i = 0; i < _low_watermark; i++)
	    {
		    AddThread();
	    }
	    state = State::kRun;
	}

    void AddThread() {
        threads.push_back(std::thread([this] {return perform(this); }));
    };

    /**
     * Signal thread pool to stop, it will stop accepting new jobs and close threads just after each become
     * free. All enqueued jobs will be complete.
     *
     * In case if await flag is true, call won't return until all background jobs are done and all threads are stopped
     */
    void Stop(bool await = false) {
        std::unique_lock<std::mutex> lock(mutex);
        state = State::kStopping;
        empty_condition.notify_all();
        if(await && !threads.empty())
        {
            while(!threads.empty())
            {
                stop_condition.wait(lock);
            }
        }
        if (threads.empty())
        {
            state = State::kStopped;
        }
    }
;

    /**
     * Add function to be executed on the threadpool. Method returns true in case if task has been placed
     * onto execution queue, i.e scheduled for execution and false otherwise.
     *
     * That function doesn't wait for function result. Function could always be written in a way to notify caller about
     * execution finished by itself
     */
    template <typename F, typename... Types> bool Execute(F &&func, Types... args) {
        // Prepare "task"
        auto exec = std::bind(std::forward<F>(func), std::forward<Types>(args)...);

        std::unique_lock<std::mutex> lock(this->mutex);
        if (state != State::kRun || tasks.size() >= _max_queue_size) {
            return false;
        }

        // Enqueue new task
        tasks.push_back(exec);
        if(free_workers == 0 && threads.size() < _hight_watermark)
        {
            AddThread();
        }
        empty_condition.notify_one();
        return true;
    }

private:
    // No copy/move/assign allowed
    Executor(const Executor &);            // = delete;
    Executor(Executor &&);                 // = delete;
    Executor &operator=(const Executor &); // = delete;
    Executor &operator=(Executor &&);      // = delete;

    /**
     * Mutex to protect state below from concurrent modification
     */
    std::mutex mutex;

    /**
     * Conditional variable to await new data in case of empty queue
     */
    std::condition_variable empty_condition, stop_condition;

    /**
     * Vector of actual threads that perorm execution
     */
    std::vector<std::thread> threads;

    /**
     * Task queue
     */
    std::deque<std::function<void()>> tasks;

    /**
     * Flag to stop bg threads
     */
    State state;

    size_t _low_watermark, _hight_watermark, _max_queue_size, _idle_time;
    size_t free_workers = 0;
};

} // namespace Concurrency
} // namespace Afina

#endif // AFINA_CONCURRENCY_EXECUTOR_H
