#ifndef ATTACKER_SOCKET_H
#define ATTACKER_SOCKET_H

// #include "ns3/mp-tcp-socket-base.h"
#include "ns3/tcp-socket-base.h"
// class TcpSocketBase;

using namespace std;

namespace ns3{

class AttackerSocket : public TcpSocketBase
{

public:
  AttackerSocket();
  virtual ~AttackerSocket();

  static TypeId GetTypeId (void);
  void SendP();

  //Inherit from TcpSocketBase
  virtual void SetInitialSSThresh (uint32_t threshold);
  virtual uint32_t GetInitialSSThresh (void) const;
  virtual void SetInitialCwnd (uint32_t cwnd);
  virtual uint32_t GetInitialCwnd (void) const;
  virtual Ptr<TcpSocketBase> Fork(void);
  virtual void DupAck (const TcpHeader& tcpHeader, uint32_t count);
  virtual void ScaleSsThresh (uint8_t scaleFactor);

  virtual void SetSndBufSize (uint32_t size);
  virtual uint32_t GetSndBufSize (void) const;
  virtual void SetRcvBufSize (uint32_t size);
  virtual uint32_t GetRcvBufSize (void) const;
  virtual void SetSegSize(uint32_t size);
  virtual uint32_t GetSegSize(void) const;

private:
  uint32_t m_initialCWnd;
  uint32_t m_ssThresh;

};

}

#endif
