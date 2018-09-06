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

tcp::Server::Server() : m_sockfd(-1), m_pollfd(-1),
                        m_run(false), m_maxconn(64),
                        m_buff(nullptr), m_buffsize(2048)
{}

tcp::Server::Server(const std::string &ip, const unsigned int port) : Server()
{
  this->bind(ip, port);
}

tcp::Server::~Server()
{
  if(m_buff) delete m_buff;
}

void tcp::Server::bind( const std::string &ip, const unsigned int port)
{
  struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
  
  m_sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if( m_sockfd < 0 )
    throw std::runtime_error("socket(): error");
  
  int turnon = 1;
  setsockopt(m_sockfd, SOL_SOCKET, SO_REUSEADDR, &turnon, sizeof(turnon));
  
  if( ::bind(m_sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0 )
    throw std::runtime_error("bind(): error");
  
  if( listen(m_sockfd, m_maxconn) < 0 )
    throw std::runtime_error("listen(): error");
  
  if( !sock_non_block(m_sockfd) )
    throw std::runtime_error("sock_non_block(): error");
}

void tcp::Server::maxConnection( unsigned int conn )
{
  if( !m_run ) m_maxconn = conn;
}

void tcp::Server::buffSize( unsigned int size )
{
  if( m_run ) return;
  
  m_buffsize = size;
  if( m_buff ) delete m_buff;
  
  m_buff = new char[m_buffsize];
}

void tcp::Server::start()
{
  struct epoll_event *events;
  
  events = new struct epoll_event[64];
  
  m_pollfd = epoll_create1(0);
  if( m_pollfd == -1 )
    throw std::runtime_error("epoll_create1(): Error");
  
  _tcp_create( m_sockfd );
  
  if(!m_buff) this->buffSize(m_buffsize);
  
  m_run = true;
  while(m_run)
  {
    int n;
    n = epoll_wait( m_pollfd, events, 64, 500 );
    for( int i = 0; i < n; i++ )
    {
      if( (events[i].events & EPOLLERR) ||
          (events[i].events & EPOLLHUP) ||
          !(events[i].events & EPOLLIN) )
      {
        OnError(events[i].data.fd, "Polling error: Object pulled without event.");
        _tcp_close(events[i].data.fd);
        continue;
      }
      else if( events[i].data.fd == m_sockfd )
      {
        while(true)
        {
          struct sockaddr in_addr; socklen_t in_len = sizeof(struct sockaddr);
          int infd;
          
          infd = accept( m_sockfd, &in_addr, &in_len );
          if( infd == -1 )
          {
            if( (errno == EAGAIN) ||
                (errno == EWOULDBLOCK) )
              break;
            else
              throw std::runtime_error("accept() error in event loop.");
          }
          
          _tcp_create(infd);
          
          std::string ip; int port;
          _gethostinfo(in_addr, ip, port);
          
          OnConnect(infd, ip, port);
        }
        continue;
      }
      else if( events[i].events & EPOLLHUP )
      {
        int fd = events[i].data.fd;
        {this->OnDisconnect(fd);}
        _tcp_close(fd);
      }
      else
      {
        int fd = events[i].data.fd;
        
        while(true)
        {
          ssize_t count = read(fd, m_buff, m_buffsize);
          
          if( count == -1)
          {
            if( errno != EAGAIN )
              throw std::runtime_error("TCP Event Loop: read() error");
            
            break;
          }
          else if( count == 0 )
          {
            {OnDisconnect(fd);}
            _tcp_close(fd);
            break;
          }
          
          {OnReceived(fd, m_buff, count);}
        }
      }
    }
  }
  
  delete[] events;
  close(m_pollfd);
  close(m_sockfd);
  m_sockfd = -1;
}

void tcp::Server::OnReceived(int fd, const char *buff, ssize_t size)
{
  write(fd, buff, size);
}

void tcp::Server::_tcp_create( int fd )
{
  if( !sock_non_block(fd) )
    throw std::runtime_error("sock_non_block(): error");
  
  struct epoll_event event;
  event.data.fd = fd;
  event.events = EPOLLIN | EPOLLET;
  if( epoll_ctl(m_pollfd, EPOLL_CTL_ADD, fd, &event) == -1 )
    throw std::runtime_error("epoll_ctl() error");
}

void tcp::Server::_tcp_close( int fd )
{
  epoll_ctl(m_pollfd, EPOLL_CTL_DEL, fd, nullptr);
  close(fd);
}

void tcp::Server::_gethostinfo(struct sockaddr &addr, std::string& ip, int& port)
{
  char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];
  getnameinfo(&addr, sizeof(addr),
              hbuf, sizeof(hbuf),
              sbuf, sizeof(sbuf),
              NI_NUMERICHOST | NI_NUMERICSERV);
  
  ip = hbuf;
  port = std::atoi(sbuf);
}
