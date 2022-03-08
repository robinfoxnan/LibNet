#ifndef LibNet_NET_BUFFER_H
#define LibNet_NET_BUFFER_H

#include "CommonHeader.h"
#include "StringPiece.h"
#include "SocketsApi.h"
//#include <unistd.h>  // ssize_t

namespace LibNet
{


/// A buffer class modeled after org.jboss.netty.buffer.ChannelBuffer
///
/// @code
/// +-------------------+------------------+------------------+
/// | prependable bytes |  readable bytes  |  writable bytes  |
/// |                   |     (CONTENT)    |                  |
/// +-------------------+------------------+------------------+
/// |                   |                  |                  |
/// 0      <=      readerIndex   <=   writerIndex    <=     size
/// @endcode
class Buffer
{
 public:
  static const size_t kCheapPrepend;
  static const size_t kInitialSize;

  explicit Buffer(size_t initialSize = kInitialSize)
    : buffer_(kCheapPrepend + initialSize), readerIndex_(kCheapPrepend), writerIndex_(kCheapPrepend)
  {
    assert(readableBytes() == 0);
    assert(writableBytes() == initialSize);
    assert(prependableBytes() == kCheapPrepend);
  }

  // implicit copy-ctor, move-ctor, dtor and assignment are fine
  // NOTE: implicit move-ctor is added in g++ 4.6

  inline void clear()
  {
    //buffer_.clear();
	//buffer_.resize(kCheapPrepend + Buffer::kInitialSize);
    readerIndex_ = kCheapPrepend;
    writerIndex_= kCheapPrepend;
	
  }

  inline size_t size()
  {
	  return buffer_.size() - kCheapPrepend;
  }

  inline bool isNeedMove()
  {
	  size_t sz = buffer_.size() - kCheapPrepend;
	  size_t gap_len =  readerIndex_ - kCheapPrepend;
	  if (gap_len > 0)
	  {
		  if (gap_len > (sz / 4))
			  return true;
	  }
	  return false;
  }

  void checkToMove()
  {
	  if (isNeedMove())
		  moveLeft();
  }


  void moveLeft()
  {
	  if (readableBytes() == 0)
	  {
		  clear();
	  }
	  else if (kCheapPrepend < readerIndex_)
	  {
		  assert(kCheapPrepend < readerIndex_);
		  size_t readable = readableBytes();
		  std::copy(begin() + readerIndex_,
			  begin() + writerIndex_,
			  begin() + kCheapPrepend);
		  readerIndex_ = kCheapPrepend;
		  writerIndex_ = readerIndex_ + readable;
		  assert(readable == readableBytes());
	  }
	
  }

  void state()
  {
	  printf("size=%zu, readIndex=%zu, writeIndex=%zu, datasize=%zu\n", buffer_.size(), readerIndex_, writerIndex_, readableBytes());
  }

  void  setAppend(size_t len)
  {
	  writerIndex_ += len;
	  assert(writerIndex_ <= buffer_.size());
  }
 

  void addRoom(size_t len )
  {
	  if (writableBytes() < len)
	  {
		  reserve(writerIndex_ + len);
	  }
  }

  void swap(Buffer& rhs)
  {
    buffer_.swap(rhs.buffer_);
    std::swap(readerIndex_, rhs.readerIndex_);
    std::swap(writerIndex_, rhs.writerIndex_);
  }

  inline size_t readableBytes() const
  { return writerIndex_ - readerIndex_; }

  inline size_t writableBytes() const
  { return buffer_.size() - writerIndex_; }

  inline size_t prependableBytes() const
  { return readerIndex_; }

  inline char * data()
  {
	  return (char *)peek();
  }

  inline const char* peek() const
  { return begin() + readerIndex_; }

  inline const char* peekEnd() const
  {
	  return begin() + writerIndex_;
  }

  inline const char* findCRLF() const
  {
    // FIXME: replace with memmem()?
    const char* crlf = std::search(peek(), beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? nullptr : crlf;
  }

  const char* findCRLF(const char* start) const
  {
    assert(peek() <= start);
    assert(start <= beginWrite());
    // FIXME: replace with memmem()?
    const char* crlf = std::search(start, beginWrite(), kCRLF, kCRLF+2);
    return crlf == beginWrite() ? NULL : crlf;
  }

  const char* findEOL() const
  {
    const void* eol = memchr(peek(), '\n', readableBytes());
    return static_cast<const char*>(eol);
  }

  const char* findEOL(const char* start) const
  {
    assert(peek() <= start);
    assert(start <= beginWrite());
    const void* eol = memchr(start, '\n', beginWrite() - start);
    return static_cast<const char*>(eol);
  }

  // retrieve returns void, to prevent
  // string str(retrieve(readableBytes()), readableBytes());
  // the evaluation of two functions are unspecified
  void retrieve(size_t len)
  {
    assert(len <= readableBytes());
    if (len < readableBytes())
    {
      readerIndex_ += len;
    }
    else
    {
      retrieveAll();
    }
  }

  void retrieveUntil(const char* end)
  {
    assert(peek() <= end);
    assert(end <= beginWrite());
    retrieve(end - peek());
  }

  void retrieveInt64()
  {
    retrieve(sizeof(int64_t));
  }

  void retrieveInt32()
  {
    retrieve(sizeof(int32_t));
  }

  void retrieveInt16()
  {
    retrieve(sizeof(int16_t));
  }

  void retrieve2B()
  {
	  retrieve(2);
  }

  void retrieveInt8()
  {
    retrieve(sizeof(int8_t));
  }

  void retrieveAll()
  {
	 // printf("all data clear\n");
    readerIndex_ = kCheapPrepend;
    writerIndex_ = kCheapPrepend;
  }

  string retrieveAllAsString()
  {
    return retrieveAsString(readableBytes());
  }

  string retrieveAsString(size_t len)
  {
    assert(len <= readableBytes());
    string result(peek(), len);
    retrieve(len);
    return result;
  }

  StringPiece toStringPiece() const
  {
    return StringPiece(peek(), static_cast<int>(readableBytes()));
  }

  void append(const StringPiece& str)
  {
    append(str.data(), str.size());
  }

  void append(const char* /*restrict*/ data, size_t len)
  {
    ensureWritableBytes(len);
    std::copy(data, data+len, beginWrite());
    hasWritten(len);
  }

  void append(const void* /*restrict*/ data, size_t len)
  {
    append(static_cast<const char*>(data), len);
  }

  void ensureWritableBytes(size_t len)
  {
    if (writableBytes() < len)
    {
      makeSpace(len);
    }
    assert(writableBytes() >= len);
  }

  char* beginWrite()
  { return begin() + writerIndex_; }

  const char* beginWrite() const
  { return begin() + writerIndex_; }

  void hasWritten(size_t len)
  {
    assert(len <= writableBytes());
    writerIndex_ += len;
  }

  void unwrite(size_t len)
  {
    assert(len <= readableBytes());
    writerIndex_ -= len;
  }

  ///
  /// Append int64_t using network endian
  ///
  void appendInt64(int64_t x)
  {
    int64_t be64 = socketsApi::hostToNetwork64(x);
    append(&be64, sizeof be64);
  }

  ///
  /// Append int32_t using network endian
  ///
  void appendInt32(int32_t x)
  {
    int32_t be32 = socketsApi::hostToNetwork32(x);
    append(&be32, sizeof be32);
  }

  void appendInt16(int16_t x)
  {
    int16_t be16 = socketsApi::hostToNetwork16(x);
    append(&be16, sizeof be16);
  }

  void appendInt8(int8_t x)
  {
    append(&x, sizeof x);
  }

  ///
  /// Read int64_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int64_t readInt64()
  {
    int64_t result = peekInt64();
    retrieveInt64();
    return result;
  }

  ///
  /// Read int32_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int32_t readInt32()
  {
    int32_t result = peekInt32();
    retrieveInt32();
    return result;
  }

  int16_t readInt16()
  {
    int16_t result = peekInt16();
    retrieveInt16();
    return result;
  }

  int8_t readInt8()
  {
    int8_t result = peekInt8();
    retrieveInt8();
    return result;
  }

  ///
  /// Peek int64_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int64_t)
  int64_t peekInt64() const
  {
    assert(readableBytes() >= sizeof(int64_t));
    int64_t be64 = 0;
    ::memcpy(&be64, peek(), sizeof be64);
    return socketsApi::networkToHost64(be64);
  }

  ///
  /// Peek int32_t from network endian
  ///
  /// Require: buf->readableBytes() >= sizeof(int32_t)
  int32_t peekInt32() const
  {
    assert(readableBytes() >= sizeof(int32_t));
    int32_t be32 = 0;
    ::memcpy(&be32, peek(), sizeof be32);
    return socketsApi::networkToHost32(be32);
  }

  int16_t peekInt16() const
  {
    assert(readableBytes() >= sizeof(int16_t));
    int16_t be16 = 0;
    ::memcpy(&be16, peek(), sizeof be16);
    return socketsApi::networkToHost16(be16);
  }

  int8_t peekInt8() const
  {
    assert(readableBytes() >= sizeof(int8_t));
    int8_t x = *peek();
    return x;
  }

  ///
  /// Prepend int64_t using network endian
  ///
  void prependInt64(int64_t x)
  {
    int64_t be64 = socketsApi::hostToNetwork64(x);
    prepend(&be64, sizeof be64);
  }

  ///
  /// Prepend int32_t using network endian
  ///
  void prependInt32(int32_t x)
  {
    int32_t be32 = socketsApi::hostToNetwork32(x);
    prepend(&be32, sizeof be32);
  }

  void prependInt16(int16_t x)
  {
    int16_t be16 = socketsApi::hostToNetwork16(x);
    prepend(&be16, sizeof be16);
  }

  void prependInt8(int8_t x)
  {
    prepend(&x, sizeof x);
  }

  void prepend(const void* /*restrict*/ data, size_t len)
  {
    assert(len <= prependableBytes());
    readerIndex_ -= len;
    const char* d = static_cast<const char*>(data);
    std::copy(d, d+len, begin()+readerIndex_);
  }

  void shrink(size_t reserve)
  {
    // FIXME: use vector::shrink_to_fit() in C++ 11 if possible.
    Buffer other;
    other.ensureWritableBytes(readableBytes()+reserve);
    other.append(toStringPiece());
    swap(other);
  }

  size_t internalCapacity() const
  {
    return buffer_.capacity();
  }

  /// Read data directly into buffer.
  ///
  /// It may implement with readv(2)
  /// @return result of read(2), @c errno is saved
  //ssize_t readFd(int fd, int* savedErrno);



  char* begin()
  { return &*buffer_.begin(); }

  const char* begin() const
  { return &*buffer_.begin(); }
 private:
  void reserve(size_t n) {

	  size_t sz = 64;
	  if (sz < buffer_.capacity())
		  sz = buffer_.capacity();
	  while (sz < n)
	  {
		  if (sz > 10 * 1024 * 1024)  // 10MB
		  {
			  sz = sz + 1024 * 1024;
		  }
		  else
		  {
			  sz = sz * 2;
		  }
	  }
	  buffer_.resize(n);

	  //cout <<" reserve : "<< size() << "," << capacity() << endl;
  }

  void makeSpace(size_t len)
  {
	  if (writableBytes() < len)
	  {
		  reserve(writerIndex_ + len);
	  }
  }
  //void makeSpace(size_t len)
  //{
  //  if (writableBytes() + prependableBytes() < len + kCheapPrepend)
  //  {
  //    // FIXME: move readable data
  //    buffer_.resize(writerIndex_+len);
  //  }
  //  else
  //  {
  //    // move readable data to the front, make space inside buffer
  //    assert(kCheapPrepend < readerIndex_);
  //    size_t readable = readableBytes();
  //    std::copy(begin()+readerIndex_,
  //              begin()+writerIndex_,
  //              begin()+kCheapPrepend);
  //    readerIndex_ = kCheapPrepend;
  //    writerIndex_ = readerIndex_ + readable;
  //    assert(readable == readableBytes());
  //  }
  //}

 private:
  std::vector<char> buffer_;
  size_t readerIndex_;
  size_t writerIndex_;

  static const char kCRLF[];
};


}  // namespace LibNet

#endif  // LibNet_NET_BUFFER_H
