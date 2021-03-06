#include "host.hpp"
#include "net.hpp"
#include "kommando.hpp"

#include <nada/log.hpp>
#include <cereal/types/vector.hpp>
#include <cereal/types/unordered_map.hpp>
#include <cereal/types/memory.hpp>
#include <SFML/System/Clock.hpp>

Host::Host(uint16_t port) {
    ENetAddress address {
        ENET_HOST_ANY,
        port
    };
    if (server = enet_host_create(&address, 32, // Max. Klienten
                                  2, // Kanäle; 0 = Request/Receive, 1 = Broadcast
                                  0, 0); !server) {
        nada::Log::err() << "An error occurred while trying to create an ENet server host.\n";
        exit(EXIT_FAILURE);
    }
}

Host::~Host() {
    stop();
    enet_host_destroy(server);
}

void Host::start() {
    while (loop) {
        welt.tick(); // TODO benchmark ms

        //Log::out() << "Server eventloop" << Log::endl;
        ENetEvent event;
        while(enet_host_service(server, &event, 50) > 0) switch (event.type) {
            case ENET_EVENT_TYPE_CONNECT:
                printf("A new client connected from %x:%u.\n",
                       event.peer->address.host,
                       event.peer->address.port);
                //event.peer->data = "Client information";
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                handle_receive(event);
                break;
            case ENET_EVENT_TYPE_DISCONNECT:
                printf("%u disconnected.\n", event.peer->address.host);
                event.peer->data = nullptr; // delete wenn nötig
                break;
            case ENET_EVENT_TYPE_NONE: break;
        }

        // Broadcast
        if (static sf::Clock timer; timer.getElapsedTime().asMilliseconds() > 1000) {
            std::stringstream ss;
            Net::Serializer s(ss);
            s << Net::BROADCAST;
            s << welt.timelapse << welt.teams << welt.zonen << welt.abschuesse;
            const std::string data(ss.str());
            ENetPacket* paket = enet_packet_create(data.c_str(), data.size(),
                                                   ENET_PACKET_FLAG_UNRELIABLE_FRAGMENT);
            enet_host_broadcast(server, 1, paket);
            enet_host_flush(server);
            timer.restart();
        }
    }
}

void Host::stop() {
    loop = false;
}

void Host::handle_receive(ENetEvent& event) {
    Net::Request request;
    std::string data((const char*) event.packet->data, event.packet->dataLength);
    //Log::debug() << "Host::handle_receive paket_daten = " << data.size() << Log::endl;
    std::stringstream ss(data);
    Net::Deserializer ds(ss);
    ds >> request;

    switch (request) {
        case Net::SUB_CMD: {
            Kommando kommando;
            ds >> kommando;
            kommando.apply(&welt);
            enet_packet_destroy(event.packet); // Aufräumen
        } break;
        case Net::AKTION_NEUES_UBOOT: {
            oid_t team;
            ds >> team;
            sende_antwort(event, Net::serialize(*welt.add_new_sub(team, false)));
        } break;
        case Net::REQUEST_SUB: {
            oid_t sub_id;
            ds >> sub_id;
            if (welt.objekte.count(sub_id)) sende_antwort(event, Net::serialize(*(const Sub*) welt.objekte[sub_id].get()));
            else sende_antwort(event, "");
        } break;
        case Net::ALLE_OBJEKTE: {
            static auto no_delete = [](Objekt*) {}; // Workaround für cereal - Objekte nicht löschen
            std::vector<std::unique_ptr<Objekt, decltype(no_delete)>> objekte;
            objekte.reserve(welt.objekte.size());
            for (const auto& paar : welt.objekte) {
                decltype(objekte)::value_type ptr(paar.second.get(), no_delete);
                objekte.push_back(std::move(ptr));
            }
            sende_antwort(event, Net::serialize(objekte));
        } break;
        case Net::NEUER_KLIENT: {
            const std::tuple<Karte> daten = {welt.karte};
            sende_antwort(event, Net::serialize(daten));
        } break;
        default: nada::Log::err() << "\tUnknown Request Net::" << (int)request << '\n'; break;
    }
}

void Host::sende_antwort(ENetEvent& event, const std::string& response) {
    //Log::debug() << "Host::sende_antwort paket_daten=" << response.size() << Log::endl;
    ENetPacket* paket = event.packet;
    enet_packet_resize(paket, response.size());
    std::copy(response.begin(), response.end(), paket->data);
    enet_peer_send(event.peer, 0, paket);
    enet_host_flush(server);
}
