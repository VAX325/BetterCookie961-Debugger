#pragma once
#ifndef UTILS_H
#define UTILS_H

void Fatal(const char* const msg, ...);

class COutStreamBuf final : public std::streambuf
{
public:
	using PrintCallbackPtr_t = void (*)(void*, char);

private:
	bool m_bIsOpen;
	std::string m_Data;

	void* m_pUserData;
	PrintCallbackPtr_t m_pCallback;

protected:
	virtual int_type overflow(int_type c = traits_type::eof())
	{
		if (m_bIsOpen)
		{
			m_Data += (char)c;
			if (m_pCallback) m_pCallback(m_pUserData, c);
		}
		return c;
	}

public:
	COutStreamBuf(PrintCallbackPtr_t callback = nullptr, void* userData = nullptr) : m_bIsOpen(true)
	{
		Open(callback, userData);
	}

	virtual ~COutStreamBuf() { Close(); }

	void Open(PrintCallbackPtr_t callback = nullptr, void* userData = nullptr)
	{
		m_bIsOpen = true;
		m_pCallback = callback;
		m_pUserData = userData;
	}

	std::pair<PrintCallbackPtr_t, void*> Close()
	{
		m_bIsOpen = false;
		m_Data = "";

		std::pair<PrintCallbackPtr_t, void*> returnData(m_pCallback, m_pUserData);

		m_pCallback = nullptr;
		m_pUserData = nullptr;

		return returnData;
	}

	std::string& str() { return m_Data; }
};

class CInStreamBuf final : public std::streambuf
{
private:
	std::mutex m_Mutex;
	std::condition_variable m_CV;
	std::vector<char> m_Buffer;
	bool m_bDataReady;

protected:
	virtual int_type underflow()
	{
		std::unique_lock<std::mutex> lock(m_Mutex);
		m_CV.wait(lock, [this] { return m_bDataReady; });

		if (!m_Buffer.empty())
		{
			setg(m_Buffer.data(), m_Buffer.data(), m_Buffer.data() + m_Buffer.size());
			m_bDataReady = false;

			return traits_type::to_int_type(*gptr());
		}
		else
		{
			m_bDataReady = false;
			return traits_type::eof();
		}
	}

public:
	CInStreamBuf() : m_bDataReady(false) {}
	virtual ~CInStreamBuf() { Close(); }

	void Open()
	{
		m_bDataReady = false;
		m_CV.notify_one();
	}

	void Close()
	{
		m_bDataReady = true;
		m_CV.notify_one();
	}

	void provide_data(const std::string& data)
	{
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Buffer.assign(data.begin(), data.end());
		m_bDataReady = true;
		m_CV.notify_one();
	}

	void clear() { m_Buffer.clear(); }
};

#endif
