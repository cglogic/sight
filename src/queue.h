#pragma once

#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include <condition_variable>

namespace Sight {

template <typename T>
class Queue {
public:
	Queue();
	Queue(Queue&& other);
	~Queue();

	void put(const T& e);
	void put(T&& e);
	T get();

	T& first();
	void remove();

	bool ready(int64_t msec = 100) const;
	void notify() const;
	size_t size() const;

private:
	std::queue<T> mQueue;
	mutable std::mutex mLock;

	mutable std::mutex mReadyLock;
	mutable std::condition_variable mReady;

};

template <typename T>
Queue<T>::Queue() {
}

template <typename T>
Queue<T>::Queue(Queue<T>&& other) :
	mQueue(std::move(other.mQueue)) {
}

template <typename T>
Queue<T>::~Queue() {
}

template <typename T>
void Queue<T>::put(const T& e) {
	std::lock_guard<std::mutex> lg(mLock);
	mQueue.push(e);
}

template <typename T>
void Queue<T>::put(T&& e) {
	std::lock_guard<std::mutex> lg(mLock);
	mQueue.push(std::move(e));
}

template <typename T>
T Queue<T>::get() {
	std::lock_guard<std::mutex> lg(mLock);
	T e = mQueue.front();
	mQueue.pop();
	return e;
}

template <typename T>
T& Queue<T>::first() {
	std::lock_guard<std::mutex> lg(mLock);
	return mQueue.front();
}

template <typename T>
void Queue<T>::remove() {
	std::lock_guard<std::mutex> lg(mLock);
	mQueue.pop();
}

template <typename T>
bool Queue<T>::ready(int64_t msec) const {
	std::unique_lock<std::mutex> lg(mReadyLock);
	if (mReady.wait_for(lg, std::chrono::milliseconds(msec), [this]{return !mQueue.empty();})) {
		return true;
	}
	return false;
}

template <typename T>
void Queue<T>::notify() const {
	mReady.notify_one();
}

template <typename T>
size_t Queue<T>::size() const {
	std::lock_guard<std::mutex> lg(mLock);
	return mQueue.size();
}

}
