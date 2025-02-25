#ifndef FP_QUEUE_HPP
#define FP_QUEUE_HPP

#include <queue>
#include <cassert>
#include <mutex>
#include <shared_mutex>

namespace fp
{


// The flowpath standard queue.
template <typename T>
class Queue
{
public:
  Queue()
    : queue_()
  { }

  ~Queue() = default;

  void enqueue(T);
  T dequeue();

  int size() const { return queue_.size(); }
  bool empty() const { return queue_.empty(); }

  std::queue<T>     queue_;
};


template <typename T>
void
Queue<T>::enqueue(T v)
{
  queue_.push(v);
}

template <typename T>
T
Queue<T>::dequeue()
{
  T temp = queue_.front();
  queue_.pop();
  return temp;
}


// The flowpath locked queue. Uses read/write locks to allow for multiple
// readers and single writer.
template <typename T>
class Locked_queue
{
public:
  Locked_queue();
  ~Locked_queue();

  void enqueue(T&);
  bool dequeue(T&);

  int size();
  bool empty() { return size(); }

private:
  std::queue<T>           queue_;
  std::shared_timed_mutex mutex_;
};


template <typename T>
Locked_queue<T>::Locked_queue()
  : queue_(), mutex_()
{ }


template <typename T>
Locked_queue<T>::~Locked_queue()
{
  while (!queue_.empty())
    queue_.pop();
}


template <typename T>
int
Locked_queue<T>::size()
{
  return queue_.size();
}


template <typename T>
void
Locked_queue<T>::enqueue(T& v)
{
  mutex_.lock();
  queue_.push(v);
  mutex_.unlock();
}


template <typename T>
bool
Locked_queue<T>::dequeue(T& v)
{
  bool ret = false;
  mutex_.lock_shared();
  if (queue_.size()) {
    v = queue_.front();
    queue_.pop();
    ret = true;
  }
  mutex_.unlock_shared();
  return ret;
}


} // end namespace fp

#endif
