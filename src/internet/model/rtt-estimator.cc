/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: Rajib Bhattacharjea<raj.b@gatech.edu>
//

// Ported from:
// Georgia Tech Network Simulator - Round Trip Time Estimator Class
// George F. Riley.  Georgia Tech, Spring 2002

// Base class allows variations of round trip time estimators to be
// implemented

#include <iostream>
#include <cmath>
#include "rtt-estimator.h"
#include "ns3/simulator.h"
#include "ns3/double.h"
#include "ns3/integer.h"
#include "ns3/uinteger.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("RttEstimator");

NS_OBJECT_ENSURE_REGISTERED (RttEstimator);

static const double TOLERANCE = 1e-6;

TypeId 
RttEstimator::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RttEstimator")
    .SetParent<Object> ()
    .SetGroupName ("Internet")
    .AddAttribute ("MaxMultiplier", 
                   "Maximum RTO Multiplier",
                   UintegerValue (64),
                   MakeUintegerAccessor (&RttEstimator::m_maxMultiplier),
                   MakeUintegerChecker<uint16_t> ())
    .AddAttribute ("InitialEstimation", 
                   "Initial RTT estimate",
                   TimeValue (Seconds (1.0)),
                   MakeTimeAccessor (&RttEstimator::m_initialEstimatedRtt),
                   MakeTimeChecker ())
   .AddAttribute ("MinRTO", 
                   "Minimum retransmit timeout value",
                   TimeValue (Seconds (0.2)), // RFC2988 says min RTO=1 sec, but Linux uses 200ms. See http://www.postel.org/pipermail/end2end-interest/2004-November/004402.html
                   MakeTimeAccessor (&RttEstimator::SetMinRto,
                                     &RttEstimator::GetMinRto),
                   MakeTimeChecker ())
  ;
  return tid;
}


void 
RttEstimator::SetMinRto (Time minRto)
{
  NS_LOG_FUNCTION (this << minRto);
  m_minRto = minRto;
}
Time 
RttEstimator::GetMinRto (void) const
{
  return m_minRto;
}
void 
RttEstimator::SetCurrentEstimate (Time estimate)
{
  NS_LOG_FUNCTION (this << estimate);
  m_currentEstimatedRtt = estimate;
}
Time 
RttEstimator::GetCurrentEstimate (void) const
{
  return m_currentEstimatedRtt;
}

Time
RttEstimator::GetEstimate (void) const
{
  return m_estimatedRtt;
}

Time 
RttEstimator::GetVariation (void) const
{
  return m_estimatedVariation;
}

//RttHistory methods
RttHistory::RttHistory (SequenceNumber32 s, uint32_t c, Time t)
  : seq (s), count (c), time (t), retx (false)
{
  NS_LOG_FUNCTION (this);
}

RttHistory::RttHistory (const RttHistory& h)
  : seq (h.seq), count (h.count), time (h.time), retx (h.retx)
{
  NS_LOG_FUNCTION (this);
}

// Base class methods

RttEstimator::RttEstimator ()
  : m_next (1), m_history (),
    m_nSamples (0),
    m_multiplier (1)
{ 
  NS_LOG_FUNCTION (this);
  
  // We need attributes initialized here, not later, so use the 
  // ConstructSelf() technique documented in the manual
  ObjectBase::ConstructSelf (AttributeConstructionList ());
  m_estimatedRtt = m_initialEstimatedRtt;
  m_currentEstimatedRtt = m_initialEstimatedRtt;//MPTCP
  m_estimatedVariation = Time (0);
  NS_LOG_DEBUG ("Initialize m_estimatedRtt to " << m_estimatedRtt.GetSeconds () << " sec.");
}

RttEstimator::RttEstimator (const RttEstimator& c)
  : Object (c),m_next (c.m_next), m_history (c.m_history),
    m_maxMultiplier (c.m_maxMultiplier), 
    m_initialEstimatedRtt (c.m_initialEstimatedRtt),
    m_estimatedRtt (c.m_estimatedRtt),
    m_currentEstimatedRtt (c.m_currentEstimatedRtt), m_minRto (c.m_minRto),
    m_estimatedVariation (c.m_estimatedVariation),
    m_nSamples (c.m_nSamples),
    m_multiplier (c.m_multiplier)
{
  NS_LOG_FUNCTION (this);
}

RttEstimator::~RttEstimator ()
{
  NS_LOG_FUNCTION (this);
}

TypeId
RttEstimator::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}


void RttEstimator::SentSeq (SequenceNumber32 seq, uint32_t size)
{ 
  NS_LOG_FUNCTION (this << seq << size);
  // Note that a particular sequence has been sent
  if (seq == m_next)
    { // This is the next expected one, just log at end
      m_history.push_back (RttHistory (seq, size, Simulator::Now () ));
      m_next = seq + SequenceNumber32 (size); // Update next expected
    }
  else
    { // This is a retransmit, find in list and mark as re-tx
      for (RttHistory_t::iterator i = m_history.begin (); i != m_history.end (); ++i)
        {
          if ((seq >= i->seq) && (seq < (i->seq + SequenceNumber32 (i->count))))
            { // Found it
              i->retx = true;
              // One final test..be sure this re-tx does not extend "next"
              if ((seq + SequenceNumber32 (size)) > m_next)
                {
                  m_next = seq + SequenceNumber32 (size);
                  i->count = ((seq + SequenceNumber32 (size)) - i->seq); // And update count in hist
                }
              break;
            }
        }
    }
}

Time RttEstimator::AckSeq (SequenceNumber32 ackSeq)
{ 
  NS_LOG_FUNCTION (this << ackSeq);
  // An ack has been received, calculate rtt and log this measurement
  // Note we use a linear search (O(n)) for this since for the common
  // case the ack'ed packet will be at the head of the list
  Time m = Seconds (0.0);
  if (m_history.size () == 0) return (m);    // No pending history, just exit
  RttHistory& h = m_history.front ();
  if (!h.retx && ackSeq >= (h.seq + SequenceNumber32 (h.count)))
    { // Ok to use this sample
      m = Simulator::Now () - h.time; // Elapsed time
      Measurement (m);                // Log the measurement
      ResetMultiplier ();             // Reset multiplier on valid measurement
    }
  // Now delete all ack history with seq <= ack
  while(m_history.size () > 0)
    {
      RttHistory& h = m_history.front ();
      if ((h.seq + SequenceNumber32 (h.count)) > ackSeq) break;               // Done removing
      m_history.pop_front (); // Remove
    }
  return m;
}

void RttEstimator::ClearSent ()
{ 
  NS_LOG_FUNCTION (this);
  // Clear all history entries
  m_next = 1;
  m_history.clear ();
}

void RttEstimator::IncreaseMultiplier ()
{
  NS_LOG_FUNCTION (this);
  m_multiplier = (m_multiplier*2 < m_maxMultiplier) ? m_multiplier*2 : m_maxMultiplier;
  NS_LOG_DEBUG ("Multiplier increased to " << m_multiplier);
}

void RttEstimator::ResetMultiplier ()
{
  NS_LOG_FUNCTION (this);
  m_multiplier = 1;
}


void RttEstimator::Reset ()
{ 
  NS_LOG_FUNCTION (this);
  // Reset to initial state
  m_next = 1;
  m_currentEstimatedRtt = m_initialEstimatedRtt;
  m_history.clear ();         // Remove all info from the history
  m_estimatedRtt = m_initialEstimatedRtt;
  m_estimatedVariation = Time (0);
  m_nSamples = 0;
  ResetMultiplier ();
}

uint32_t 
RttEstimator::GetNSamples (void) const
{
  return m_nSamples;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// Mean-Deviation Estimator

NS_OBJECT_ENSURE_REGISTERED (RttMeanDeviation);

TypeId 
RttMeanDeviation::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::RttMeanDeviation")
    .SetParent<RttEstimator> ()
    .SetGroupName ("Internet")
    .AddConstructor<RttMeanDeviation> ()
    .AddAttribute ("Alpha",
                   "Gain used in estimating the RTT, must be 0 <= alpha <= 1",
                   DoubleValue (0.125),
                   MakeDoubleAccessor (&RttMeanDeviation::m_alpha),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("Beta",
                   "Gain used in estimating the RTT variation, must be 0 <= beta <= 1",
                   DoubleValue (0.25),
                   MakeDoubleAccessor (&RttMeanDeviation::m_beta),
                   MakeDoubleChecker<double> (0, 1))
    .AddAttribute ("Gain",
                   "Gain used in estimating the RTT, must be 0 < Gain < 1",
                   DoubleValue (0.1),
                   MakeDoubleAccessor (&RttMeanDeviation::m_gain),
                   MakeDoubleChecker<double> ())
  ;
  return tid;
}

RttMeanDeviation::RttMeanDeviation():
 m_variance (0) 
{
  NS_LOG_FUNCTION (this);
}

RttMeanDeviation::RttMeanDeviation (const RttMeanDeviation& c)
  : RttEstimator (c), m_alpha (c.m_alpha), m_beta (c.m_beta),m_gain (c.m_gain), m_variance (c.m_variance)
{
  NS_LOG_FUNCTION (this);
}

TypeId
RttMeanDeviation::GetInstanceTypeId (void) const
{
  return GetTypeId ();
}

uint32_t
RttMeanDeviation::CheckForReciprocalPowerOfTwo (double val) const
{
  NS_LOG_FUNCTION (this << val);
  if (val < TOLERANCE)
    {
      return 0;
    }
  // supports 1/32, 1/16, 1/8, 1/4, 1/2
  if (std::abs (1/val - 8) < TOLERANCE)
    {
      return 3;
    }
  if (std::abs (1/val - 4) < TOLERANCE)
    {
      return 2;
    }
  if (std::abs (1/val - 32) < TOLERANCE)
    {
      return 5;
    }
  if (std::abs (1/val - 16) < TOLERANCE)
    {
      return 4;
    }
  if (std::abs (1/val - 2) < TOLERANCE)
    {
      return 1;
    }
  return 0;
}

void
RttMeanDeviation::FloatingPointUpdate (Time m)
{
  NS_LOG_FUNCTION (this << m);

  // EWMA formulas are implemented as suggested in
  // Jacobson/Karels paper appendix A.2

  // SRTT <- (1 - alpha) * SRTT + alpha *  R'
  Time err (m - m_estimatedRtt);
  double gErr = err.ToDouble (Time::S) * m_alpha;
  m_estimatedRtt += Time::FromDouble (gErr, Time::S);

  // RTTVAR <- (1 - beta) * RTTVAR + beta * |SRTT - R'|
  Time difference = Abs (err) - m_estimatedVariation;
  m_estimatedVariation += Time::FromDouble (difference.ToDouble (Time::S) * m_beta, Time::S);

  return;
}

void
RttMeanDeviation::IntegerUpdate (Time m, uint32_t rttShift, uint32_t variationShift)
{
  NS_LOG_FUNCTION (this << m << rttShift << variationShift);
  // Jacobson/Karels paper appendix A.2
  int64_t meas = m.GetInteger ();
  int64_t delta = meas - m_estimatedRtt.GetInteger ();
  int64_t srtt = (m_estimatedRtt.GetInteger () << rttShift) + delta;
  m_estimatedRtt = Time::From (srtt >> rttShift);
  if (delta < 0)
    {
      delta = -delta;
    }
  delta -= m_estimatedVariation.GetInteger ();
  int64_t rttvar = m_estimatedVariation.GetInteger () << variationShift;
  rttvar += delta;
  m_estimatedVariation = Time::From (rttvar >> variationShift);
  return;
}

void 
RttMeanDeviation::Measurement (Time m)
{
  NS_LOG_FUNCTION (this << m);
  if (m_nSamples)
    { 
      // If both alpha and beta are reciprocal powers of two, updating can
      // be done with integer arithmetic according to Jacobson/Karels paper.
      // If not, since class Time only supports integer multiplication,
      // must convert Time to floating point and back again
      // MPTCP	-----------------
      Time err (m - m_currentEstimatedRtt);
      double gErr = err.ToDouble (Time::S) * m_gain;
      m_currentEstimatedRtt += Time::FromDouble (gErr, Time::S);
      Time difference = Abs (err) - m_variance;
      NS_LOG_DEBUG ("m_variance += " << Time::FromDouble (difference.ToDouble (Time::S) * m_gain, Time::S));
      m_variance += Time::FromDouble (difference.ToDouble (Time::S) * m_gain, Time::S);
      // -------------------------
      uint32_t rttShift = CheckForReciprocalPowerOfTwo (m_alpha);
      uint32_t variationShift = CheckForReciprocalPowerOfTwo (m_beta);
      if (rttShift && variationShift)
        {
          IntegerUpdate (m, rttShift, variationShift);
        }
      else
        {
          FloatingPointUpdate (m);
        }
    }
  else
    { // First sample
      // MPTCP -------------------
      m_currentEstimatedRtt = m;             // Set estimate to current
      //variance = sample / 2;               // And variance to current / 2
      m_variance = m; // try this 
      // -------------------------
      m_estimatedRtt = m;               // Set estimate to current
      m_estimatedVariation = m / 2;  // And variation to current / 2
      NS_LOG_DEBUG ("(first sample) m_estimatedVariation += " << m);
    }
  m_nSamples++;
}

Time 
RttMeanDeviation::RetransmitTimeout ()
{
  NS_LOG_FUNCTION (this);
  NS_LOG_DEBUG ("RetransmitTimeout:  var " << m_variance.GetSeconds () << " est " << m_currentEstimatedRtt.GetSeconds () << " multiplier " << m_multiplier);
  // RTO = srtt + 4* rttvar
  int64_t temp = m_currentEstimatedRtt.ToInteger (Time::MS) + 4 * m_variance.ToInteger (Time::MS);
  if (temp < m_minRto.ToInteger (Time::MS))
    {
      temp = m_minRto.ToInteger (Time::MS);
    } 
  temp = temp * m_multiplier; // Apply backoff
  Time retval = Time::FromInteger (temp, Time::MS);
  NS_LOG_DEBUG ("RetransmitTimeout:  return " << retval.GetSeconds ());
  return (retval);  
}

Ptr<RttEstimator> 
RttMeanDeviation::Copy () const
{
  NS_LOG_FUNCTION (this);
  return CopyObject<RttMeanDeviation> (this);
}

void 
RttMeanDeviation::Reset ()
{ 
  NS_LOG_FUNCTION (this);
  m_variance = Seconds (0);
  RttEstimator::Reset ();
}

void RttMeanDeviation::Gain (double g)
{
  NS_LOG_FUNCTION (this);
  NS_ASSERT_MSG( (g > 0) && (g < 1), "RttMeanDeviation: Gain must be less than 1 and greater than 0" );
  m_gain = g;
}


} //namespace ns3
