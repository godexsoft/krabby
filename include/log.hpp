#pragma once

#include <fmt/format.h>
#include <mutex>

namespace schwifty::logger {

#ifdef ENABLE_LOG
inline constexpr bool EnableLog = true;
#else
inline constexpr bool EnableLog = false;
#endif

enum class loglevel : int8_t { fatal = 0, warning, info, debug, trace };

class log {
private:
	log() : enable_(false), level_(loglevel::info) {}

public:
	log(const log &) = delete;
	log &operator=(const log &) = delete;
	log(log &&)                 = delete;
	log &operator=(log &&) = delete;

	inline static void enable(bool enable) { instance().enable_ = enable; }
	inline static bool enabled() { return instance().enable_; }
	inline static void level(loglevel level) { instance().level_ = level; }
	inline static loglevel level() { return instance().level_; }

	template<typename... Args>
	static void trace(const char *format, const Args &... args) {
		if constexpr (EnableLog)
			instance().dump(loglevel::trace, format, args...);
	}

	template<typename... Args>
	static void debug(const char *format, const Args &... args) {
		if constexpr (EnableLog)
			instance().dump(loglevel::debug, format, args...);
	}

	template<typename... Args>
	static void info(const char *format, const Args &... args) {
		if constexpr (EnableLog)
			instance().dump(loglevel::info, format, args...);
	}

	template<typename... Args>
	static void warn(const char *format, const Args &... args) {
		if constexpr (EnableLog)
			instance().dump(loglevel::warning, format, args...);
	}

	template<typename... Args>
	static void fatal(const char *format, const Args &... args) {
		instance().dump(loglevel::fatal, format, args...);
	}

private:
	static log &instance() {
		static log log;
		return log;
	}

	template<typename... Args>
	void dump(loglevel level, const char *format, const Args &... args) {
		if (enable_ && level_ >= level) {
			auto g = std::lock_guard(m_);
			fmt::print(stderr, "[{}] ", prefix_for(level));
			fmt::vprint(stderr, format, fmt::make_format_args(args...));
			fmt::print(stderr, "\n");
		}
	}

	inline std::string prefix_for(loglevel level) {
		switch (level) {
		case loglevel::fatal:
			return "X_X";
		case loglevel::warning:
			return "O_O";
		case loglevel::info:
			return "o.o";
		case loglevel::debug:
			return ">_<";
		case loglevel::trace:
			return "T.T";
		default:
			return "?.?";
		}
	}

	bool enable_;
	loglevel level_;
	std::mutex m_;
};
}  // namespace schwifty::logger