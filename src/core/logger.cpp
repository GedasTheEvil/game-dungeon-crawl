#include "logger.h"
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <iomanip>
#include <sstream>

std::unique_ptr<Logger> Logger::instance = nullptr;
bool Logger::enabled = true;

Logger::Logger() {
	logFile.open("game.log", std::ios::app);
	if (logFile.is_open()) {
		logFile << "\n=== Game Session Started ===\n";
		logFile.flush();
	}
}

void Logger::initialize() {
	if (!instance) {
		instance = std::unique_ptr<Logger>(new Logger());
	}
}

void Logger::shutdown() {
	if (instance && instance->logFile.is_open()) {
		instance->logFile << "=== Game Session Ended ===\n\n";
		instance->logFile.close();
	}
	instance.reset();
}

void Logger::setEnabled(bool enable) { enabled = enable; }

void Logger::log(LogLevel level, const std::string& category, const std::string& message) {
	if (!enabled || !instance)
		return;

	const char* levelStr;
	switch (level) {
	case LogLevel::DEBUG:
		levelStr = "DEBUG";
		break;
	case LogLevel::INFO:
		levelStr = "INFO";
		break;
	case LogLevel::WARNING:
		levelStr = "WARNING";
		break;
	case LogLevel::ERROR:
		levelStr = "ERROR";
		break;
	default:
		levelStr = "?";
		break;
	}

	// Get timestamp
	auto now = std::time(nullptr);
	auto tm = *std::localtime(&now);

	char timestamp[32];
	std::strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &tm);

	// Log to file
	if (instance->logFile.is_open()) {
		instance->logFile << "[" << timestamp << "] " << levelStr << " [" << category << "] " << message << "\n";
		instance->logFile.flush();
	}

	// Also print to console for errors
	if (level == LogLevel::ERROR) {
		std::fprintf(stderr, "[%s] %s [%s] %s\n", timestamp, levelStr, category.c_str(), message.c_str());
	}
}

void Logger::debug(const std::string& category, const std::string& message) { log(LogLevel::DEBUG, category, message); }

void Logger::info(const std::string& category, const std::string& message) { log(LogLevel::INFO, category, message); }

void Logger::warning(const std::string& category, const std::string& message) {
	log(LogLevel::WARNING, category, message);
}

void Logger::error(const std::string& category, const std::string& message) { log(LogLevel::ERROR, category, message); }

namespace {
std::string formatString(const char* format, va_list args) {
	va_list copy;
	va_copy(copy, args);
	const int size = std::vsnprintf(nullptr, 0, format, copy);
	va_end(copy);
	if (size < 0)
		return format;
	std::string out(static_cast<std::size_t>(size), '\0');
	std::vsnprintf(out.data(), out.size() + 1, format, args);
	return out;
}
} // namespace

#define LOGGER_FORMATTED(level)                                                                                        \
	va_list args;                                                                                                      \
	va_start(args, format);                                                                                            \
	std::string message = formatString(format, args);                                                                  \
	va_end(args);                                                                                                      \
	log(level, category, message);

void Logger::debugf(const std::string& category, const char* format, ...) { LOGGER_FORMATTED(LogLevel::DEBUG) }
void Logger::infof(const std::string& category, const char* format, ...) { LOGGER_FORMATTED(LogLevel::INFO) }
void Logger::warningf(const std::string& category, const char* format, ...) { LOGGER_FORMATTED(LogLevel::WARNING) }
void Logger::errorf(const std::string& category, const char* format, ...) { LOGGER_FORMATTED(LogLevel::ERROR) }
