

#include <iostream>
#include "boost/asio.hpp"

#include "udp-wrapper.hpp"

#include "vtrc-delayed-call.h"

using timer = vtrc::common::delayed_call;

class udp_connector0: public udp_connector {

    using reader = std::function<void (const ba::ip::udp::endpoint &from,
                                       std::uint8_t *data, std::size_t len)>;
    reader reader_;

    int test = 10;

    timer timer_;

public:

    udp_connector0( ba::io_service &ios,
                    const ba::ip::udp::endpoint &to )
        :udp_connector(ios, to)
        ,timer_(ios)
    {
        reader_ = [this](const ba::ip::udp::endpoint &from,
                         std::uint8_t *data, std::size_t len )
        {
            first_read( from, data, len );
        };
    }

    void first_read( const ba::ip::udp::endpoint &from,
                     std::uint8_t *data, std::size_t len )
    {
        std::cout << "first_read "
                  << " from " << from.address( ).to_string( )
                  << ":" << from.port( )
                  << " len "
                  << len << std::endl;

        sock( ).connect( from );

        reader_ = [this](const ba::ip::udp::endpoint &from,
                         std::uint8_t *data, std::size_t len )
        {
            next_read( from, data, len );
        };

        write( "!", 1 );
    }

    void next_read( const ba::ip::udp::endpoint &from,
                    std::uint8_t *data, std::size_t len )
    {
        std::cout << "next_read "
                  << " from " << from.address( ).to_string( )
                  << ":" << from.port( )
                  << " len "
                  << len << std::endl;
        if( test-- ) {
            timer_.call_from_now([this](...) {
                write( "&", 1 );
            }, timer::seconds(1) );
        }
    }

    void on_read( const bs::error_code &err,
                  const ba::ip::udp::endpoint &from,
                  std::uint8_t *data, std::size_t len )
    {
        if( !err ) {
            reader_( from, data, len );
            read( );
        } else {
            std::cout << "Error! " << err.message( ) << "\n";
        }
    }
};


int main( )
{
    try {

        ba::io_service ios;

        ba::ip::udp::endpoint ep( ba::ip::address::from_string( "127.0.0.1" ), 12356 );

        udp_connector0 bc( ios, ep );
        bc.start( );
        //bc.sock( ).connect( ep );
        bc.write_to( "hellO!", 6, ep );
        bc.read_from( ep );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error " << ex.what( ) << "\n";
    }

    return 0;
}
