#ifndef UDP_ACCEPTOR_H
#define UDP_ACCEPTOR_H

#include "udp-wrapper.hpp"
#include <chrono>

namespace test {
    class acceptor_base: public udp_endpoint {

    public:

        static std::uint64_t ticks_now( )
        {
            using std::chrono::duration_cast;
            using microsec = std::chrono::microseconds;
            auto n = std::chrono::high_resolution_clock::now( );
            return duration_cast<microsec>(n.time_since_epoch( )).count( );
        }

    public:

        void on_write( const bs::error_code &err, std::size_t len );
        void start( );
        void on_read( const bs::error_code &,
                      const ba::ip::udp::endpoint &from,
                      std::uint8_t *, std::size_t );

    };
}

#endif // UDP_ACCEPTOR_H
