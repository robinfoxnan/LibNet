#ifndef LibNet_BASE_NONCOPYABLE_H
#define LibNet_BASE_NONCOPYABLE_H

namespace LibNet
{

  class noncopyable
  {
  public:
    noncopyable(const noncopyable &) = delete;
    void operator=(const noncopyable &) = delete;

  protected:
    noncopyable() = default;
    ~noncopyable() = default;
  };

} // namespace LibNet

#endif // LibNet_BASE_NONCOPYABLE_H
