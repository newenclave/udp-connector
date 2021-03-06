#include <vector>
#include <memory>
#include <iostream>
#include <set>
#include <thread>

#include "boost/asio.hpp"

#include "async-transport-point.hpp"
#include "vtrc-delayed-call.h"

#include "udp-wrapper.hpp"

#include "udp-listener.h"

namespace ba = boost::asio;
namespace bs = boost::system;

std::set<std::shared_ptr<udp_connector> > clients;
ba::io_service ios;

class udp_endpoint_atapter;

using delayed_call = vtrc::common::delayed_call;

std::uint64_t ticks_now( )
{
    using std::chrono::duration_cast;
    using microsec = std::chrono::microseconds;
    auto n = std::chrono::high_resolution_clock::now( );
    return duration_cast<microsec>(n.time_since_epoch( )).count( );
}

struct client_info: public std::enable_shared_from_this<client_info> {

    using shared_type = std::shared_ptr<client_info>;

    ba::ip::udp::endpoint my_;

    udp_endpoint_atapter *parent_ = nullptr;
    std::uint64_t         last_;
    vtrc::common::delayed_call dcall_;

    client_info( const ba::ip::udp::endpoint myep,
                 boost::asio::io_service &ios )
        :my_(myep)
        ,last_(0ull)
        ,dcall_(ios)
    {
        last_ = ticks_now( );
        start_keeper( );
    }

    ba::ip::udp::endpoint &get_endpoint( )
    {
        return my_;
    }

    ~client_info( )
    {
//        std::cout << "Client out " << my_.address( ).to_string( )
//                  << ":" << my_.port( ) << "\n";
    }

    void keeper_handler( const bs::error_code &err );

    void start_keeper( )
    {
        dcall_.call_from_now( [this](const bs::error_code &err) {
            keeper_handler( err );
        }, delayed_call::seconds( 10 ) );
    }

    void on_read( const bs::error_code &err, std::uint8_t *, std::size_t );
};

using client_map = std::map<ba::ip::udp::endpoint, client_info::shared_type>;

class udp_endpoint_atapter: public udp_endpoint {

    bool         master_;
    client_map   clients_;

public:

    void add_client( const ba::ip::udp::endpoint &from,
                     client_info::shared_type cl )
    {
        clients_[from] = cl;
    }

    client_info::shared_type get_client( const ba::ip::udp::endpoint &from )
    {
        auto f = clients_.find( from );
        if( f != clients_.end( ) ) {
            return f->second;
        }
        return client_info::shared_type( );
    }

public:

    udp_endpoint_atapter( ba::io_service &ios  )
        :udp_endpoint(ios)
    { }

    std::size_t size( ) const
    {
        return clients_.size( );
    }

    void remove_client( const ba::ip::udp::endpoint &from )
    {
//        std::cout << "Erase: " << from.address( ).to_string( )
//                  << ":" << from.port( )
//                  << std::endl;
        clients_.erase( from );
        on_remove( );
    }

    virtual void on_remove( )
    {

    }

    virtual void call_client( const bs::error_code &err,
                              const ba::ip::udp::endpoint &from,
                              std::uint8_t *data, std::size_t len ) = 0;

    void on_read( const bs::error_code &err,
                  const ba::ip::udp::endpoint &from,
                  std::uint8_t *data, std::size_t len )
    {
        call_client( err, from, data, len );
        read_from( get_endpoint( ) );
    }
};

class udp_endpoint_master;

class udp_endpoint_slave: public udp_endpoint_atapter {

    ba::ip::udp::endpoint ep_;
    udp_endpoint_master  *parent_master_;

public:

    udp_endpoint_slave( ba::io_service &ios,
                        const ba::ip::udp::endpoint &ep,
                        udp_endpoint_master *master)
        :udp_endpoint_atapter(ios)
        ,ep_(ba::ip::udp::endpoint(ep.address( ), 0))
        ,parent_master_(master)
    { }


    void on_remove( );

    void start( )
    {
        ep_.address( ).is_v4( ) ? get_socket( ).open( ba::ip::udp::v4( ) )
                                : get_socket( ).open( ba::ip::udp::v6( ) );
        get_socket( ).bind( ep_ );
        ep_ = get_socket( ).local_endpoint( );
//        std::cout << "open slave ep: " << ep_.address( ).to_string( )
//                  << ":" << ep_.port( ) << "\n";
        read_from( get_endpoint( ) );
    }

    void call_client( const bs::error_code &err,
                      const ba::ip::udp::endpoint &from,
                      std::uint8_t *data, std::size_t len )
    {
        auto cl = get_client( from );
        if( cl ) {
            //std::cout << "S" << std::ends;
            cl->on_read( err, data, len );
        }
    }
};

class udp_endpoint_master: public udp_endpoint_atapter {

    ba::ip::udp::endpoint ep_;

    std::vector<std::shared_ptr<udp_endpoint_slave> > slaves_;

public:

    udp_endpoint_master( ba::io_service &ios,
                         const std::string &addr,
                         std::uint16_t port, size_t slaves )
        :udp_endpoint_atapter(ios)
        ,ep_(ba::ip::address::from_string(addr), port)
    {
        while(slaves--) {
            slaves_.push_back(std::make_shared<udp_endpoint_slave>( ios, std::cref(ep_), this ));
        }
    }

    void start( )
    {
        ep_.address( ).is_v4( ) ? get_socket( ).open( ba::ip::udp::v4( ) )
                                : get_socket( ).open( ba::ip::udp::v6( ) );
        get_socket( ).bind( ep_ );
        for( auto s: slaves_ ) {
            s->start( );
        }
        read_from( get_endpoint( ) );
    }

    void dec_slave( udp_endpoint_slave *slave )
    {
        auto f = slaves_.begin( );
        for( ; f!= slaves_.end( ); ++f) {
            if( f->get( ) == slave ) {
                break;
            }
        }
        while( f != slaves_.begin( ) ) {
            auto prev = f - 1;
            if( (*prev)->size( ) < (*f)->size( ) ) {
                std::swap( *f, *prev );
                f = prev;
            } else {
                break;
            }
        }
        for( auto i: slaves_ ) {
            std::cout << " " << i->size( ) << std::endl;
        }
    }

    void inc_slave( )
    {
        auto f = slaves_.begin( );
        while( f != slaves_.end( ) ) {
            auto next = f + 1;
            if( next != slaves_.end( ) ) {
                if( (*next)->size( ) < (*f)->size( ) ) {
                    std::swap( *f, *next );
                    f = next;
                } else {
                    break;
                }
            } else {
                break;
            }
        }

        for( auto i: slaves_ ) {
            std::cout << " " << i->size( ) << std::endl;
        }
    }

    void call_client( const bs::error_code &err,
                      const ba::ip::udp::endpoint &from,
                      std::uint8_t *data, std::size_t len )
    {
        auto cl = get_client( from );
        if( !cl ) {
            cl = std::make_shared<client_info>( from,
                                                std::ref(get_io_service( )) );
            if( (*slaves_.begin( ))->size( ) < size( ) ) {
                cl->parent_ = slaves_.begin( )->get( );
                cl->parent_->add_client( from, cl );
                inc_slave( );
            } else {
                cl->parent_ = this;
                add_client( from, cl );
                inc_slave( );
            }
        } else {
//            std::cout << "A";
//            std::cout.flush( );
        }
        cl->on_read( err, data, len );
    }
};

void udp_endpoint_slave::on_remove( )
{
    parent_master_->dec_slave( this );
}

void client_info::keeper_handler( const bs::error_code &err )
{
    if( !err ) {
        auto now = ticks_now( );
        if( now - last_ > 10000000 ) {
            parent_->dispatch( [this]( ) {
                parent_->remove_client( my_ );
            } );
        } else {
            start_keeper( );
        }
    }
}

void client_info::on_read( const bs::error_code &err,
                           std::uint8_t *, std::size_t )
{
    last_ = ticks_now( );
//    std::cout << "Got! " << my_.address( ).to_string( )
//              << ":" << my_.port( )
//              << std::endl;
    parent_->write_to( "hello!", 6, my_ );
}

int main( )
{

    try {

        ba::io_service::work wrk(ios);

        udp_endpoint_master eua( ios, "0.0.0.0", 55667, 6 );
        eua.start( );

        std::thread([ ]( ){ ios.run( ); }).detach( );
        std::thread([ ]( ){ ios.run( ); }).detach( );
        std::thread([ ]( ){ ios.run( ); }).detach( );
        std::thread([ ]( ){ ios.run( ); }).detach( );
        std::thread([ ]( ){ ios.run( ); }).detach( );

        ios.run( );

    } catch( const std::exception &ex ) {
        std::cerr << "Error " << ex.what( ) << "\n";
    }

    return 0;
}
