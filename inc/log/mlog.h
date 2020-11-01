#ifndef M_LOG_HANDLE_
#define M_LOG_HANDLE_
#include <memory>
#include <thread>

#define DIR_SIZE 1024
#define FILE_NAME_SIZE 64

/*
* ��־�ȼ�
*/
enum Level{DEBUG, INFO, WARN, ERR, FATAL};

/*
*	��־������Ϣ���ݽṹ
*/
typedef struct{
	/*
	*	��־����·�� default: "{module_dir}\\cloudwalk\\log"
	*	��ѡ����:
	*	{module_dir} 
	*	windows : C:\\Users\\account\\AppData\\Local
	*	linux	: \tmp\
	*/
	char szDirPath[DIR_SIZE];

	/*
	*	�ļ�����ʽ	default: log_%Y%m%d.log
	*	����	log_20201011.log
	*/
	char szFileName[FILE_NAME_SIZE];

	/*
	*	��־�ȼ�
	*	default��DEBUG
	*/
	Level level;

	/*
	*	��־�������� default: 7
	*/
	int nFileDays;

	/*
	*	��־��С default: 10MB
	*/
	int nMaxFileSize;

}configure;

#ifdef _MSC_VER
#define M_LOG_API __declspec(dllexport)
#elif LINUX
#define M_LOG_API __attribute__((visibility("default")))
#else
#define M_LOG_API
#endif

#ifdef _MSC_VER
#include <windows.h>
#define THREAD_ID() GetCurrentThreadId()
#else
#include <unistd.h>
#define THREAD_ID() 200
#endif

#define LINE_SIZE 2048

class M_LOG_API CHelper
{
	int msg_size;

	char buffer[LINE_SIZE];
public:
	CHelper();

	CHelper(int line, int thread_num, const char* file, const char* func, const char* level, const char* format, ...);

	const int size() const;

	const char* data() const;
};

template<typename T>
class CSingleBase
{
protected:
	CSingleBase()
	{

	}

	class Token
	{
		public:
	};
public:
	static T& GetInstance()
	{
		Token token;
		static T base(token);
		return base;
	}
};

class M_LOG_API CMLog : public CSingleBase<CMLog> 
{
public:
	CMLog(Token token);

	~CMLog();

	/*
	*	��ʼ����־��,���Է�����ʼ������ǰ���Ѿ���ʼ��������治�ٳ�ʼ��
	*	params = nullptr ��ʾʹ��Ĭ�ϲ���
	*/
	bool init(const configure* params = nullptr);

	/*
	* �ͷ���־
	*/
	void uninit();

	/*
	*	��ȡ��־�ȼ�
	*/
	Level level()const;

	/*
	* �����־��Ϣ
	*/
	void push(const CHelper&);

protected:
	class CMLogImpl;
	CMLogImpl* m_impl; 
};


#define LOG_DEBUG(format, ...) \
		if(CMLog::GetInstance().level() <= DEBUG)  \
			CMLog::GetInstance().push(CHelper(__LINE__, THREAD_ID(), __FILE__, __FUNCTION__, "DEBUG", format, ##__VA_ARGS__)); 


#define LOG_INFO(format, ...) \
		if(CMLog::GetInstance().level() <= INFO) \
					CMLog::GetInstance().push(CHelper(__LINE__, THREAD_ID(), __FILE__, __FUNCTION__, "INFO", format, ##__VA_ARGS__)); 


#define LOG_WARN(format, ...) \
		if(CMLog::GetInstance().level() <= WARN)  \
					CMLog::GetInstance().push(CHelper(__LINE__, THREAD_ID(), __FILE__, __FUNCTION__, "WARN", format, ##__VA_ARGS__)); 


#define LOG_ERROR(format, ...) \
		if(CMLog::GetInstance().level() <= ERR)  \
					CMLog::GetInstance().push(CHelper(__LINE__, THREAD_ID(), __FILE__, __FUNCTION__, "ERROR", format, ##__VA_ARGS__)); 

#define LOG_FATAL(format, ...) \
		if(CMLog::GetInstance().level() <= FATAL) \
					CMLog::GetInstance().push(CHelper(__LINE__, THREAD_ID(), __FILE__, __FUNCTION__, "FATAL", format, ##__VA_ARGS__));


#endif
