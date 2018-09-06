#include <iostream>
#include <csignal>

#include <tcp>

tcp::Server *serv;

int sig_handler(int sig)
{
  switch(sig)
  {
    case SIGINT:
      serv->stop();
      break;
    default:;
  }
  return 0;
}

int main(int argc, char **argv)
{
  serv = new tcp::Server("127.0.0.1", 6666);
  serv->buffSize( 512 );
  serv->maxConnection( 5 );
  serv->start();
  
  return 0;
}