#ifndef __LOG_H__
#define __LOG_H__

#include<string>
#include<stdint.h>
#include<memory>
#include<list>
#include<sstream>
#include<fstream>
#include<iostream>
#include<vector>
#include <stdarg.h>
#include <map>

//成员为 m_member与接口区分命名
namespace tz116 {
	class Logger;
	class LoggerManager;

	class LogEvent {
	public:
		typedef std::shared_ptr<LogEvent> ptr;
		LogEvent(std::shared_ptr<Logger> logger, LogLevel::Level level
			, const char* file, int32_t line, uint32_t elapse
			, uint32_t thread_id, uint32_t fiber_id, uint64_t time
			, const std::string& thread_name);

		const char* getFile()const { return m_file; }
		int32_t getLine() const { return m_line; }
		uint32_t getElapse() const { return m_elapse; }
		uint32_t getThreadId() const { return m_threadId; }
		uint32_t getFiberId() const { return m_fiberId; }
		uint64_t getTime()const { return m_time; }
		const std::string& getThreadName() const { return m_threadName; }
		const std::string& getContent() const { return m_content.str(); }
		std::shared_ptr<Logger> getLogger() const { return m_logger; }
		LogLevel::Level getLevel() const { return m_level; }
		std::stringstream& getSS() { return m_content; }
		void format(const char* fmt, ...);
		void format(const char* fmt, va_list al);
	private:
		const char* m_file = nullptr; // 文件名
		int32_t m_line = 0;			// 行号
		uint32_t m_elapse = 0;		// 程序启动到现在的毫秒数
		uint32_t m_threadId = 0;	//线程ID
		uint32_t m_fiberId = 0;		//协程ID
		uint64_t m_time = 0;		//时间戳

		std::string m_threadName;	// 线程名称
		std::stringstream m_content;	// 线程消息体流
		std::shared_ptr<Logger> m_logger;	// 目标日志器
		
		LogLevel::Level m_level;	// 日志级别
	};
	// 日志事件包装类型（利用析构函数，触发日志写入）
	class LogEventWrap {
	public:
		LogEventWrap(LogEvent::ptr e);
		~LogEventWrap();
		LogEvent::ptr getEvent() const { return m_event; }
		std::stringstream& getSS();
	private:
		LogEvent::ptr m_event;
	};

	// 日志级别
	class LogLevel {
	public:
		enum Level {
			UNKNOW = 0,
			DEBUG = 1,
			INFO = 2,
			WARN = 3,
			ERROR = 4,
			FATAL = 5
		};
		static const char* ToString(LogLevel::Level level);
	};

	// 日志格式器
	class LogFormatter {
	public:
		typedef std::shared_ptr<LogFormatter> ptr;
		LogFormatter(const std::string& pattern):m_pattern(pattern){}
		// 将LogEvent 输出成字符串
		std::string format(std::shared_ptr<Logger> logger,LogLevel::Level level, LogEvent::ptr event);
	public:
		class FormatItem {
		public:
			typedef std::shared_ptr<FormatItem> ptr;
			virtual ~FormatItem(){}
			virtual void format(std::ostream& os, std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event) = 0;
		};
		void init();
		bool isError() const { return m_error; }
		const std::string getPattern() const { return m_pattern; }
	private:
		std::string m_pattern;
		std::vector<FormatItem::ptr> m_items;
		bool m_error = false;
	};
	// 日志输出地
	class LogAppender {
	public:
		typedef std::shared_ptr<LogAppender> ptr;
		typedef Spinlock MutexType;

		virtual ~LogAppender(){} 
		virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)=0;
		
		//将日志落地点输出成Yaml格式的配置
		virtual std::string toYamlString() = 0;

		void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
		LogFormatter::ptr getFormatter()const { return m_formatter; }
		LogLevel::Level getLevel() const { return m_level; }
		void setLevel(LogLevel::Level val) { m_level = val; }
	protected:
		LogLevel::Level m_level = LogLevel::DEBUG;
		bool m_hasFormatter = false;
		MutexType m_mutex;
		//日志格式器
		LogFormatter::ptr m_formatter;
	};
	// 日志器
	class Logger : public std::enable_shared_from_this<Logger> {
	public: 
		typedef std::shared_ptr<Logger> ptr;
		typedef Spinlock MutexType;

		Logger(const std::string& name = "root");
		void log(LogLevel::Level level, LogEvent::ptr event);

		void debug(LogEvent::ptr event);
		void info(LogEvent::ptr event);
		void warn(LogEvent::ptr event);
		void error(LogEvent::ptr event);
		void fatal(LogEvent::ptr event);

		void addAppender(LogAppender::ptr appender);
		void delAppender(LogAppender::ptr appender);
		void clearAppenders();

		LogLevel::Level getLevel() const { return m_level; }
		void setLevel(LogLevel::Level val) { m_level = val; }
		const std::string& getName() const { return m_name; }
		// 设置日志格式
		void setFormatter(LogFormatter::ptr val);
		void setFormatter(const std::string& val);

		LogFormatter::ptr getFormatter();
		// 转成Yaml格式的配置文本
		std::string toYamlString();
	private:
		std::string m_name;		// 日志名称
		LogLevel::Level m_level;	// 日志级别
		std::list<LogAppender::ptr> m_appenders;			// 日志目的地列表
		LogFormatter::ptr m_formatter;
		//主日志器,如果当前日志未定义，使用主日志器输出
		Logger::ptr m_root;
	};


	// 输出到控制台的Appender
	class StdoutLogAppender :public LogAppender {
	public:
		typedef std::shared_ptr<StdoutLogAppender> ptr;
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
		std::string toYamlString() override;
	};
	// 输出到文件的Appender
	class FileLogAppender :public LogAppender {
	public:
		typedef std::shared_ptr<FileLogAppender> ptr;
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
		FileLogAppender(const std::string& filename):m_filename(filename) {}
		std::string toYamlString() override;
		//重新打开文件,成功返回true
		bool reopen();
	private:
		std::string m_filename;
		std::ofstream m_filestream;
		uint64_t m_lastTime = 0;
	};
}
#endif