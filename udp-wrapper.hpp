#ifndef UDPWRAPPER_HPP
#define UDPWRAPPER_HPP

#include <memory>
#include <queue>
#include <functional>
#include <list>

#include "boost/asio.hpp"

namespace ba = boost::asio;
namespace bs = boost::system;
namespace ph = std::placeholders;


class udp_endpoint: public std::enable_shared_from_this<udp_endpoint> {

    ba::io_service             &ios_;
    ba::ip::udp::socket         sock_;
    std::vector<std::uint8_t>   data_;
    ba::ip::udp::endpoint       remote_;

    typedef std::string message_type;
    typedef std::function <
        void (const boost::system::error_code &)
    > write_closure;

    struct queue_value {

        typedef std::shared_ptr<queue_value> shared_type;

        message_type    message_;
        write_closure   success_;

        queue_value( const char *data, size_t length )
            :message_(data, length)
        { }

        static
        shared_type create( const char *data, size_t length )
        {
            return std::make_shared<queue_value>( data, length );
        }
    };

    void write_handler( const bs::error_code &err, std::size_t len )
    {
        on_write( err, len );
    }

    void read_handler( const bs::error_code &err, std::size_t len )
    {
        on_read( err, remote_, &data_[0], len );
    }

    void read_handler2( const bs::error_code &err,
                        std::size_t len,
                        std::shared_ptr<ba::ip::udp::endpoint> from )
    {
        on_read( err, *from, &data_[0], len );
    }

public:

    udp_endpoint( ba::io_service &ios )
        :ios_(ios)
        ,sock_(ios_)
        ,data_(4096)
    { }


    ba::ip::udp::endpoint &get_endpoint( )
    {
        return remote_;
    }

    void open_v4( )
    {
        sock_.open( ba::ip::udp::v4( ) );
    }

    void open_v6( )
    {
        sock_.open( ba::ip::udp::v6( ) );
    }

    ba::io_service &get_io_service( )
    {
        return ios_;
    }

    ba::ip::udp::socket &sock( )
    {
        return sock_;
    }

    void write( const char *data, size_t len )
    {
        sock_.async_send( ba::buffer(data, len), 0,
            std::bind( &udp_endpoint::write_handler, this, ph::_1, ph::_2 ) );
    }

    void write_to( const char *data, size_t len,
                   const ba::ip::udp::endpoint &to )
    {
        sock_.async_send_to( ba::buffer(data, len), to, 0,
            std::bind( &udp_endpoint::write_handler, this, ph::_1, ph::_2 ) );
    }

    void read(  )
    {
        sock_.async_receive( ba::buffer(&data_[0], data_.size( )),
                std::bind( &udp_endpoint::read_handler, this,
                           ph::_1, ph::_2 ));
    }

    void read_from( ba::ip::udp::endpoint from )
    {
        auto ep = std::make_shared<ba::ip::udp::endpoint>(std::move(from));
        sock_.async_receive_from( ba::buffer(&data_[0], data_.size( )),
                                  *ep, 0,
                std::bind( &udp_endpoint::read_handler2, this,
                           ph::_1, ph::_2, ep ));
    }

    virtual void start( ) = 0;
    virtual void on_write( const bs::error_code &, std::size_t ) = 0;
    virtual void on_read( const bs::error_code &,
                          const ba::ip::udp::endpoint &from,
                          std::uint8_t *, std::size_t ) = 0;

};

class udp_connector: public udp_endpoint {

    ba::ip::udp::endpoint ep_;

public:

    using read_signal = std::function<void (const ba::ip::udp::endpoint &,
                                            std::uint8_t *, std::size_t)>;

    read_signal on_read_sig;


    udp_connector( ba::io_service &ios,
                   const ba::ip::udp::endpoint &to )
        :udp_endpoint(ios)
        ,ep_(to)
    {

    }

    ba::ip::udp::endpoint &endpoint( )
    {
        return ep_;
    }

    void reconnect( const ba::ip::udp::endpoint &to )
    {
        sock( ).connect( to );
        std::cout << "re_connect to: " << ep_.address( ).to_string( )
                  << ":" << ep_.port( ) << std::endl;
    }

    void start( ) override
    {
        ep_.address( ).is_v4( ) ? open_v4( ) : open_v6( );
    }

    void on_write( const bs::error_code &, std::size_t )
    {

    }

    void on_read( const bs::error_code &err,
                  const ba::ip::udp::endpoint &from,
                  std::uint8_t *data, std::size_t len )
    {
        if( !err ) {
            on_read_sig( from, data, len );
        }
    }
};

class udp_acceptor: public udp_endpoint {

    ba::ip::udp::endpoint ep_;

public:


    using accept_signal = std::function<void (const ba::ip::udp::endpoint &,
                                        std::uint8_t *, std::size_t)>;

    accept_signal on_accept;

    udp_acceptor( ba::io_service &ios,
                  const std::string &addr, std::uint16_t port )
        :udp_endpoint(ios)
        ,ep_(ba::ip::address::from_string( addr ), port)
    {

    }

    void start( ) override
    {
        ep_.address( ).is_v4( ) ? open_v4( ) : open_v6( );
        sock( ).bind( ep_ );
        read_from( ep_ );
    }

    void on_write( const bs::error_code &, std::size_t )
    {

    }

    void on_read( const bs::error_code &err,
                  const ba::ip::udp::endpoint &from,
                  std::uint8_t *data, std::size_t len )
    {

        if( !err ) {
            on_accept( from, data, len );
            read_from( get_endpoint( ) );
        }
    }

};

#endif // UDPWRAPPER_HPP
