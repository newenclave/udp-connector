#ifndef VTRC_MONOTONIC_TIMER_H
#define VTRC_MONOTONIC_TIMER_H

#include "boost/asio.hpp"
#include <chrono>

namespace vtrc { namespace common { namespace timer {

    struct monotonic_traits {

        typedef boost::posix_time::milliseconds milliseconds;
        typedef boost::posix_time::microseconds microseconds;
        typedef boost::posix_time::seconds seconds;
        typedef boost::posix_time::minutes minutes;
        typedef boost::posix_time::hours hours;

        typedef std::chrono::steady_clock::time_point time_type;
        typedef boost::posix_time::time_duration duration_type;

        static time_type now( )
        {
            return std::chrono::steady_clock::now( );
        }

        static time_type add( const time_type& time,
                              const duration_type& duration )
        {
            return time +
                   std::chrono::microseconds( duration.total_microseconds( ) );
        }

        static duration_type subtract( const time_type& timeLhs,
                                       const time_type& timeRhs )
        {
          std::chrono::microseconds duration_us (
                std::chrono::duration_cast<std::chrono::microseconds>
                                                        (timeLhs - timeRhs));
          return microseconds( duration_us.count( ) );
        }

        static bool less_than( const time_type& timeLhs,
                               const time_type& timeRhs )
        {
            return timeLhs < timeRhs;
        }

        static duration_type to_posix_duration( const duration_type& duration )
        {
            return duration;
        }
    };

    typedef  boost::asio::basic_deadline_timer< std::chrono::steady_clock,
                                                monotonic_traits > monotonic;

}}}

#endif // VTRCMONOTONICTIMER_H
