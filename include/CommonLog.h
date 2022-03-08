#pragma once
#include "CommonHeader.h"



#define postfix(msg)  std::string(msg).append(" ##")\
	.append(__FILE__).append(":").append(__func__)\
	.append(":").append(int2string(__LINE__))\
	.append("##").c_str()

#ifdef LOG4CPP
#include "log4wraper.h"


#define LOG_ERROR1(msg) log4wraper::instance().error(postfix(msg)) 
#define LOG_WARN1(msg)  log4wraper::instance().warn(postfix(msg))
#define LOG_INFO1(msg)  log4wraper::instance().info(postfix(msg))
#define LOG_DEBUG1(msg) log4wraper::instance().debug(postfix(msg))

#define LOG_ERROR(msg) log4wraper::instance().error(msg)
#define LOG_WARN(msg)  log4wraper::instance().warn(msg)
#define LOG_INFO(msg)  log4wraper::instance().info(msg)
#define LOG_DEBUG(msg) log4wraper::instance().debug(msg)

#elif defined(LOG4Z)
#include "log4zwraper.h"
#define LOG_ERROR1(msg) LOGE(postfix(msg)) 
#define LOG_WARN1(msg)  LOGW(postfix(msg))
#define LOG_INFO1(msg)  LOGI(postfix(msg))
#define LOG_DEBUG1(msg) LOGD(postfix(msg))

//#define LOG_ERROR(msg)   LOGE(msg)
//#define LOG_WARN(msg)    LOGW(msg)
//#define LOG_INFO(msg)    LOGI(msg)
//#define LOG_DEBUG(msg)   LOGD(msg)

#define  FORMAT_DEBUG  LOGFMTD
#define  FORMAT_INFO   LOGFMTI
#define  FORMAT_WARN   LOGFMTW
#define  FORMAT_ERROR  LOGFMTE

#define  LOG_FORMAT_GROUP 1
#else

#include "LogWriter.h"
#define LOG_ERROR1(msg) LogWriter::instance()->error(postfix(msg)) 
#define LOG_WARN1(msg)  LogWriter::instance()->warn(postfix(msg))
#define LOG_INFO1(msg)  LogWriter::instance()->info(postfix(msg))
#define LOG_DEBUG1(msg) LogWriter::instance()->debug(postfix(msg))

#define LOG_ERROR(msg) LogWriter::instance()->error(msg)
#define LOG_WARN(msg)  LogWriter::instance()->warn(msg)
#define LOG_INFO(msg)  LogWriter::instance()->info(msg)
#define LOG_DEBUG(msg) LogWriter::instance()->debug(msg)

#endif

#ifndef LOG_FORMAT_GROUP
#define LOG_FORMAT
#define MSG_BUF_LEN 260
#define  FORMAT_DEBUG(...) {char buffer[MSG_BUF_LEN]; int len = snprintf(buffer, MSG_BUF_LEN, __VA_ARGS__); \
if (len < MSG_BUF_LEN) LOG_DEBUG(buffer);\
else { char * buf = new char[len + 2]; snprintf(buf, len + 2, __VA_ARGS__); LOG_DEBUG(buf); delete[] buf;}  }

#define  FORMAT_INFO(...)  {char buffer[MSG_BUF_LEN]; int len = snprintf(buffer, MSG_BUF_LEN, __VA_ARGS__); \
if (len < MSG_BUF_LEN) LOG_INFO(buffer);\
else { char * buf = new char[len + 2]; snprintf(buf, len + 2, __VA_ARGS__); LOG_INFO(buf);delete[] buf;} }

#define  FORMAT_WARN(...)  {char buffer[MSG_BUF_LEN]; int len = snprintf(buffer, MSG_BUF_LEN, __VA_ARGS__); \
if (len < MSG_BUF_LEN) LOG_WARN(buffer);\
else { char * buf = new char[len + 2]; snprintf(buf, len + 2, __VA_ARGS__); LOG_WARN(buf);delete[] buf;} }

#define  FORMAT_ERROR(...) {char buffer[MSG_BUF_LEN]; int len = snprintf(buffer, MSG_BUF_LEN, __VA_ARGS__); \
if (len < MSG_BUF_LEN) LOG_ERROR(buffer);\
else { char * buf = new char[len + 2]; snprintf(buf, len + 2, __VA_ARGS__); LOG_ERROR(buf);delete[] buf;} }

#endif  // LOG_FORMAT_GROUP

//#define LOG_TRACE std::cout
//#define LOG_WARN  std::cout
//#define LOG_SYSERR std::cout
//#define LOG_SYSFATAL  std::cout
//#define LOG_FATAL std::cout
//#define  LOG_ERROR std::cout

