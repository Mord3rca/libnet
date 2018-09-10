#include "socket"

tcp::Socket::Socket() : m_fd(-1)
{}

tcp::Socket::Socket(int fd) : m_fd(fd)
{}

tcp::Socket::~Socket()
{}

ssize_t tcp::Socket::write(const char* buff, ssize_t size)
{
  return (m_fd != -1) ? ::write(m_fd, buff, size) : -1;
}

ssize_t tcp::Socket::read(char* buff, ssize_t size)
{
  return (m_fd != -1) ? ::read(m_fd, buff, size) : -1;
}
