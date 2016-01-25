#ifndef FP_PORT_UDP_HPP
#define FP_PORT_UDP_HPP

#include "port.hpp"
#include "packet.hpp"
#include "context.hpp"
#include "dataplane.hpp"

#include <string>

namespace fp
{

// UDP Port thread work function.
extern void* udp_work_fn(void*);

class Port_udp : public Port
{
public:
  using Port::Port;

  // Constructors/Destructor.
  Port_udp(Port::Id, std::string const&);
  ~Port_udp();

  // Packet related funtions.
  Context*  recv();
  int       send();

  // Port state functions.
  int     open();
  void    close();

  // Accessors.
  Function work_fn() { return udp_work_fn; }

  // Data members.
  //
  // Socket file descriptor.
  int sock_fd_;
  int send_fd_;
  // Socket addresses.
  struct sockaddr_in src_addr_;
  struct sockaddr_in dst_addr_;
  // Message containers.
  struct mmsghdr     messages_[2048];
  struct iovec       iovecs_[2048];
  char               in_buffers_[2048][1024];
  struct timespec    timeout_;
};

} // end namespace fp

#endif
