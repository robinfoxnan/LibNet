#include "../include/Buffer.h"
#include "../include/SocketsApi.h"


#include <errno.h>


using namespace LibNet;

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend(8);
const size_t Buffer::kInitialSize(4096);


