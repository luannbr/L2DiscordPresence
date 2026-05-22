//  L2DiscordPresence-IPC
//
//  Discord Rich Presence via local named-pipe IPC, no Discord Game SDK required.
//  Supports up to 2 link buttons (visible to other users on your profile).
//
//  Build-time configurable values below are patched by patch.ps1 from presence.ini.

#include "pch/pch.h"

// === build-time config (do not move these lines; patch.ps1 relies on them) ===
constexpr const char* APP_ID        = "0000000000000000000";
constexpr const char* DETAILS       = "Playing Lineage 2";
constexpr const char* STATE         = "your-server.com";
constexpr const char* LARGE_IMAGE   = "test_big_pic";
constexpr const char* LARGE_TEXT    = "big text";
constexpr const char* SMALL_IMAGE   = "test_smol_pic";
constexpr const char* SMALL_TEXT    = "smol text";
constexpr const char* BUTTON1_LABEL = "";
constexpr const char* BUTTON1_URL   = "";
constexpr const char* BUTTON2_LABEL = "";
constexpr const char* BUTTON2_URL   = "";
// === /build-time config ===

namespace {

constexpr DWORD OP_HANDSHAKE = 0;
constexpr DWORD OP_FRAME     = 1;
constexpr DWORD OP_PING      = 3;
constexpr DWORD OP_PONG      = 4;

HANDLE g_pipe = INVALID_HANDLE_VALUE;

bool isSet( const char* s ) { return s && s[0] != '\0'; }

bool openDiscordPipe()
{
    char name[64];
    for ( int i = 0; i <= 9; ++i )
    {
        sprintf( name, "\\\\.\\pipe\\discord-ipc-%d", i );
        HANDLE h = CreateFileA( name, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr );
        if ( h != INVALID_HANDLE_VALUE )
        {
            g_pipe = h;
            return true;
        }
    }
    return false;
}

bool writeFrame( DWORD opcode, const std::string& payload )
{
    if ( g_pipe == INVALID_HANDLE_VALUE ) return false;
    DWORD len = (DWORD)payload.size();
    DWORD written = 0;
    if ( !WriteFile( g_pipe, &opcode, 4, &written, nullptr ) || written != 4 ) return false;
    if ( !WriteFile( g_pipe, &len,    4, &written, nullptr ) || written != 4 ) return false;
    if ( len > 0 )
    {
        if ( !WriteFile( g_pipe, payload.data(), len, &written, nullptr ) || written != len ) return false;
    }
    return true;
}

bool readFrame( DWORD& opcode, std::string& payload )
{
    if ( g_pipe == INVALID_HANDLE_VALUE ) return false;
    DWORD got = 0;
    DWORD len = 0;
    if ( !ReadFile( g_pipe, &opcode, 4, &got, nullptr ) || got != 4 ) return false;
    if ( !ReadFile( g_pipe, &len,    4, &got, nullptr ) || got != 4 ) return false;
    payload.assign( len, '\0' );
    if ( len > 0 )
    {
        if ( !ReadFile( g_pipe, &payload[0], len, &got, nullptr ) || got != len ) return false;
    }
    return true;
}

std::string jsonEscape( const char* s )
{
    std::string out;
    if ( !s ) return out;
    for ( const char* p = s; *p; ++p )
    {
        unsigned char c = (unsigned char)*p;
        switch ( c )
        {
        case '"':  out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b";  break;
        case '\f': out += "\\f";  break;
        case '\n': out += "\\n";  break;
        case '\r': out += "\\r";  break;
        case '\t': out += "\\t";  break;
        default:
            if ( c < 0x20 )
            {
                char buf[8];
                sprintf( buf, "\\u%04x", c );
                out += buf;
            }
            else
            {
                out += (char)c;
            }
        }
    }
    return out;
}

std::string buildActivityPayload()
{
    int64_t startTs = std::chrono::duration_cast< std::chrono::seconds >(
        std::chrono::system_clock::now().time_since_epoch() ).count();

    std::ostringstream o;
    o << "{\"cmd\":\"SET_ACTIVITY\",\"args\":{\"pid\":" << GetCurrentProcessId()
      << ",\"activity\":{";

    bool needComma = false;
    auto addStr = [ & ]( const char* k, const char* v )
    {
        if ( !isSet( v ) ) return;
        if ( needComma ) o << ",";
        o << "\"" << k << "\":\"" << jsonEscape( v ) << "\"";
        needComma = true;
    };

    addStr( "details", DETAILS );
    addStr( "state",   STATE );

    if ( needComma ) o << ",";
    o << "\"timestamps\":{\"start\":" << startTs << "}";
    needComma = true;

    if ( isSet( LARGE_IMAGE ) || isSet( LARGE_TEXT ) || isSet( SMALL_IMAGE ) || isSet( SMALL_TEXT ) )
    {
        o << ",\"assets\":{";
        bool first = true;
        auto addA = [ & ]( const char* k, const char* v )
        {
            if ( !isSet( v ) ) return;
            if ( !first ) o << ",";
            o << "\"" << k << "\":\"" << jsonEscape( v ) << "\"";
            first = false;
        };
        addA( "large_image", LARGE_IMAGE );
        addA( "large_text",  LARGE_TEXT );
        addA( "small_image", SMALL_IMAGE );
        addA( "small_text",  SMALL_TEXT );
        o << "}";
    }

    std::string buttons;
    auto addBtn = [ & ]( const char* label, const char* url )
    {
        if ( !isSet( label ) || !isSet( url ) ) return;
        if ( !buttons.empty() ) buttons += ",";
        buttons += "{\"label\":\"";
        buttons += jsonEscape( label );
        buttons += "\",\"url\":\"";
        buttons += jsonEscape( url );
        buttons += "\"}";
    };
    addBtn( BUTTON1_LABEL, BUTTON1_URL );
    addBtn( BUTTON2_LABEL, BUTTON2_URL );
    if ( !buttons.empty() )
    {
        o << ",\"buttons\":[" << buttons << "]";
    }

    o << "}},\"nonce\":\"set-activity-1\"}";
    return o.str();
}

DWORD APIENTRY worker( LPVOID )
{
    // Discord may not be running yet; retry for ~10 minutes.
    for ( int tries = 0; tries < 600; ++tries )
    {
        if ( openDiscordPipe() ) break;
        Sleep( 1000 );
    }
    if ( g_pipe == INVALID_HANDLE_VALUE ) return 0;

    {
        std::ostringstream hs;
        hs << "{\"v\":1,\"client_id\":\"" << APP_ID << "\"}";
        if ( !writeFrame( OP_HANDSHAKE, hs.str() ) ) return 0;
    }

    DWORD       op = 0;
    std::string payload;
    if ( !readFrame( op, payload ) ) return 0;

    writeFrame( OP_FRAME, buildActivityPayload() );

    // Keep the pipe open so Discord doesn't clear the activity; reply to pings.
    while ( true )
    {
        if ( !readFrame( op, payload ) ) break;
        if ( op == OP_PING ) writeFrame( OP_PONG, payload );
    }

    return 0;
}

}  // namespace

extern "C" __declspec( dllexport ) int APIENTRY Anchor()
{
    return 0;
}

bool APIENTRY DllMain( const HINSTANCE hinstDLL
                     , const DWORD     fdwReason
                     , LPVOID )
{
    if ( fdwReason == DLL_PROCESS_ATTACH )
    {
        DisableThreadLibraryCalls( hinstDLL );
        CreateThread( nullptr, 0, worker, nullptr, 0, nullptr );
    }
    return true;
}
