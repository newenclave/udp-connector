#ifndef UDP_LISTENER_H
#define UDP_LISTENER_H

#include "boost/asio.hpp"

namespace test {

    class udp_listener {

        boost::asio::ip::udp::endpoint ep_;
        std::uint32_t slaves_;

    public:

        udp_listener( const std::string &addr, std::uint16_t port,
                      std::uint32_t slaves )
            :ep_(boost::asio::ip::address::from_string(addr), port)
            ,slaves_(slaves)
        { }

        std::string name( ) const
        {
            std::ostringstream oss;
            oss << "udp://" << ep_.address( ).to_string( )
                << ":" << ep_.port( );
            return oss.str( );
        }

        void start( )
        {

        }

        void stop ( )
        {

        }

        bool is_active( )   const
        {

        }

        bool is_local( )    const
        {
            return false;
        }
    };

}

#endif // UDPLISTENER_H
