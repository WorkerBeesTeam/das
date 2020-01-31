#ifndef DAS_THREAD_IFACE_H
#define DAS_THREAD_IFACE_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <QPair>

namespace Das {

template<typename DataPackT, typename T>
struct thread_iface {
    thread_iface(void(T::*thread_func)(), T* obj) : thread_(thread_func, obj) {}
    std::thread thread_;
};

template<typename DataPackT>
class thread_simple : protected thread_iface<DataPackT, thread_simple<DataPackT>> {
public:
    thread_simple() : thread_iface<DataPackT, thread_simple>(&thread_simple<DataPackT>::thread_func, this) {}
protected:
    virtual void init() {}
    virtual void process_data(DataPackT&) = 0;
    virtual void loop_iteration() {}
    virtual void stoped() {}
private:
    void thread_func()
    {
        init();

        std::unique_lock lock(this->mutex_, std::defer_lock);
        while (true)
        {
            lock.lock();
            this->cond_.wait(lock, [this](){ return this->data_queue_.size(); });
            std::queue<DataPackT> data_queue = std::move(this->data_queue_);
            lock.unlock();

            while (data_queue.size())
            {
                DataPackT data{std::move(data_queue.front())};
                data_queue.pop();

                process_data(data);
            }

            loop_iteration();
        }
        stoped();
    }
};

} // namespace Das

#endif // DAS_THREAD_IFACE_H
