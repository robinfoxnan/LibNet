#ifndef __DETAIL_H
#define __DETAIL_H
#include <string>
#include <assert.h>
#include <cassert>
#include <iostream>
#include <ostream>
#include <sstream>
#include <fstream>
#include <functional>
#include <cctype>
#include <iomanip>
#include <map>
#include <vector>
#include "../include/Buffer.h"

#if defined(_MSC_VER)
#ifdef _WIN64
using ssize_t = __int64;
#else
using ssize_t = int;
#endif

/////////////////////////////////

#if _MSC_VER < 1900
#define snprintf _snprintf_s
#endif
#endif // _MSC_VER

#ifndef S_ISREG
#define S_ISREG(m) (((m)&S_IFREG) == S_IFREG)
#endif // S_ISREG

#ifndef S_ISDIR
#define S_ISDIR(m) (((m)&S_IFDIR) == S_IFDIR)
#endif // S_ISDIR

#ifndef NOMINMAX
#define NOMINMAX
#endif // NOMINMAX

#if defined(_MSC_VER)
#include <io.h>
#include <winsock2.h>

#include <wincrypt.h>
#include <ws2tcpip.h>
#endif

#ifndef WSA_FLAG_NO_HANDLE_INHERIT
#define WSA_FLAG_NO_HANDLE_INHERIT 0x80
#endif

#ifdef _MSC_VER
#pragma comment(lib, "ws2_32.lib")
#pragma comment(lib, "crypt32.lib")
#pragma comment(lib, "cryptui.lib")
#endif

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif // strcasecmp

using namespace  std;

namespace LibNet
{
	enum  Error {
		WaitCode = 0,
		WaitHeader,
		WaitBody,
		Success,
		Unknown,
		Connection,
		ConnectionReset,
		Unreachable,
		BindIPAddress,
		Read,
		Write,
		ExceedRedirectCount,
		Canceled,
		SSLConnection,
		SSLLoadingCerts,
		SSLServerVerification,
		UnsupportedMultipartBoundaryChars,
		Compression,
		ResponseHeaderError,
		Timeout
	};


	template <class T, class... Args>
	typename std::enable_if<!std::is_array<T>::value, std::unique_ptr<T>>::type
		make_unique(Args &&...args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	template <class T>
	typename std::enable_if<std::is_array<T>::value, std::unique_ptr<T>>::type
		make_unique(std::size_t n)
	{
		typedef typename std::remove_extent<T>::type RT;
		return std::unique_ptr<T>(new RT[n]);
	}


	class compareIgnoreCase
	{
	public:
		bool operator()(const std::string &s1, const std::string &s2) const
		{
			return std::lexicographical_compare(s1.begin(), s1.end(), s2.begin(), s2.end(),
				[](unsigned char c1, unsigned char c2)
			{
				return ::tolower(c1) < ::tolower(c2);
			});
		}
	};

	//using Headers = std::multimap<std::string, std::string, compareIgnoreCase>;
	using Headers = std::map<std::string, std::string>;
	using Params =  std::map<std::string, std::string>;

	class HttpUtil
	{
	public:
		static void read_file(const std::string &path, std::string &out);
		static StringPiece readLine(Buffer & resBuffer);
		static bool readLine(StringPiece & strView, string & dest, const string & str);

		static bool is_hex(char c, int &v);
		static bool from_hex_to_i(const std::string &s, size_t i, size_t cnt, int &val);
		static string from_i_to_hex(size_t n);
		static size_t to_utf8(int code, char *buff);

		static string encode_query_param(const string &value);
		static string encode_url(const string &s);
		static string decode_url(const string &s, bool convert_plus_to_space=true);
	};

	
	//////////////////////////////////////////////////////////////////////////
	using Range = std::pair<ssize_t, ssize_t>;
	using Ranges = std::vector<Range>;

	struct Request 
	{
		std::string method;
		std::string path;
		Headers headers;
		std::string body;
		std::string reqStr;
		int64_t  seqId;
		int64_t  startTm;   // used for test timeout

		std::string remote_addr;
		int remote_port = -1;

		void reset()
		{
			method = "GET";
			path = "";
			headers.clear();
			body = "";
			reqStr = "";
			seqId = 0;
		}
	};
	
	//////////////////////////////////////////////////////////////////////////////
	inline std::string error_to_string(const Error error)
	{
		switch (error) {
		case Error::WaitCode: return "Waiting Header Code";
		case Error::WaitHeader: return "Waiting Headers";
		case Error::WaitBody: return "Waiting body";
		case Error::Success: return "Success";
		case Error::Connection: return "Connection";
		case Error::ConnectionReset: return "Connection reset by remote";
		case Error::Unreachable: return "remote address and port can't connect";
		case Error::BindIPAddress: return "BindIPAddress";
		case Error::Read: return "Read";
		case Error::Write: return "Write";
		case Error::ExceedRedirectCount: return "ExceedRedirectCount";
		case Error::Canceled: return "Canceled";
		case Error::SSLConnection: return "SSLConnection";
		case Error::SSLLoadingCerts: return "SSLLoadingCerts";
		case Error::SSLServerVerification: return "SSLServerVerification";
		case Error::UnsupportedMultipartBoundaryChars:
			return "UnsupportedMultipartBoundaryChars";
		case Error::Compression: return "Compression";
		case Error::Unknown: return "Unknown";
		case Error::Timeout: return "Timeout";
		default: break;
		}

		return "Invalid";
	}

	inline Error SetError(Error code, string & info)
	{
		info = error_to_string(code);
		return code;
	}

	inline std::ostream &operator<<(std::ostream &os, const Error &obj) {
		os << to_string(obj);
		os << " (" << static_cast<std::underlying_type<Error>::type>(obj) << ')';
		return os;
	}

	//////////////////////////////////////////////////////////////////////////
	struct Response
	{
		std::string version;
		int code;
		std::string reason;
		Headers headers;
		std::string body;
		std::string location; // Redirect location

		Response() = default;
		Response(const Response &) = default;
		Response &operator=(const Response &) = default;
		Response(Response &&) = default;
		Response &operator=(Response &&) = default;
		~Response() 
		{
			
		}

		bool isSuccess()
		{
			return error == Error::Success;
		}

		string getErrorInfo()
		{
			return error_to_string(error);
		}

		void reset()
		{
			error = WaitCode;
			code = 0;
			reason = "";
			version = "";

			headers.clear();
			body = "";
			
		}

		// private members...
		size_t content_length_ = 0;
		bool is_chunked_content_provider_ = false;
		bool content_provider_success_ = false;
		Error error = WaitCode;
	};
	using ResponsePtr = std::shared_ptr<Response>;



	

}
#endif  // __DETAIL_H

