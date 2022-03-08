#pragma once
#include "log4z.h"

using namespace zsummer::log4z;

namespace LibNet
{
	class log4zwraper
	{
	public:
		log4zwraper()
		{
			ILog4zManager::getRef().setLoggerPath(LOG4Z_MAIN_LOGGER_ID, "../libNet11Log");
			ILog4zManager::getRef().setLoggerDisplay(LOG4Z_MAIN_LOGGER_ID, false);

            ILog4zManager::getRef().setLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_DEBUG);
            //ILog4zManager::getRef().setLoggerLevel(LOG4Z_MAIN_LOGGER_ID, LOG_LEVEL_ERROR);
			ILog4zManager::getRef().setLoggerLimitsize(LOG4Z_MAIN_LOGGER_ID, 100);   // 1000M bytes
			ILog4zManager::getRef().setLoggerReserveTime(LOG4Z_MAIN_LOGGER_ID, 3600);
			ILog4zManager::getRef().start();
		}
		~log4zwraper()
		{
			ILog4zManager::getRef().stop();
		}
	};

	static log4zwraper log4z;
}

