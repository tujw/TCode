#ifndef SERIALIZE_LOCK_HANDLE
#define SERIALIZE_LOCK_HANDLE
#include <mutex>
#include <stdint.h>
#include <condition_variable>

class CSerializeLock 
{
	static constexpr uint32_t BUCKET_SIZE = 16;

	class CBucket
	{
	public:
		CBucket();
		~CBucket();

		bool IsWait()const;
		void EnableWait(bool bEnable);

	private:
		bool m_bWait;
		std::condition_variable m_cond;
	};
public:

};
#endif