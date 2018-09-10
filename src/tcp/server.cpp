#include "server"

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

template<>
void tcp::Server<int>::OnReceived( void *ptr, const char* buff, ssize_t count)
{
  int fd = *( static_cast<int*>(ptr) );
  ::write(fd, buff, count);
}

template<>
struct tcp::Server<int>::Worker::container* tcp::Server<int>::Worker::create( int fd )
{
  if( !sock_non_block(fd) )
    throw std::runtime_error("sock_non_block(): error");
  
  struct epoll_event event;
  auto *obj = new tcp::Server<int>::Worker::container(fd, nullptr);
  event.data.ptr = obj;
  event.events = EPOLLIN | EPOLLET;
  if( epoll_ctl(root->m_pollfd, EPOLL_CTL_ADD, fd, &event) == -1 )
    throw std::runtime_error("epoll_ctl() error");
  
  m_clients.push_back(obj);
  
  return nullptr;
}

template<>
void tcp::Server<int>::Worker::close( struct tcp::Server<int>::Worker::container *ptr )
{
  epoll_ctl(root->m_pollfd, EPOLL_CTL_DEL, ptr->fd, nullptr);
  ::close(ptr->fd);
  
  auto i = std::find(m_clients.begin(), m_clients.end(), ptr);
  if( i != m_clients.end() )
    m_clients.erase(i);
  
  delete ptr;
}

template<>
void tcp::Server<int>::Worker::loop()
{
  struct epoll_event *events;
  
  events = new struct epoll_event[64];
  
  root->m_pollfd = epoll_create1(0);
  if( root->m_pollfd == -1 )
    throw std::runtime_error("epoll_create1(): Error");
  
  create( root->m_sockfd );
  
  if(!m_buff) m_buff = new char[root->m_buffsize];
  
  root->m_run = true; m_last_timeout_check = time(nullptr);
  while(root->m_run)
  {
    int n = epoll_wait( root->m_pollfd, events, 64, 500 );
    for( int i = 0; i < n; i++ )
    {
      auto *con_ptr = static_cast<struct tcp::Server<int>::Worker::container*>(events[i].data.ptr);
      if( (events[i].events & EPOLLERR) ||
          (events[i].events & EPOLLHUP) ||
          !(events[i].events & EPOLLIN) )
      {
        root->OnError(&con_ptr->fd, "Polling error: Object pulled without event.");
        close(con_ptr);
        continue;
      }
      else if( con_ptr->fd == root->m_sockfd )
      {
        while(true)
        {
          struct sockaddr in_addr; socklen_t in_len = sizeof(struct sockaddr);
          int infd;
          
          infd = accept( root->m_sockfd, &in_addr, &in_len );
          if( infd == -1 )
          {
            if( (errno == EAGAIN) ||
                (errno == EWOULDBLOCK) )
              break;
            else
              throw std::runtime_error("accept() error in event loop.");
          }
          
          create(infd);
          
          std::string ip; int port;
          gethostinfo(in_addr, ip, port);
          
          root->OnConnect(&infd, ip, port);
        }
        continue;
      }
      else if( events[i].events & EPOLLHUP )
      {
        {root->OnDisconnect(&con_ptr->fd);}
        close(con_ptr);
      }
      else
      {
        while(true)
        {
          ssize_t count = read(con_ptr->fd, m_buff, root->m_buffsize);
          
          if( count == -1)
          {
            if( errno != EAGAIN )
              throw std::runtime_error("TCP Event Loop: read() error");
            
            break;
          }
          else if( count == 0 )
          {
            {root->OnDisconnect(&con_ptr->fd);}
            close(con_ptr);
            break;
          }
          
          {root->OnReceived(&con_ptr->fd, m_buff, count);}
        }
      }
      con_ptr->last_event = time(nullptr);
    }
    _checkTimeout();
  }
  
  delete[] events;
  ::close(root->m_pollfd);
  ::close(root->m_sockfd);
  root->m_sockfd = -1;
}
