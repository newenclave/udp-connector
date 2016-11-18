#include <vector>
#include <memory>
#include <iostream>
#include <set>

#include "boost/asio.hpp"

#include "async-transport-point.hpp"

#include "udp-wrapper.hpp"

namespace ba = boost::asio;
namespace bs = boost::system;

std::set<std::shared_ptr<udp_connector> > clients;
ba::io_service ios;

void on_accept( const ba::ip::udp::endpoint &from,
                std::uint8_t *data, std::size_t len )
{
    std::cout << "New client! " << from.address( ).to_string( )
              << ":" << from.port( ) << "\n";

    auto client = std::make_shared<udp_connector>(std::ref(ios), from);
    client->start( );
    client->sock( ).connect( from );
    client->write( "Hello!", 6 );
    client->read( );
    client->on_read_sig =
            [client](const ba::ip::udp::endpoint &from,
            std::uint8_t *data, std::size_t len )
            {
                std::cout << "on read " << from.address( ).to_string( )
                          << ":" << from.port( ) << "\n";
                client->write("Hello2!", 7);
                client->read( );
            };

    clients.insert(client);
}

int main( )
{

    try {
        ba::io_service::work wrk(ios);

        udp_acceptor ua( ios, "0.0.0.0", 12356 );
        ua.on_accept = &on_accept;

        ua.start( );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error " << ex.what( ) << "\n";
    }

    return 0;
}
