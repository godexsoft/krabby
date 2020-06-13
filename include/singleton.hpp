#pragma once

#include <stdexcept>
#include <type_traits>

namespace schwifty::krabby {

template<typename T>
class singleton : public T {
	static T *instance_;

public:
	template<typename... Args>
	explicit singleton(Args &&... args) : T(std::forward<Args>(args)...) {
		if (instance_)
			throw std::runtime_error("Second instance not allowed for singleton");
		instance_ = this;
	}
	~singleton() { instance_ = nullptr; }
	static T &instance() { return *instance_; }
};

template<typename T>
T *singleton<T>::instance_ = nullptr;

}  // namespace schwifty::krabby