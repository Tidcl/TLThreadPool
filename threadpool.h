#include <thread>
#include <functional>
#include <future>
#include <vector>
#include <condition_variable>
#include <mutex>

#define THREAD_COUNT 8

//参考网络上100行线程池代码

class TLThreadpool
{
public:
	static TLThreadpool& getInstance() {
        static TLThreadpool threadPool;
        return threadPool;
	};

	template<typename F, class... Args>
    auto postTask(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>;
    void initialThreadPool();
	void waitAll();
private:
	TLThreadpool() { initialThreadPool(); };
    ~TLThreadpool();

    //std::vector<std::function<void()>> m_tasks;           //任务容器
	std::vector<std::packaged_task<void()>> m_tasks;           //packaged_task任务容器
    std::vector<std::thread> m_threads;                   //线程容器
    std::condition_variable m_condition;
    std::mutex m_mutex;
    bool m_stop;
};

TLThreadpool::~TLThreadpool()
{
	this->m_stop = true;
	this->m_condition.notify_all();
	for (std::thread& work : m_threads)
	{	
		//if(task.joinable())
		work.join();
	}
}

void TLThreadpool::waitAll()
{
	while (!this->m_tasks.empty())
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(100ms);
	}
}

void TLThreadpool::initialThreadPool()
{
	this->m_stop = false;
	for (int i = 0; i < THREAD_COUNT; i++)
	{
		m_threads.emplace_back([this]{
			while (true)
			{
				//std::function<void()> task;
				std::packaged_task<void()> task;
				{
					std::unique_lock<std::mutex> lock(this->m_mutex);
					this->m_condition.wait(lock, [this]{
						return this->m_stop || !this->m_tasks.empty();});
					if (this->m_stop && this->m_tasks.empty())//虚假唤醒
					{
						return;
					}
					task = std::move(*(this->m_tasks.rbegin()));
					this->m_tasks.pop_back();
				}
				task();
			}
			}
		);
	}
}


template<typename F, class... Args>
auto TLThreadpool::postTask(F&& f, Args&&... args) -> std::future<typename std::result_of<F(Args...)>::type>
{
	using return_type = typename std::result_of<F(Args...)>::type;
	auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args...>(args)...)
		);
	std::future<return_type> ret = task->get_future();
	{
		std::unique_lock<std::mutex> lock(this->m_mutex);
		this->m_tasks.emplace_back([task] {(*task)(); });
	}
	this->m_condition.notify_one();
	return ret;
}