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

  void
    AttackerSocket::SetSndBufSize(uint32_t size)
    {
        //m_txBuffer.SetMaxBufferSize(size);
        //sendingBuffer = new DataBuffer(size);
        //sendingBuffer.SetBufferSize(size);
    }
    uint32_t
    AttackerSocket::GetSndBufSize(void) const
    {
        //return m_txBuffer.MaxBufferSize();
        return 0;
    }
    void
    AttackerSocket::SetRcvBufSize(uint32_t size)
    {
        //m_rxBuffer.SetMaxBufferSize(size);
        //recvingBuffer = new DataBuffer(size);
        // Size of recving buffer does not allocate any memory instantly but allows node to store to this bound.
        //recvingBuffer.SetBufferSize(size);//50000000
        //NS_LOG_UNCOND("++++++++++++++++("<< size << ")");
    }
    uint32_t
    AttackerSocket::GetRcvBufSize(void) const
    {
        //return m_rxBuffer.MaxBufferSize();
        return 0;
    }

    void
    AttackerSocket::SetSegSize(uint32_t size)
    {
        // segmentSize = size;
        // NS_ABORT_MSG_UNLESS(m_state == CLOSED, "Cannot change segment size dynamically.");
    }

    uint32_t
    AttackerSocket::GetSegSize(void) const
    {
        //return segmentSize;
    }

}
