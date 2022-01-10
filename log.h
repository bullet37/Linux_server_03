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

//��ԱΪ m_member��ӿ���������
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
		const char* m_file = nullptr; // �ļ���
		int32_t m_line = 0;			// �к�
		uint32_t m_elapse = 0;		// �������������ڵĺ�����
		uint32_t m_threadId = 0;	//�߳�ID
		uint32_t m_fiberId = 0;		//Э��ID
		uint64_t m_time = 0;		//ʱ���

		std::string m_threadName;	// �߳�����
		std::stringstream m_content;	// �߳���Ϣ����
		std::shared_ptr<Logger> m_logger;	// Ŀ����־��
		
		LogLevel::Level m_level;	// ��־����
	};
	// ��־�¼���װ���ͣ���������������������־д�룩
	class LogEventWrap {
	public:
		LogEventWrap(LogEvent::ptr e);
		~LogEventWrap();
		LogEvent::ptr getEvent() const { return m_event; }
		std::stringstream& getSS();
	private:
		LogEvent::ptr m_event;
	};

	// ��־����
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

	// ��־��ʽ��
	class LogFormatter {
	public:
		typedef std::shared_ptr<LogFormatter> ptr;
		LogFormatter(const std::string& pattern):m_pattern(pattern){}
		// ��LogEvent ������ַ���
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
	// ��־�����
	class LogAppender {
	public:
		typedef std::shared_ptr<LogAppender> ptr;
		typedef Spinlock MutexType;

		virtual ~LogAppender(){} 
		virtual void log(std::shared_ptr<Logger> logger, LogLevel::Level level, LogEvent::ptr event)=0;
		
		//����־��ص������Yaml��ʽ������
		virtual std::string toYamlString() = 0;

		void setFormatter(LogFormatter::ptr val) { m_formatter = val; }
		LogFormatter::ptr getFormatter()const { return m_formatter; }
		LogLevel::Level getLevel() const { return m_level; }
		void setLevel(LogLevel::Level val) { m_level = val; }
	protected:
		LogLevel::Level m_level = LogLevel::DEBUG;
		bool m_hasFormatter = false;
		MutexType m_mutex;
		//��־��ʽ��
		LogFormatter::ptr m_formatter;
	};
	// ��־��
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
		// ������־��ʽ
		void setFormatter(LogFormatter::ptr val);
		void setFormatter(const std::string& val);

		LogFormatter::ptr getFormatter();
		// ת��Yaml��ʽ�������ı�
		std::string toYamlString();
	private:
		std::string m_name;		// ��־����
		LogLevel::Level m_level;	// ��־����
		std::list<LogAppender::ptr> m_appenders;			// ��־Ŀ�ĵ��б�
		LogFormatter::ptr m_formatter;
		//����־��,�����ǰ��־δ���壬ʹ������־�����
		Logger::ptr m_root;
	};


	// ���������̨��Appender
	class StdoutLogAppender :public LogAppender {
	public:
		typedef std::shared_ptr<StdoutLogAppender> ptr;
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
		std::string toYamlString() override;
	};
	// ������ļ���Appender
	class FileLogAppender :public LogAppender {
	public:
		typedef std::shared_ptr<FileLogAppender> ptr;
		void log(Logger::ptr logger, LogLevel::Level level, LogEvent::ptr event) override;
		FileLogAppender(const std::string& filename):m_filename(filename) {}
		std::string toYamlString() override;
		//���´��ļ�,�ɹ�����true
		bool reopen();
	private:
		std::string m_filename;
		std::ofstream m_filestream;
		uint64_t m_lastTime = 0;
	};
}
#endif