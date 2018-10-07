#include "utils"

bool sock_non_block(int fd) noexcept
{
  int flags = fcntl( fd, F_GETFL, 0 );
  if(flags == -1)
    return false;
  
  flags |= O_NONBLOCK;
  if( fcntl( fd, F_SETFL, flags ) == -1 )
    return false;
  
  return true;
}

void gethostinfo(const struct sockaddr &addr, std::string &ip, int &port)
{
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  getnameinfo(&addr, sizeof(addr),
              hbuf, sizeof(hbuf),
              sbuf, sizeof(sbuf),
              NI_NUMERICHOST | NI_NUMERICSERV);
  ip = hbuf;
  port = std::atoi(sbuf);
}
