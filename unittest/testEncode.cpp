#include <iostream>

#include "../samples/Detail.h"
#include "../samples/HttpClient.h"

using namespace std;
using namespace  LibNet;

void testHex()
{
	size_t n = 0x12345678abcdef;
	string str = HttpUtil::from_i_to_hex(n);
	cout << str << endl;

	int m;
	bool ret = HttpUtil::from_hex_to_i(str, 5, 5, m);
	if (ret)
	{
		printf("%X \n", m);
	}

}

int parse_url(const string & url)
{
	struct http_parser_url u;
	if (0 == http_parser_parse_url(url.c_str(), url.size(), 0, &u))
	{
		if (u.field_set & (1 << UF_PORT))
		{
			printf("port=%d \n", u.port);
		}
		else
		{
			printf("port=%d \n", 80);
		}

		if (u.field_set & (1 << UF_HOST))
		{
			string host = url.substr(u.field_data[UF_HOST].off, u.field_data[UF_HOST].len);
			cout << host << endl;
		}

		if (u.field_set & (1 << UF_PATH))
		{
			string path = url.substr(u.field_data[UF_PATH].off, u.field_data[UF_PATH].len);

			cout << path << endl;
		}

		if (u.field_set & (1 << UF_QUERY))
		{
			string path = url.substr(u.field_data[UF_QUERY].off, u.field_data[UF_QUERY].len);

			cout << path << endl;
		}

		if (u.field_set & (1 << UF_FRAGMENT))
		{
			string path = url.substr(u.field_data[UF_FRAGMENT].off, u.field_data[UF_FRAGMENT].len);

			cout << path << endl;
		}

		return 0;
	}

	return -1;
}

int main(int argc, char *argv[])
{
	//testHex();
	string url = "http://127.0.0.1:8000/test/hi?user=robin&age=50#main";
	parse_url(url);


	return 0;
}



