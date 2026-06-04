#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <memory>

enum class LogLevel { DEBUG, INFO, WARNING, ERROR };

class Logger {
  private:
	static std::unique_ptr<Logger> instance;
	static bool enabled;
	std::ofstream logFile;

	Logger();

  public:
	static void initialize();
	static void shutdown();
	static void setEnabled(bool enable);

	static void log(LogLevel level, const std::string& category, const std::string& message);
	static void debug(const std::string& category, const std::string& message);
	static void info(const std::string& category, const std::string& message);
	static void warning(const std::string& category, const std::string& message);
	static void error(const std::string& category, const std::string& message);

	template <typename... Args>
	static void debugf(const std::string& category, const std::string& format, Args... args);

	template <typename... Args> static void infof(const std::string& category, const std::string& format, Args... args);

	template <typename... Args>
	static void warningf(const std::string& category, const std::string& format, Args... args);

	template <typename... Args>
	static void errorf(const std::string& category, const std::string& format, Args... args);
};

// Convenience macros for different categories
#define LOG_DEBUG(cat, msg) Logger::debug(cat, msg)
#define LOG_INFO(cat, msg) Logger::info(cat, msg)
#define LOG_WARNING(cat, msg) Logger::warning(cat, msg)
#define LOG_ERROR(cat, msg) Logger::error(cat, msg)

#define LOG_DEBUGF(cat, fmt, ...) Logger::debugf(cat, fmt, __VA_ARGS__)
#define LOG_INFOF(cat, fmt, ...) Logger::infof(cat, fmt, __VA_ARGS__)
#define LOG_WARNINGF(cat, fmt, ...) Logger::warningf(cat, fmt, __VA_ARGS__)
#define LOG_ERRORF(cat, fmt, ...) Logger::errorf(cat, fmt, __VA_ARGS__)

#endif // LOGGER_H
