#include "attacker-socket.h"

using namespace std;

namespace ns3{

  NS_OBJECT_ENSURE_REGISTERED (AttackerSocket);

  TypeId
  AttackerSocket::GetTypeId (void)
  {
    static TypeId tid = TypeId ("ns3::AttackerSocket")
      .SetParent<TcpSocketBase> ()
      .AddConstructor<AttackerSocket>()
      .SetGroupName ("Internet")
      ;
    return tid;
  }

  AttackerSocket::AttackerSocket(){}

  AttackerSocket::~AttackerSocket(){}

  void
  AttackerSocket::SendP(void){}

  void
  AttackerSocket::SetInitialSSThresh(uint32_t threshold)
  {
      m_ssThresh = threshold;
  }

  uint32_t
  AttackerSocket::GetInitialSSThresh(void) const
  {
      return m_ssThresh;
  }

  void
  AttackerSocket::SetInitialCwnd(uint32_t cwnd)
  {
      // NS_ABORT_MSG_UNLESS(m_state == CLOSED,
      //                     "AttackerSocket::SetInitialCwnd() cannot change initial cwnd after connection started.");
      m_initialCWnd = cwnd;
  }

  uint32_t
  AttackerSocket::GetInitialCwnd(void) const
  {
      return m_initialCWnd;
  }

  Ptr<TcpSocketBase>
  AttackerSocket::Fork(void)
  {
      // NS_LOG_FUNCTION_NOARGS();
      return CopyObject<AttackerSocket>(this);
  }

  /** Cut cwnd and enter fast recovery mode upon triple dupack */
  void
  AttackerSocket::DupAck(const TcpHeader& t, uint32_t count)
  {
      // NS_LOG_FUNCTION_NOARGS();
  }

  void
  AttackerSocket::ScaleSsThresh (uint8_t scaleFactor)
  {
      // NS_LOG_FUNCTION(this << (int)scaleFactor);
  }

}
