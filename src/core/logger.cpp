#include "logger.h"
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

	const char* levelStr = "";
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

template <typename... Args> static std::string formatString(const std::string& format, Args... args) {
	int size = std::snprintf(nullptr, 0, format.c_str(), args...) + 1;
	if (size <= 0)
		return format;

	std::unique_ptr<char[]> buf(new char[size]);
	std::snprintf(buf.get(), size, format.c_str(), args...);
	return std::string(buf.get(), buf.get() + size - 1);
}

template <typename... Args> void Logger::debugf(const std::string& category, const std::string& format, Args... args) {
	debug(category, formatString(format, args...));
}

template <typename... Args> void Logger::infof(const std::string& category, const std::string& format, Args... args) {
	info(category, formatString(format, args...));
}

template <typename... Args>
void Logger::warningf(const std::string& category, const std::string& format, Args... args) {
	warning(category, formatString(format, args...));
}

template <typename... Args> void Logger::errorf(const std::string& category, const std::string& format, Args... args) {
	error(category, formatString(format, args...));
}

// Explicit template instantiations for common types
template void Logger::debugf<>(const std::string&, const std::string&);
template void Logger::infof<>(const std::string&, const std::string&);
template void Logger::warningf<>(const std::string&, const std::string&);
template void Logger::errorf<>(const std::string&, const std::string&);

template void Logger::debugf<int>(const std::string&, const std::string&, int);
template void Logger::infof<int>(const std::string&, const std::string&, int);
template void Logger::warningf<int>(const std::string&, const std::string&, int);
template void Logger::errorf<int>(const std::string&, const std::string&, int);

template void Logger::debugf<int, int>(const std::string&, const std::string&, int, int);
template void Logger::infof<int, int>(const std::string&, const std::string&, int, int);
template void Logger::warningf<int, int>(const std::string&, const std::string&, int, int);
template void Logger::errorf<int, int>(const std::string&, const std::string&, int, int);

template void Logger::debugf<const char*>(const std::string&, const std::string&, const char*);
template void Logger::infof<const char*>(const std::string&, const std::string&, const char*);
template void Logger::warningf<const char*>(const std::string&, const std::string&, const char*);
template void Logger::errorf<const char*>(const std::string&, const std::string&, const char*);

// Additional instantiations for textures.cpp
template void Logger::infof<const char*, int, int>(const std::string&, const std::string&, const char*, int, int);
template void Logger::infof<unsigned long>(const std::string&, const std::string&, unsigned long);
template void Logger::infof<const char*, int>(const std::string&, const std::string&, const char*, int);
template void Logger::warningf<const char*, unsigned short>(const std::string&, const std::string&, const char*,
															unsigned short);
template void Logger::infof<unsigned int>(const std::string&, const std::string&, unsigned int);
template void Logger::errorf<const char*, const char*>(const std::string&, const std::string&, const char*,
													   const char*);

// Additional instantiations for sound.cpp
template void Logger::debugf<void*>(const std::string&, const std::string&, void*);
template void Logger::errorf<char*>(const std::string&, const std::string&, char*);

// Additional instantiations for monster.cpp (loading models with char[] arrays decaying to char*)
template void Logger::infof<char*>(const std::string&, const std::string&, char*);

// Additional instantiations for trap.cpp (debugText with 4 floats)
template void Logger::debugf<float, float, float, float>(const std::string&, const std::string&, float, float, float,
														 float);
