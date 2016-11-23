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

class udp_endpoint {

    ba::io_service             &ios_;
    ba::io_service::strand      dispatcher_;
    ba::ip::udp::socket         sock_;
    std::vector<std::uint8_t>   data_;
    ba::ip::udp::endpoint       remote_;

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

    void set_buf_size( size_t len )
    {
        data_.resize( len );
    }

public:

    udp_endpoint( ba::io_service &ios )
        :ios_(ios)
        ,dispatcher_(ios_)
        ,sock_(ios_)
        ,data_(4096)
    { }

    const std::uint8_t *get_data( ) const
    {
        return &data_[0];
    }

    void set_buffer_size( size_t len )
    {
        dispatch( std::bind( &udp_endpoint::set_buf_size, this, len ) );
    }

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

    void dispatch( std::function<void ()> call )
    {
        dispatcher_.dispatch( std::move(call) );
    }

    ba::ip::udp::socket &get_socket( )
    {
        return sock_;
    }

    void write( const char *data, size_t len )
    {
        sock_.async_send( ba::buffer(data, len), 0,
            dispatcher_.wrap(
                std::bind( &udp_endpoint::write_handler, this, ph::_1, ph::_2 )
            ) );
    }

    void write_to( const char *data, size_t len,
                   const ba::ip::udp::endpoint &to )
    {
        sock_.async_send_to( ba::buffer(data, len), to, 0,
            dispatcher_.wrap(
                std::bind( &udp_endpoint::write_handler, this, ph::_1, ph::_2 )
            ) );
    }

    void read(  )
    {
        sock_.async_receive( ba::buffer(&data_[0], data_.size( )),
                dispatcher_.wrap(
                    std::bind( &udp_endpoint::read_handler, this,
                               ph::_1, ph::_2 )
                ) );
    }

    void read_from( ba::ip::udp::endpoint from )
    {
        auto ep = std::make_shared<ba::ip::udp::endpoint>(std::move(from));
        sock_.async_receive_from( ba::buffer(&data_[0], data_.size( )), *ep, 0,
                            dispatcher_.wrap(
                                std::bind( &udp_endpoint::read_handler2, this,
                                            ph::_1, ph::_2, ep )
                            ) );
    }

    virtual void on_write( const bs::error_code &, std::size_t ) { }
    virtual void start( ) = 0;
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
        get_socket( ).bind( ep_ );
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
