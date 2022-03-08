// Taken from PCRE pcre_stringpiece.h
//
// Copyright (c) 2005, Google Inc.
// All rights reserved.


#ifndef LibNet_BASE_STRINGPIECE_H
#define LibNet_BASE_STRINGPIECE_H

#include <string>
#include <string.h>
#include <iosfwd> // for ostream forward-declaration

using namespace std;
namespace LibNet
{

	// For passing C-style string argument to a function.
	class StringArg // copyable
	{
	public:
		StringArg(const char *str) : str_(str)
		{
		}

		StringArg(const string &str) : str_(str.c_str())
		{
		}

		const char *c_str() const { return str_; }

	private:
		const char *str_;
	};

	class StringPiece
	{
	private:
		const char *ptr_;
		size_t length_;

	public:
		// We provide non-explicit singleton constructors so users can pass
		// in a "const char*" or a "string" wherever a "StringPiece" is
		// expected.
		StringPiece() : ptr_(NULL), length_(0) {}
		StringPiece(const char *str) : ptr_(str), length_(static_cast<int>(strlen(ptr_))) {}
		StringPiece(const unsigned char *str) : ptr_(reinterpret_cast<const char *>(str)),
			length_(static_cast<int>(strlen(ptr_))) {}

		StringPiece(const string &str)
			: ptr_(str.data()), length_(static_cast<int>(str.size())) {}

		StringPiece(const char *offset, size_t len)
			: ptr_(offset), length_(len) {}

		// data() may return a pointer to a buffer with embedded NULs, and the
		// returned buffer may or may not be null terminated.  Therefore it is
		// typically a mistake to pass data() to a routine that expects a NUL
		// terminated string.  Use "as_string().c_str()" if you really need to do
		// this.  Or better yet, change your routine so it does not rely on NUL
		// termination.
		const char *data() const { return ptr_; }
		size_t size() const { return length_; }
		bool empty() const { return length_ == 0; }
		const char *begin() const { return ptr_; }
		const char *end() const { return ptr_ + length_; }

		size_t size()
		{
			return length_;
		}

		void clear()
		{
			ptr_ = NULL;
			length_ = 0;
		}
		void set(const char *buffer, size_t len)
		{
			ptr_ = buffer;
			length_ = len;
		}
		void set(const char *str)
		{
			ptr_ = str;
			length_ = static_cast<int>(strlen(str));
		}
		void set(const void *buffer, size_t len)
		{
			ptr_ = reinterpret_cast<const char *>(buffer);
			length_ = len;
		}

		char operator[](size_t i) const { return ptr_[i]; }

		void remove_prefix(size_t n)
		{
			ptr_ += n;
			length_ -= n;
		}

		void remove_suffix(size_t n)
		{
			length_ -= n;
		}

		bool operator==(const StringPiece &x) const
		{
			return ((length_ == x.length_) &&
				(memcmp(ptr_, x.ptr_, length_) == 0));
		}
		bool operator!=(const StringPiece &x) const
		{
			return !(*this == x);
		}

#define STRINGPIECE_BINARY_PREDICATE(cmp, auxcmp)                            \
  bool operator cmp(const StringPiece &x) const                              \
  {                                                                          \
    int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_); \
    return ((r auxcmp 0) || ((r == 0) && (length_ cmp x.length_)));          \
  }
		STRINGPIECE_BINARY_PREDICATE(< , < );
		STRINGPIECE_BINARY_PREDICATE(<= , < );
		STRINGPIECE_BINARY_PREDICATE(>= , > );
		STRINGPIECE_BINARY_PREDICATE(> , > );
#undef STRINGPIECE_BINARY_PREDICATE

		int compare(const StringPiece &x) const
		{
			int r = memcmp(ptr_, x.ptr_, length_ < x.length_ ? length_ : x.length_);
			if (r == 0)
			{
				if (length_ < x.length_)
					r = -1;
				else if (length_ > x.length_)
					r = +1;
			}
			return r;
		}

		string as_string() const
		{
			return string(data(), size());
		}

		void CopyToString(string *target) const
		{
			target->assign(ptr_, length_);
		}

		// Does "this" start with "x"
		bool starts_with(const StringPiece &x) const
		{
			return ((length_ >= x.length_) && (memcmp(ptr_, x.ptr_, x.length_) == 0));
		}
	};

} // namespace LibNet

// ------------------------------------------------------------------
// Functions used to create STL containers that use StringPiece
//  Remember that a StringPiece's lifetime had better be less than
//  that of the underlying string or char*.  If it is not, then you
//  cannot safely store a StringPiece into an STL container
// ------------------------------------------------------------------

#ifdef HAVE_TYPE_TRAITS
// This makes vector<StringPiece> really fast for some STL implementations
template <>
struct __type_traits<LibNet::StringPiece>
{
	typedef __true_type has_trivial_default_constructor;
	typedef __true_type has_trivial_copy_constructor;
	typedef __true_type has_trivial_assignment_operator;
	typedef __true_type has_trivial_destructor;
	typedef __true_type is_POD_type;
};
#endif

// allow StringPiece to be logged
std::ostream &operator<<(std::ostream &o, const LibNet::StringPiece &piece);

#endif // LibNet_BASE_STRINGPIECE_H
