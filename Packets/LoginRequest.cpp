#include "LoginRequest.h"
#include<endian.h>
#include<iostream>
#include<string.h>

#include<string>
#include<locale>
#include<codecvt>

#include "Functions.h"

#include "Server.h"
#include "Entity/Entity.h"
#include "Config.h"
#include "SpawnPosition.h"
#include "PlayerAbilities.h"
#include "TimeUpdate.h"
#include "ChunkAllocation.h"
#include "ChunkData.h"
#include "Keepalive.h"
#include "PlayerPositionandLook.h"
#include "SetWindowItems.h"

LoginRequest::LoginRequest(unsigned char* data, int len, Client* c): Packet(0x01)
{
    ProtocolVersion = be32toh(*(int*)data);
    data += 4;
    std::cout << "Protocol version: " << ProtocolVersion << std::endl;
    short namelen = be16toh(*(short*)data);
    data += 2;
    char16_t tmp[namelen+1];
    tmp[namelen] = 0;
    processUnicodes(tmp, data, namelen);
    username = std::u16string(tmp);
    
    std::wstring_convert<std::codecvt_utf16<wchar_t, 0x10ffff, std::little_endian>,wchar_t> conv;
    std::wstring test = conv.from_bytes(
        reinterpret_cast<const char*>(&username[0]),
        reinterpret_cast<const char*>(&username[0]+username.size()));
    std::wcout << "username: " << test << std::endl;
    

}

LoginRequest::LoginRequest(int EntId, std::u16string LevelType, int ServerMode, int Dimension, char Difficulty, unsigned char MaxPLayers): Packet(0x01)
{
    EntityID = EntId;
    this->LevelType = LevelType;
    this->ServerMode = ServerMode;
    this->Dimension = Dimension;
    this->Difficulty = Difficulty;
    this->MaxPlayers = MaxPLayers;


}

char* LoginRequest::build()
{
    int tmp_len = LevelType.length()*2;
    int l = 20 + tmp_len;
    this->len=l;
    char* resp = new char[this->len];
    resp[0] = OPCode;
    *(int*)(resp+1) = htobe32(EntityID);

    *(short*)(resp+5) = (short)0;

    *(short*)(resp+7) = htobe16((short)tmp_len/2);

    
    processUnicodes((char16_t*)(resp+9), (unsigned char*)LevelType.c_str(), LevelType.length());

    
    *(int*)(resp+9+tmp_len) = htobe32(ServerMode);
    *(int*)(resp+13+tmp_len) = htobe32(Dimension);
    *(resp+17+tmp_len) = Difficulty;
    *(resp+18+tmp_len) = (char)0;
    *(resp+19+tmp_len) = MaxPlayers;
    return resp;

}

void LoginRequest::Response(Client* c)
{
    std::cout << "S->C LoginRequest" << std::endl;
    Player e = Player();
    e.generateID();
    c->player = e;
    Server::addEntity(e);
    LoginRequest req(e.getEntityID(), Config::LevelType, Config::ServerMode, 0, Config::Difficulty, Config::MaxPlayers);
    char* t = req.build();
    c->writeBytes(t, req.len);

    //Send spawn position
    SpawnPosition s(Config::SpawnX, Config::SpawnY, Config::SpawnZ);
    s.Send(c);

    PlayerAbilities abilities(false, false, false, false);
    abilities.Send(c);

    TimeUpdate timeupdate(Server::getTime());
    timeupdate.Send(c);

    Keepalive keepalive;
    keepalive.Send(c);

    SetWindowItems setWindowItems(0, Inventory::SlotCount, c->player.inventory.slots);
    setWindowItems.Send(c);

    for (int z=Config::SpawnZ-(5*16);z<Config::SpawnZ+(5*16); z+=16)
    {
        for (int x=Config::SpawnX-(5*16);x<Config::SpawnX+(5*16); x+=16)
        {
            Server::overworld->LoadRegion(x>>9,z>>9);
            ChunkAllocation alloc(x/16,z/16,true);
            alloc.Send(c);
            /*ChunkData data(x,z, Server::overworld);
            data.Send(c);*/
            std::cout << "Sent out Chunk data to " << x << " " << z << std::endl;
        }
    }

    PlayerPositionandLook ppan;
    ppan.copyFromPlayer(&(c->player));
    ppan.Send(c);

    

}