#include "Core.h"
#include "Console.h"
#include "Export.h"
#include "PluginManager.h"
#include "DataDefs.h"

#include <vector>
#include <string>
#include <map>
#include <memory>
#include <PassiveSocket.h>
#include <ActiveSocket.h>
#include "MiscUtils.h"
#include "LuaTools.h"
#include "DataFuncs.h"
#include <stdexcept> //todo convert errors to lua-errors and co. Then remove this

using namespace DFHack;
using namespace df::enums;
struct server
{
    CPassiveSocket *socket;
    std::map<int,CActiveSocket*> clients;
    int last_client_id;
    void close();
};
std::map<int,server> servers;
typedef std::map<int,CActiveSocket*> clients_map;
clients_map clients; //free clients, i.e. non-server spawned clients
DFHACK_PLUGIN("luasocket");


void server::close()
{
    for(auto it=clients.begin();it!=clients.end();it++)
    {
        CActiveSocket* sock=it->second;
        sock->Close();
        delete sock;
    }
    clients.clear();
    socket->Close();
    delete socket;
}
std::pair<CActiveSocket*,clients_map*> get_client(int server_id,int client_id)
{
    std::map<int,CActiveSocket*>* target=&clients;
    if(server_id>0)
    {
        if(servers.count(server_id)==0)
        {
            throw std::runtime_error("Server with this id does not exist");
        }
        server &cur_server=servers[server_id];
        target=&cur_server.clients;
    }

    if(target->count(client_id)==0)
    {
        throw std::runtime_error("Client does with this id not exist");
    }
    CActiveSocket *sock=(*target)[client_id];
    return std::make_pair(sock,target);
}
void handle_error(CSimpleSocket::CSocketError err,bool skip_timeout=true)
{
    if (err == CSimpleSocket::SocketSuccess)
        return;
    if (err == CSimpleSocket::SocketTimedout && skip_timeout)
        return;
    if (err == CSimpleSocket::SocketEwouldblock && skip_timeout)
        return;
    throw std::runtime_error(CSimpleSocket::DescribeError(err));
}
static int lua_socket_bind(std::string ip,int port)
{
    static int server_id=0;
    CPassiveSocket* sock=new CPassiveSocket;
    if(!sock->Initialize())
    {
        CSimpleSocket::CSocketError err=sock->GetSocketError();
        delete sock;
        handle_error(err,false);
    }
    sock->SetBlocking();
    if(!sock->Listen((uint8_t*)ip.c_str(),port))
    {
        handle_error(sock->GetSocketError(),false);
    }
    server_id++;
    server& cur_server=servers[server_id];
    cur_server.socket=sock;
    cur_server.last_client_id=0;
    return server_id;
}
static int lua_server_accept(int id,bool fail_on_timeout)
{
    if(servers.count(id)==0)
    {
        throw std::runtime_error("Server not bound");
    }
    server &cur_server=servers[id];
    CActiveSocket* sock=cur_server.socket->Accept();
    if(!sock)
    {
        handle_error(sock->GetSocketError(),!fail_on_timeout);
        return 0;
    }
    else
    {
        cur_server.last_client_id++;
        cur_server.clients[cur_server.last_client_id]=sock;
        return cur_server.last_client_id;
    }
}
static void lua_client_close(int server_id,int client_id)
{
    auto info=get_client(server_id,client_id);

    CActiveSocket *sock=info.first;
    std::map<int,CActiveSocket*>* target=info.second;

    target->erase(client_id);
    CSimpleSocket::CSocketError err=CSimpleSocket::SocketSuccess;
    if(!sock->Close())
        err=sock->GetSocketError();
    delete sock;
    if(err!=CSimpleSocket::SocketSuccess)
    {
        throw std::runtime_error(CSimpleSocket::DescribeError(err));
    }
}
static void lua_server_close(int server_id)
{
    if(servers.count(server_id)==0)
    {
        throw std::runtime_error("Server with this id does not exist");
    }
    server &cur_server=servers[server_id];
    try{
        cur_server.close();
    }
    catch(...)
    {
        servers.erase(server_id);
        throw;
    }
}
static std::string lua_client_receive(int server_id,int client_id,int bytes,std::string pattern,bool fail_on_timeout)
{
    auto info=get_client(server_id,client_id);
    CActiveSocket *sock=info.first;
    if(bytes>0)
    {
        if(sock->Receive(bytes)<=0)
        {
            throw std::runtime_error(sock->DescribeError());
        }
        return std::string((char*)sock->GetData(),bytes);
    }
    else
    {
        std::string ret;
        if(pattern=="*a") //??
        {
            while(true)
            {
                int received=sock->Receive(1);
                if(received<0)
                {
                    handle_error(sock->GetSocketError(),!fail_on_timeout);
                    return "";//maybe return partial string?
                }
                else if(received==0)
                {
                    break;
                }
                ret+=(char)*sock->GetData();

            }
            return ret;
        }
        else if (pattern=="" || pattern=="*l")
        {
            while(true)
            {

                if(sock->Receive(1)<=0)
                {
                    handle_error(sock->GetSocketError(),!fail_on_timeout);
                    return "";//maybe return partial string?
                }
                char rec=(char)*sock->GetData();
                if(rec=='\n')
                    break;
                ret+=rec;
            }
            return ret;
        }
        else
        {
            throw std::runtime_error("Unsupported receive pattern");
        }
    }
}
static void lua_client_send(int server_id,int client_id,std::string data)
{
    if(data.size()==0)
        return;
    std::map<int,CActiveSocket*>* target=&clients;
    if(server_id>0)
    {
        if(servers.count(server_id)==0)
        {
            throw std::runtime_error("Server with this id does not exist");
        }
        server &cur_server=servers[server_id];
        target=&cur_server.clients;
    }

    if(target->count(client_id)==0)
    {
        throw std::runtime_error("Client does with this id not exist");
    }
    CActiveSocket *sock=(*target)[client_id];
    if(sock->Send((const uint8_t*)data.c_str(),data.size())!=data.size())
    {
        throw std::runtime_error(sock->DescribeError());
    }
}
static int lua_socket_connect(std::string ip,int port)
{
    static int last_client_id=0;
    CActiveSocket* sock=new CActiveSocket;
    if(!sock->Initialize())
    {
        CSimpleSocket::CSocketError err=sock->GetSocketError();
        delete sock;
        throw std::runtime_error(CSimpleSocket::DescribeError(err));
    }
    if(!sock->Open((const uint8_t*)ip.c_str(),port))
    {
        CSimpleSocket::CSocketError err=sock->GetSocketError();
        delete sock;
        throw std::runtime_error(CSimpleSocket::DescribeError(err));
    }
    sock->SetNonblocking();
    last_client_id++;
    clients[last_client_id]=sock;
    return last_client_id;
}
CSimpleSocket* get_socket(int server_id, int client_id)
{
    std::map<int, CActiveSocket*>* target = &clients;
    if (server_id>0)
    {
        if (servers.count(server_id) == 0)
        {
            throw std::runtime_error("Server with this id does not exist");
        }
        server &cur_server = servers[server_id];
        if (client_id == -1)
        {
            return cur_server.socket;
        }
        target = &cur_server.clients;
    }

    if (target->count(client_id) == 0)
    {
        throw std::runtime_error("Client does with this id not exist");
    }
    CActiveSocket *sock = (*target)[client_id];
    return sock;
}
static void lua_socket_set_timeout(int server_id,int client_id,int32_t sec,int32_t msec)
{
    CSimpleSocket *sock = get_socket(server_id, client_id);
    sock->SetConnectTimeout(sec,msec);
    if (!sock->SetReceiveTimeout(sec, msec) ||
        !sock->SetSendTimeout(sec, msec))
    {
        CSimpleSocket::CSocketError err = sock->GetSocketError();
        throw std::runtime_error(CSimpleSocket::DescribeError(err));
    }
}
static bool lua_socket_select(int server_id, int client_id, int32_t sec, int32_t msec)
{
    CSimpleSocket *sock = get_socket(server_id, client_id);
    return sock->Select(sec, msec);
}
static void lua_socket_set_blocking(int server_id, int client_id, bool value)
{
    CSimpleSocket *sock = get_socket(server_id, client_id);
    bool ok;
    if (value)
    {
        ok = sock->SetBlocking();
    }
    else
    {
        ok = sock->SetNonblocking();
    }
    if (!ok)
    {
        CSimpleSocket::CSocketError err = sock->GetSocketError();
        throw std::runtime_error(CSimpleSocket::DescribeError(err));
    }
}
static bool lua_socket_is_blocking(int server_id, int client_id)
{
    CSimpleSocket *sock = get_socket(server_id, client_id);
    return !sock->IsNonblocking();
}
DFHACK_PLUGIN_LUA_FUNCTIONS {
    DFHACK_LUA_FUNCTION(lua_socket_bind), //spawn a server
    DFHACK_LUA_FUNCTION(lua_socket_connect),//spawn a client (i.e. connection)
    DFHACK_LUA_FUNCTION(lua_socket_select),
    DFHACK_LUA_FUNCTION(lua_socket_set_blocking),
    DFHACK_LUA_FUNCTION(lua_socket_is_blocking),
    DFHACK_LUA_FUNCTION(lua_socket_set_timeout),
    DFHACK_LUA_FUNCTION(lua_server_accept),
    DFHACK_LUA_FUNCTION(lua_server_close),
    DFHACK_LUA_FUNCTION(lua_client_close),
    DFHACK_LUA_FUNCTION(lua_client_send),
    DFHACK_LUA_FUNCTION(lua_client_receive),
    DFHACK_LUA_END
};
DFhackCExport command_result plugin_init ( color_ostream &out, std::vector <PluginCommand> &commands)
{

    return CR_OK;
}
DFhackCExport command_result plugin_shutdown ( color_ostream &out )
{
    for(auto it=clients.begin();it!=clients.end();it++)
    {
        CActiveSocket* sock=it->second;
        sock->Close();
        delete sock;
    }
    clients.clear();
    for(auto it=servers.begin();it!=servers.end();it++)
    {
        it->second.close();
    }
    servers.clear();
    return CR_OK;
}
