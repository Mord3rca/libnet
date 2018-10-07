#include "tcp/server"

tcp::Server::Server() :  m_sockfd(-1), m_maxconn(64), m_run(false)
{}
    
tcp::Server::Server(const std::string &ip, const unsigned int port) : Server()
{ this->bind(ip, port); }
    
tcp::Server::~Server() { stop(); }
    
void tcp::Server::bind(const std::string &ip, const unsigned int port)
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

void tcp::Server::start()
{
  m_run = true;
  //No Thread
  if( m_workers.size() == 1 )
  {
    m_workers[0]->setListeningSock(m_sockfd);
    m_workers[0]->start();
  }
  //Threaded Operation
  else if( m_workers.size() > 1 )
  {
    for(auto i : m_workers)
      i->threaded_start();
    
    int fd; struct sockaddr addr; socklen_t addr_len = sizeof(addr);
    
    struct pollfd pollfds[1];
    pollfds[0].fd = m_sockfd;
    pollfds[0].events = POLLIN;
    
    while( m_run )
    {
      int ret = poll(pollfds, 1, 500);
      
      if( ret > 0 )
      {
        fd = accept(m_sockfd, &addr, &addr_len);
        _distribute()->createNewItemFromFd(fd, addr);
      }
      else if( ret == -1 && errno != EINTR)
        throw std::runtime_error("tcp::Server: poll() error");
    }
  }
  else
    std::runtime_error("tcp::Server: No Worker Set.");
}
void tcp::Server::stop()
{
  m_run = false;
  for(auto i : m_workers)
  {
    i->stop();
    delete i;
  }
}
    
bool tcp::Server::isRunning() const noexcept
{return m_run;}
    
void tcp::Server::addWorker( IEventWorker* worker)
{ if(!m_run) m_workers.push_back(worker); }
    
void tcp::Server::setMaxConnection(int maxconn) noexcept
{ if(!m_run) m_maxconn = maxconn; }

IEventWorker* tcp::Server::_distribute( void )
{
  auto i = m_workers[0];
  for( size_t j = 1; j < m_workers.size(); j++ )
    i = (m_workers[j]->getClientNumber() < i->getClientNumber()) ? m_workers[j] : i;
  
  return i;
}
