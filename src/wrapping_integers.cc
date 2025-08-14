#include "wrapping_integers.hh"
#include <algorithm>

using namespace std;

const uint64_t pow2_32 = 4294967296;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
  /* Construct a Wrap32 given an absolute sequence number n and the zero point. */

  // 为了避免负数，使用加法
  return Wrap32( ( n % pow2_32 + zero_point.raw_value_ ) % pow2_32 );
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  uint64_t diff = this->raw_value_ - zero_point.raw_value_; // 环内偏移
  uint64_t base = checkpoint & ( ~0ULL << 32 );             // checkpoint基础值

  // 距离checkpoint最近的三个候选内容
  uint64_t candidates[3] = { base + diff - ( 1ULL << 32 ), base + diff, base + diff + ( 1ULL << 32 ) };

  int best_index = 0;
  uint64_t best_dist = ( candidates[0] > checkpoint ) ? candidates[0] - checkpoint : checkpoint - candidates[0];
  for ( int i = 1; i < 3; ++i ) {
    uint64_t dist = ( candidates[i] > checkpoint ) ? candidates[i] - checkpoint : checkpoint - candidates[i];

    if ( dist < best_dist ) {
      best_dist = dist;
      best_index = i;
    }
  }

  return candidates[best_index];
}
