#ifndef OSC_RECEIVER_H
#define OSC_RECEIVER_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <functional>

#if defined( WIN32 )
    #include <winsock2.h>
    #include <ws2tcpip.h>
    typedef int socklen_t ;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <fcntl.h>
    #include <unistd.h>
#endif

struct OscArg
{
    char  type ; // 'f' or 'i'
    union
    {
        float   f ;
        int32_t i ;
    } ;
} ;

class OscReceiver
{
public:
    typedef std::function< void( const std::string & , const std::vector< OscArg > & ) > MessageHandler ;

    OscReceiver() : mSocketFd( -1 ) {}
    ~OscReceiver() { Close() ; }

    bool Start( uint16_t port )
        {
    #if defined( WIN32 )
            WSADATA wsaData ;
            WSAStartup( MAKEWORD( 2 , 2 ) , & wsaData ) ;
    #endif
            mSocketFd = ::socket( AF_INET , SOCK_DGRAM , 0 ) ;
            if( mSocketFd < 0 ) return false ;

            int reuse = 1 ;
            ::setsockopt( mSocketFd , SOL_SOCKET , SO_REUSEADDR , reinterpret_cast< const char * >( & reuse ) , sizeof( reuse ) ) ;

    #if defined( WIN32 )
            u_long mode = 1 ;
            ioctlsocket( mSocketFd , FIONBIO , & mode ) ;
    #else
            int flags = ::fcntl( mSocketFd , F_GETFL , 0 ) ;
            ::fcntl( mSocketFd , F_SETFL , flags | O_NONBLOCK ) ;
    #endif

            sockaddr_in addr ;
            memset( & addr , 0 , sizeof( addr ) ) ;
            addr.sin_family      = AF_INET ;
            addr.sin_addr.s_addr = htonl( INADDR_ANY ) ;
            addr.sin_port        = htons( port ) ;

            if( ::bind( mSocketFd , reinterpret_cast< sockaddr * >( & addr ) , sizeof( addr ) ) < 0 )
            {
                Close() ;
                return false ;
            }
            printf( "OSC Receiver listening on UDP port %u\n" , port ) ;
            return true ;
        }

    void Close()
        {
            if( mSocketFd >= 0 )
            {
    #if defined( WIN32 )
                ::closesocket( mSocketFd ) ;
    #else
                ::close( mSocketFd ) ;
    #endif
                mSocketFd = -1 ;
            }
        }

    void Poll( const MessageHandler & handler )
        {
            if( mSocketFd < 0 ) return ;
            uint8_t buffer[ 4096 ] ;
            for( ;; )
            {
                sockaddr_in sender ;
                socklen_t senderLen = sizeof( sender ) ;
                int bytesRead = ::recvfrom( mSocketFd , reinterpret_cast< char * >( buffer ) , sizeof( buffer ) , 0 ,
                                            reinterpret_cast< sockaddr * >( & sender ) , & senderLen ) ;
                if( bytesRead <= 0 ) break ;
                ParsePacket( buffer , static_cast< size_t >( bytesRead ) , handler ) ;
            }
        }

private:
    int mSocketFd ;

    static size_t Align4( size_t n ) { return ( n + 3 ) & ~size_t( 3 ) ; }

    static uint32_t ReadBE32( const uint8_t * p )
    {
        return ( uint32_t( p[0] ) << 24 ) |
               ( uint32_t( p[1] ) << 16 ) |
               ( uint32_t( p[2] ) <<  8 ) |
               ( uint32_t( p[3] )       ) ;
    }

    static void ParsePacket( const uint8_t * data , size_t size , const MessageHandler & handler )
    {
        if( size < 4 ) return ;

        // Handle OSC #bundle packets recursively
        if( size >= 16 && memcmp( data , "#bundle\0" , 8 ) == 0 )
        {
            size_t offset = 16 ; // Skip "#bundle\0" (8) + timetag (8)
            while( offset + 4 <= size )
            {
                uint32_t elemSize = ReadBE32( data + offset ) ;
                offset += 4 ;
                if( offset + elemSize > size ) break ;
                ParsePacket( data + offset , elemSize , handler ) ;
                offset += elemSize ;
            }
            return ;
        }

        // Parse standard OSC message: address, type tag string, arguments
        size_t addrLen = strnlen( reinterpret_cast< const char * >( data ) , size ) ;
        if( addrLen == size ) return ;
        std::string address( reinterpret_cast< const char * >( data ) , addrLen ) ;

        size_t offset = Align4( addrLen + 1 ) ;
        if( offset >= size || data[ offset ] != ',' ) return ;

        size_t tagsLen = strnlen( reinterpret_cast< const char * >( data + offset ) , size - offset ) ;
        if( offset + tagsLen == size ) return ;
        std::string typeTags( reinterpret_cast< const char * >( data + offset + 1 ) , tagsLen - 1 ) ;

        offset += Align4( tagsLen + 1 ) ;

        std::vector< OscArg > args ;
        for( char tag : typeTags )
        {
            if( tag == 'f' )
            {
                if( offset + 4 > size ) break ;
                uint32_t raw = ReadBE32( data + offset ) ;
                OscArg arg ;
                arg.type = 'f' ;
                memcpy( & arg.f , & raw , sizeof( float ) ) ;
                args.push_back( arg ) ;
                offset += 4 ;
            }
            else if( tag == 'i' )
            {
                if( offset + 4 > size ) break ;
                uint32_t raw = ReadBE32( data + offset ) ;
                OscArg arg ;
                arg.type = 'i' ;
                arg.i = static_cast< int32_t >( raw ) ;
                args.push_back( arg ) ;
                offset += 4 ;
            }
        }
        handler( address , args ) ;
    }
} ;

#endif