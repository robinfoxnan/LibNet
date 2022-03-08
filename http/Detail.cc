#include "Detail.h"

using namespace LibNet;

void HttpUtil::read_file(const std::string &path, std::string &out)
{
	std::ifstream fs(path, std::ios_base::binary);
	fs.seekg(0, std::ios_base::end);
	auto size = fs.tellg();
	fs.seekg(0);
	out.resize(static_cast<size_t>(size));
	fs.read(&out[0], static_cast<std::streamsize>(size));
}

// read line from 
StringPiece HttpUtil::readLine(Buffer & resBuffer)
{
	const char * end = resBuffer.findCRLF();
	if (end == nullptr)
		return StringPiece(nullptr, 0);

	size_t len = end - resBuffer.peek();

	StringPiece view = StringPiece(resBuffer.peek(), len);
	resBuffer.retrieve(len + 2);
	return view;
}


bool HttpUtil::readLine(StringPiece & strView, string & dest, const string & str)
{
	const char *ptr = std::search(strView.begin(), strView.end(), str.c_str(), str.c_str() + str.length());
	if (ptr == strView.end())
		return false;

	dest.assign(strView.begin(), ptr - strView.begin());
	strView.remove_prefix(ptr - strView.begin() + 1);
	return true;
}


bool HttpUtil::is_hex(char c, int &v)
{
	if (0x20 <= c && isdigit(c)) {
		v = c - '0';
		return true;
	}
	else if ('A' <= c && c <= 'F') {
		v = c - 'A' + 10;
		return true;
	}
	else if ('a' <= c && c <= 'f') {
		v = c - 'a' + 10;
		return true;
	}
	return false;
}

bool HttpUtil::from_hex_to_i(const std::string &s, size_t i, size_t cnt, int &val)
{
	if (i >= s.size()) { return false; }

	val = 0;
	for (; cnt; i++, cnt--)
	{
		if (!s[i])
		{
			return false;
		}

		int v = 0;
		if (is_hex(s[i], v))
		{
			val = val * 16 + v;
		}
		else
		{
			return false;
		}
	}
	return true;
}

std::string HttpUtil::from_i_to_hex(size_t n)
{
	const char *charset = "0123456789abcdef";
	//std::string ret;
	char buffer[20] = { 0 };
	int index = 19;
	do {
		index--;
		buffer[index] = charset[n & 15];
		n >>= 4;
	} while (n > 0 && index >= 0);
	return string((char*)buffer + index);
}

size_t HttpUtil::to_utf8(int code, char *buff)
{
	if (code < 0x0080) {
		buff[0] = (code & 0x7F);
		return 1;
	}
	else if (code < 0x0800) {
		buff[0] = static_cast<char>(0xC0 | ((code >> 6) & 0x1F));
		buff[1] = static_cast<char>(0x80 | (code & 0x3F));
		return 2;
	}
	else if (code < 0xD800) {
		buff[0] = static_cast<char>(0xE0 | ((code >> 12) & 0xF));
		buff[1] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
		buff[2] = static_cast<char>(0x80 | (code & 0x3F));
		return 3;
	}
	else if (code < 0xE000) { // D800 - DFFF is invalid...
		return 0;
	}
	else if (code < 0x10000) {
		buff[0] = static_cast<char>(0xE0 | ((code >> 12) & 0xF));
		buff[1] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
		buff[2] = static_cast<char>(0x80 | (code & 0x3F));
		return 3;
	}
	else if (code < 0x110000) {
		buff[0] = static_cast<char>(0xF0 | ((code >> 18) & 0x7));
		buff[1] = static_cast<char>(0x80 | ((code >> 12) & 0x3F));
		buff[2] = static_cast<char>(0x80 | ((code >> 6) & 0x3F));
		buff[3] = static_cast<char>(0x80 | (code & 0x3F));
		return 4;
	}

	// NOTREACHED
	return 0;
}

std::string HttpUtil::encode_query_param(const std::string &value)
{
	std::ostringstream escaped;
	escaped.fill('0');
	escaped << std::hex;

	for (auto c : value)
	{
		if (std::isalnum(static_cast<uint8_t>(c)) || c == '-' || c == '_' ||
			c == '.' || c == '!' || c == '~' || c == '*' || c == '\'' || c == '(' ||
			c == ')')
		{
			escaped << c;
		}
		else
		{
			escaped << std::uppercase;
			escaped << '%' << std::setw(2) << static_cast<int>(static_cast<unsigned char>(c));
			escaped << std::nouppercase;
		}
	}

	return escaped.str();
}

// result str will longer than sorce string
std::string HttpUtil::encode_url(const std::string &s)
{
	std::stringstream result;
	
	for (size_t i = 0; s[i]; i++) 
	{
		switch (s[i]) {
		case ' ':  result << "%20"; break;
		case '+':  result << "%2B"; break;
		case '\r': result << "%0D"; break;
		case '\n': result << "%0A"; break;
		case '\'': result << "%27"; break;
		case ',':  result << "%2C"; break;
	// case ':': result += "%3A"; break; // ok? probably...
		case ';':  result << "%3B"; break;
		default:
			auto c = static_cast<uint8_t>(s[i]);
			if (c >= 0x80) 
			{
				result << '%';
				char hex[4];
				auto len = snprintf(hex, sizeof(hex) - 1, "%02X", c);
				assert(len == 2);
				result << hex;
			}
			else 
			{
				result << s[i];
			}
			break;
		}
	}

	return result.str();
}

// result string is equal or shorter than source string
std::string HttpUtil::decode_url(const std::string &s, bool convert_plus_to_space)
{
	std::string result;
	result.reserve(s.size());

	for (size_t i = 0; i < s.size(); i++)
	{
		if (s[i] == '%' && i + 1 < s.size())
		{
			if (s[i + 1] == 'u')
			{
				int val = 0;
				if (from_hex_to_i(s, i + 2, 4, val)) {
					// 4 digits Unicode codes
					char buff[4];
					size_t len = to_utf8(val, buff);
					if (len > 0) { result.append(buff, len); }
					i += 5; // 'u0000'
				}
				else
				{
					result += s[i];
				}
			}
			else
			{
				int val = 0;
				if (from_hex_to_i(s, i + 1, 2, val))
				{
					// 2 digits hex codes
					result += static_cast<char>(val);
					i += 2; // '00'
				}
				else
				{
					result += s[i];
				}
			}
		}
		else if (convert_plus_to_space && s[i] == '+')
		{
			result += ' ';
		}
		else
		{
			result += s[i];
		}
	}

	return result;
}


