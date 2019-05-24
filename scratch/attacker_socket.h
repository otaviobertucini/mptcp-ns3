#ifndef ATTACKER_SOCKET_H
#define ATTACKER_SOCKET_H

#include "ns3/tcp-socket-base.h"
// class TcpSocketBase;

using namespace std;

namespace ns3{

class AttackerSocket : public TcpSocketBase
{

public:
  AttackerSocket();
  ~AttackerSocket();
private:
  void SendP();

  void SetInitialSSThresh (uint32_t threshold);
  uint32_t GetInitialSSThresh (void) const;
  void SetInitialCwnd (uint32_t cwnd);
  uint32_t GetInitialCwnd (void) const;
  Ptr<TcpSocketBase> Fork(void);
  void DupAck (const TcpHeader& tcpHeader, uint32_t count);
  void ScaleSsThresh (uint8_t scaleFactor);

};

}

#endif
