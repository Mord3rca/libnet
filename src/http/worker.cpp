#include "http/worker"

http::Worker::Worker(){}

http::Worker::~Worker(){}

void http::Worker::OnConnect(http::Client*, const std::string &ip, const int port)
{
  std::cout << "Client connected: " << ip << ":" << port << std::endl;
}

void http::Worker::OnReceived(http::Client *cli, const char *buff, const size_t size)
{
  http::Request* req;
  
  if( !cli->m_incomming ) cli->m_incomming = new http::Request();
  req = cli->m_incomming;
  
  http::Parser writer(req);
  writer.write( buff, size );
  
  if( writer.isComplete() )
  {
    switch( req->getMethod() )
    {
      case http::method::GET:
        this->OnGet(*cli, *req);
        break;
      
      case http::method::POST:
        this->OnPost(*cli, *req);
        break;
      
      case http::method::UNKNOWN:
      default:
      std::cout << "Paquet dropped" << std::endl;
    }
    std::string connection_policy = req->getHeader("Connection");
    if( connection_policy.find("Keep-Alive") == std::string::npos ||
        connection_policy.find("keep-alive") == std::string::npos )
      _http_close_request(cli);
    
    delete req; cli->m_incomming = nullptr;
  }
}

void http::Worker::OnError(http::Client*, const std::string &err)
{
  std::cerr << "Something went wrong: " << err << std::endl;
}

void http::Worker::OnDisconnect(http::Client*)
{
  std::cout << "Connection terminated.." << std::endl;
}

//Default HTTP CallBacks:
void http::Worker::OnPost(http::Client &client, const http::Request &req)
{
  http::Response resp;
  resp.setStatusCode(http::status_code::OK);
  resp.addHeader("Content-Type", "text/plain");
  
  resp.appendData("Working.");
  
  client << resp;
}

void http::Worker::OnGet(http::Client &client, const http::Request &req)
{
  http::Response resp;
  resp.setStatusCode(http::status_code::OK);
  resp.addHeader("Content-Type", "text/plain");
  
  resp.appendData("Working.");
  
  client << resp;
}

//TODO: Create this func.
void http::Worker::_http_close_request(http::Client*){}
