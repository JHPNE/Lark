#pragma once
#include <string>
#include <vector>
#include <memory>
#include <chrono>

enum class MessageType {
	Info = 1,
	Warning = 2,
	Error = 4
};

struct LogMessage {
	std::chrono::system_clock::time_point time;
	MessageType type;
	std::string message;
	std::string file;
	std::string caller;
	int line;

	LogMessage(MessageType type, const std::string& msg,
		const std::string& file, const std::string& caller, int line)
		: time(std::chrono::system_clock::now()), 
		type(type), 
		message(msg), 
		file(file), 
		caller(caller), 
		line(line) {}
};

class Logger {
public:
	static Logger& Get() {
		static Logger instance;
		return instance;
	}

	void Log(MessageType type, const std::string& message,
		const std::string& file = "", const std::string& caller = "", int line = 0);
	void Clear();
	const std::vector<LogMessage>& GetMessages() const { return m_messages; }
	int GetMessageFilter() const { return m_messageFilter; }
	void SetMessageFilter(int mask) { m_messageFilter = mask; }

private:
	Logger() = default;
	~Logger() = default;

	std::vector<LogMessage> m_messages;
	int m_messageFilter = static_cast<int>(MessageType::Info) | 
						static_cast<int>(MessageType::Warning) | 
						static_cast<int>(MessageType::Error);

};