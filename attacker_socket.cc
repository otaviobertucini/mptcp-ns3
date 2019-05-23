#include "attacker_socket.h"

using namespace std;

namespace ns3{

  AttackerSocket::AttackerSocket(){}

  AttackerSocket::~AttackerSocket(){}

  void
  AttackerSocket::SendP(void){}

  void
  MpTcpSocketBase::SetInitialSSThresh(uint32_t threshold)
  {
      m_ssThresh = threshold;
  }

  uint32_t
  MpTcpSocketBase::GetInitialSSThresh(void) const
  {
      return m_ssThresh;
  }

  void
  MpTcpSocketBase::SetInitialCwnd(uint32_t cwnd)
  {
      NS_ABORT_MSG_UNLESS(m_state == CLOSED,
                          "MpTcpsocketBase::SetInitialCwnd() cannot change initial cwnd after connection started.");
      m_initialCWnd = cwnd;
  }

  uint32_t
  MpTcpSocketBase::GetInitialCwnd(void) const
  {
      return m_initialCWnd;
  }

  Ptr<TcpSocketBase>
  MpTcpSocketBase::Fork(void)
  {
      NS_LOG_FUNCTION_NOARGS();
      return CopyObject<AttackerSocket>(this);
  }

  /** Cut cwnd and enter fast recovery mode upon triple dupack */
  void
  MpTcpSocketBase::DupAck(const TcpHeader& t, uint32_t count)
  {
      NS_LOG_FUNCTION_NOARGS();
  }

  void
  MpTcpSocketBase::ScaleSsThresh (uint8_t scaleFactor)
  {
      NS_LOG_FUNCTION(this << (int)scaleFactor);
  }

}
