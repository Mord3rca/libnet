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
