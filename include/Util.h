/**
  * utils for dev.
  * author : robin
  * date :  2021-11-19 
  * version:  1.0
  * copyright (C) 2008-2021  all rights reserved.
  */
#pragma once
#include <chrono>
#include <random>
#include <string>
#include <inttypes.h>

#include <cstdlib>
#include <cstdio>
#include <sstream>
#include <thread>
#include <iostream>
#include <map>

#include <string.h>

using namespace std;


#if defined(_WIN32) || defined(_WIN64)

#include <process.h>
#include <windows.h>
#include <psapi.h>
#pragma comment(lib,"psapi.lib")
#define  getProcessId() _getpid()

#else   //linux below

#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
//#include <linux/if_link.h>
#define  getProcessId() getpid()

#endif

namespace LibNet
{
 
    class Utils
    {
	public:
		/*
		const char * vec[8] =
	{
		"30 s",
		"1d 2h 3m 4s",
		"1d",
		"2 h ",
		"2 h 3 m",
		"1d 2 h 3 m",
		"3 m 4 s",
		"3m 1d"  // wrong
	};
		*/

		// parse date span string to seconds
		static uint64_t parseReserveTime(const string & str)
		{
			uint64_t  delta = 0;
			string timeStr = str;
			if (str.empty())
				return 0;

			auto pos = str.find("d");
			if (pos != string::npos)
			{
				string days = str.substr(0, pos);
				delta += std::atoi(days.c_str()) * 24 * 3600;
				timeStr = str.substr(pos + 1);
			}

			pos = str.find("h");
			if (pos != string::npos)
			{
				string hours = timeStr.substr(0, pos);
				delta += std::atoi(hours.c_str()) * 3600;
				timeStr = str.substr(pos + 1);
			}

			pos = str.find("m");
			if (pos != string::npos)
			{
				string minu = timeStr.substr(0, pos);
				delta += std::atoi(minu.c_str()) * 60;
				timeStr = str.substr(pos + 1);
			}


			delta += std::atoi(timeStr.c_str());
			timeStr = str.substr(pos + 1);


			return delta;

		}

		// static uint32_t getThreadId()
		// {
		// 	auto tid = std::this_thread::get_id();
		// 	_Thrd_id_t t = *(_Thrd_id_t*)&tid;
		// 	return t;
		// }
		static inline void splitLine(string str, char mid, string &name, string &val)
		{
			std::stringstream ss(str);
			std::getline(ss, name, mid);
			std::getline(ss, val, mid);
		}
		// "refer_guid=%s;trip_guid=%s
		static void parseString(string & data, char line, char mid, std::map<string, string> & mapStr)
		{
			std::stringstream ss(data);
			std::string item;

			string name;
			string val;
		
			while (std::getline(ss, item, line))
			{
				splitLine(item, mid, name, val);
				mapStr[name] = val;
			}
		}

		static void sleepFor(int msecs)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(msecs));
		}

		static uint64_t getThreadId1()
		{
			std::ostringstream oss;
			oss << std::this_thread::get_id();
			std::string stid = oss.str();
			uint64_t tid = std::stoull(stid);
			return tid;
		}

		static string getThreadIdStr()
		{
			std::ostringstream oss;
			oss << std::this_thread::get_id();
			std::string stid = oss.str();
			return stid;
		}

        static inline int64_t getSecNow()
        {
			// tp.time_since_epoch().count()  windows timer  granularity is 100ns, while linux can be 1 ns
            auto time_now = std::chrono::system_clock::now();
            auto duration_in_ms = std::chrono::duration_cast<std::chrono::seconds>(time_now.time_since_epoch());
            return duration_in_ms.count();
        }

		static inline int64_t getmSecNow()
		{
			auto time_now = std::chrono::system_clock::now();
			auto duration_in_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_now.time_since_epoch());
			return duration_in_ms.count();
		}


		static inline int64_t getnSecNow()
		{
			auto time_now = std::chrono::system_clock::now();
			auto duration_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(time_now.time_since_epoch());
			return duration_in_ns.count();
		}

		static inline std::string getSecNowStr()
        {
            return std::to_string(getSecNow());
        }
		
        
        static  std::string hashString()
        {
            char dx[] = "0123456789abcdef";
            char str[17]={};
            
            static std::default_random_engine e((unsigned int)getmSecNow());
            static std::uniform_int_distribution<unsigned> u(0, 15);
            for (int i = 0; i < 16; i++)
            {
                str[i] = dx[u(e)];
            }
            return str;
        }

        static inline int64_t hash(std::string str)
        {
            unsigned int seed = 131;
            unsigned int hash = 0;
            for(auto c : str)
            {
                hash = hash * seed  + c;
            }
            return (hash & 0x7FFFFFFF);
        }

#if defined(WIN32) ||(_WIN64)
		// 这里是系统内存的占用率0~100
		static inline double getMemoryUsgae()
		{
			MEMORYSTATUS ms;
			::GlobalMemoryStatus(&ms);
			return ms.dwMemoryLoad;
		}

		// 返回占用了MB
		static inline double getAppMemoryUsgae()
		{
			// don't close handle
			HANDLE handle = GetCurrentProcess();
			PROCESS_MEMORY_COUNTERS pmc;
			GetProcessMemoryInfo(handle, &pmc, sizeof(pmc));
			return pmc.WorkingSetSize * 1.0 / 1000000;
		}

		//CPU使用率；使用方法：直接调用getCpuUsage（）函数
		//原理：记录一定时间内CPU的空闲时间和繁忙时间，然后计算得出
		static inline __int64 CompareFileTime(FILETIME time1, FILETIME time2)
		{
			__int64 a = (__int64)time1.dwHighDateTime << 32 | time1.dwLowDateTime;
			__int64 b = (__int64)time2.dwHighDateTime << 32 | time2.dwLowDateTime;

			return   (b - a);
		}

		static double getCpuUsage()
		{
			static bool bInit = false;
			static FILETIME pre_idleTime;
			static FILETIME pre_kernelTime;
			static FILETIME pre_userTime;

			FILETIME idleTime;//空闲时间 
			FILETIME kernelTime;//核心态时间 
			FILETIME userTime;//用户态时间 

			bool res = GetSystemTimes(&idleTime, &kernelTime, &userTime);


			__int64 idle = CompareFileTime(pre_idleTime, idleTime);
			__int64 kernel = CompareFileTime(pre_kernelTime, kernelTime);
			__int64 user = CompareFileTime(pre_userTime, userTime);

			pre_userTime = userTime;
			pre_kernelTime = kernelTime;
			pre_idleTime = idleTime;

			if (bInit == false)
			{
				bInit = true;
				return 0;
			}

			if ((kernel + user) == 0)
				return 0;

			double cpu_occupancy_rate = double(kernel + user - idle) * 1.0 / double(kernel + user);
			//（总的时间 - 空闲时间）/ 总的时间 = 占用CPU时间的比率，即占用率

			//int cpu_idle_rate = idle * 100.0 / (kernel + user);
			//空闲时间 / 总的时间 = 闲置CPU时间的比率，即闲置率 

			//int cpu_kernel_rate = kernel * 100 / (kernel + user);
			//核心态时间 / 总的时间 = 核心态占用的比率 

			//int cpu_user_rate = user * 100 / (kernel + user);
			//用户态时间 / 总的时间 = 用户态占用的比率 

			/*cout << "CPU占用率：" << cpu_occupancy_rate << "%" << endl
				<< "CPU闲置率：" << cpu_idle_rate << "%" << endl
				<< "核心态占比率：" << cpu_kernel_rate << "%" << endl
				<< "用户态占比率：" << cpu_user_rate << "%" << endl << endl;*/

			// 返回小数
			return cpu_occupancy_rate;
		}

#else  // linux
		static std::string getIPAddress()
		{
			string ip;
		
			struct ifaddrs *ifaddr, *ifa;
			if (getifaddrs(&ifaddr) == -1)
			{
				perror("getifaddrs");
				return "127.0.0.1";
			}
			for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
			{
				if (ifa->ifa_addr == NULL)
					continue;
				if (ifa->ifa_addr->sa_family == AF_INET)
				{
					std::string s(inet_ntoa(((struct sockaddr_in *)ifa->ifa_addr)->sin_addr));
					if (s != "127.0.0.1")
						ip.append(s).append("#");
				}
			}
			freeifaddrs(ifaddr);
			if (ip.size() > 1)
				ip.erase(ip.end() - 1);
			return ip;
		}

	////////////////////////////////////////////////////////////////////
	// /proc/meminfo
	static int getProcInfo(const char * name, std::map<string, int> & keyValues) 
	{
		FILE *fd = fopen(name, "r");
		char key_value[1024];
		string key;
		int value = -1;
		stringstream ss;
		int count =0;
		while(fgets(key_value, sizeof(key_value), fd)) 
		{
			ss.str("");
			ss.clear();
			ss << key_value;
			ss >> key >> value;
			count ++;
			keyValues.insert(std::make_pair(key, value));
			//std:cout << key << " " << value << std::endl;
		}
		fclose(fd);
		return count;
	}

	// 0.15ms
	// https://blog.csdn.net/tugouxp/article/details/119836969
	static double getAppMemoryUsgae()
	{
		std::map<string, int> mapValues;

		string str = "/proc/" + std::to_string(getpid())+ "/status";
		//string str = "/proc/meminfo";
		getProcInfo(str.c_str(), mapValues);

		u_long mem_used = mapValues["VmSize:"]; 

		// size in MB
		return mem_used * 1.0 / (1000000.0);

	}
	static double getMemoryUsgae()
	{
		std::map<string, int> mapValues;

		//string str = "/proc/" + std::to_string(getpid())+ "/status";
		string str = "/proc/meminfo";
		getProcInfo(str.c_str(), mapValues);

		
		u_long mem_used;
		u_long mem_cached;

		u_long mem_total = mapValues["MemTotal:"];
		u_long mem_free = mapValues["MemFree:"];	 
		u_long mem_buffers= mapValues["Buffers:"];

		u_long page_cache = mapValues["Cached:"];
		u_long slab_reclaimable = mapValues["SReclaimable:"];
		mem_cached = page_cache + slab_reclaimable;


		mem_used = mem_total - mem_free - mem_cached - mem_buffers;
	
		double t = mem_used * 100.0 / mem_total;
		//cout << t << endl;
		return t;
	}

	///////////////////////////////////////////////////////////////////
	// robin test2021-11-19
	typedef struct _SysCPUInfo
	{
		char name[260];
		uint64_t user;
		uint64_t  nic;
		uint64_t system;
		uint64_t idle;
	}SysCPUInfo;

	static void GetHostCPUInfo(SysCPUInfo *cpuinfo)
	{

		FILE 	*fd;
		char	buff[256];
		memset(buff, '\0', 256);

		fd = fopen("/proc/stat", "r");
		fgets(buff, sizeof(buff), fd);

		sscanf(buff, "%s %lu %lu %lu %lu",
			(char *)&cpuinfo->name,
			&cpuinfo->user,
			&cpuinfo->nic,
			&cpuinfo->system,
			&cpuinfo->idle);
		fclose(fd);

	}
	static double getCpuUsage()
	{
		static uint64_t old_user;
		static uint64_t old_system;
		static uint64_t old_nic;
		static uint64_t old_idle;

		static bool bInit = false;
		
		SysCPUInfo  curInfo;
		GetHostCPUInfo(&curInfo);


		//if  ( unlikely(bInit == false)  )
		if  ( bInit == false )
		{
			old_user = curInfo.user;
			old_system = curInfo.system;
			old_idle = curInfo.idle;
			old_nic = curInfo.nic;

			bInit = true;
			return 0;
		}
		
		float cpu_use = 0.0;
		uint64_t old_CPU_Time = (uint64_t)(old_user + old_nic + old_system + old_idle);
		uint64_t new_CPU_Time = (uint64_t)(curInfo.user + curInfo.nic + curInfo.system + curInfo.idle);
		
		uint64_t usr_Time_Diff = (uint64_t)(curInfo.user - old_user);
		uint64_t sys_Time_Diff = (uint64_t)(curInfo.system - old_system);
		uint64_t nic_Time_Diff = (uint64_t)(curInfo.nic - old_nic);

		if ((new_CPU_Time - old_CPU_Time) != 0)
			cpu_use = (double)1.0 * (usr_Time_Diff + sys_Time_Diff + nic_Time_Diff) / (new_CPU_Time - old_CPU_Time);
		else
			cpu_use = 0.0;

		old_user = curInfo.user;
		old_system = curInfo.system;
		old_idle = curInfo.idle;
		old_nic = curInfo.nic;
		//cout << cpu_use << endl;
		return cpu_use;
	}
	////////////////////////////////////////
		static inline std::string execute_command(const char *cmd)
		{
			if (cmd == NULL)
			{
				return "";
			}

			FILE *fp = popen(cmd, "r");
			if (NULL == fp )
			{
				return "";
			}

			char buf[BUFSIZ] = {};
			int i;
			std::string result;
			while ((i = fread(buf, sizeof(char), BUFSIZ, fp)) > 0)
			{

				result.append(buf);
				memset(buf, 0, BUFSIZ);
				if (i != BUFSIZ)
				{
					break;
				}
			}
			pclose(fp);
			int index = result.find_last_of("\r\n");
			if (index > 0)
			{
				return result.substr(0, index);
			}
			index = result.find_last_of("\n");
			if (index > 0)
			{
				return result.substr(0, index);
			}
			return result;
		}

		

		static double cpuRate()
		{
			std::string cpu = execute_command(
				"top -bn 2  |awk '/Cpu/{split($0,a,\",\");print a[4]}'|awk '{split($0,a,\"%\");print a[1]}'|sed '1d'|awk '{print $1}'|awk '{sum+=$1} END {print (100-sum/NR)/100}'");
			return atof(cpu.c_str());
		}

		static double memRate()
		{
			std::string mem = execute_command(
				"free -m|sed '3,$d'|awk '{printf $0}'|awk '{if($0~/buff/)print ($8)*100/$7;else print $8*100/$7}'");
			return atof(mem.c_str());
		}
#endif // end of linux

//	static void test()
//	{
//		cout << oneapm::Utils::getMemoryUsgae() << endl;

//		// init once
//		cout << oneapm::Utils::getCpuUsage() << endl;
//		for (int i=0; i<10; i++)
//		{
//			std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//			cout << oneapm::Utils::getCpuUsage() << endl;
//		}
		
//	}
	}; // class utils
    
} // namespace oneapm
