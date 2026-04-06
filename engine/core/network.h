#pragma once
#include "core/types.h"
#include "math/vec2.h"
#include <string>
#include <vector>
#include <functional>

namespace kairo {

// a network packet
struct Packet {
    std::vector<u8> data;
    std::string sender_address;
    u16 sender_port = 0;
};

class UDPSocket {
public:
    UDPSocket() = default;
    ~UDPSocket();

    // bind to a port for receiving
    bool bind(u16 port);

    // send data to address:port
    bool send(const std::string& address, u16 port, const void* data, size_t size);

    // receive a packet (non-blocking, returns false if no data)
    bool receive(Packet& packet);

    void close();
    bool is_open() const;

private:
    int m_socket = -1;
};

// simple entity state for network sync
struct NetEntityState {
    u32 entity_id = 0;
    Vec2 position;
    Vec2 velocity;
    float rotation = 0.0f;
};

// serialize/deserialize entity states to bytes
void serialize_state(const NetEntityState& state, std::vector<u8>& out);
bool deserialize_state(const u8* data, size_t size, NetEntityState& out);

} // namespace kairo
