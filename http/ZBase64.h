#pragma once


#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace std;

class ZBase64
{
public:

	static string Encode(const char* Data, int DataByte);
	static string Decode(const char* Data, int DataByte, int& OutByte);


	static string readFileIntoString(const char * filename);
	static bool writeToFile(const char * filename, const string & content);
};

