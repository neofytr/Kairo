#include "core/network.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>

namespace kairo {

UDPSocket::~UDPSocket() {
    close();
}

bool UDPSocket::bind(u16 port) {
    if (m_socket >= 0) close();

    m_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
    if (m_socket < 0) return false;

    // set non-blocking
    int flags = ::fcntl(m_socket, F_GETFL, 0);
    if (flags < 0 || ::fcntl(m_socket, F_SETFL, flags | O_NONBLOCK) < 0) {
        close();
        return false;
    }

    // allow address reuse
    int opt = 1;
    ::setsockopt(m_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (::bind(m_socket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) {
        close();
        return false;
    }

    return true;
}

bool UDPSocket::send(const std::string& address, u16 port, const void* data, size_t size) {
    // create socket on demand if not yet open
    if (m_socket < 0) {
        m_socket = ::socket(AF_INET, SOCK_DGRAM, 0);
        if (m_socket < 0) return false;

        int flags = ::fcntl(m_socket, F_GETFL, 0);
        if (flags >= 0) ::fcntl(m_socket, F_SETFL, flags | O_NONBLOCK);
    }

    sockaddr_in dest{};
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);
    if (::inet_pton(AF_INET, address.c_str(), &dest.sin_addr) <= 0) {
        return false;
    }

    ssize_t sent = ::sendto(m_socket, data, size, 0,
                            reinterpret_cast<sockaddr*>(&dest), sizeof(dest));
    return sent >= 0;
}

bool UDPSocket::receive(Packet& packet) {
    if (m_socket < 0) return false;

    u8 buffer[65536];
    sockaddr_in from{};
    socklen_t from_len = sizeof(from);

    ssize_t received = ::recvfrom(m_socket, buffer, sizeof(buffer), 0,
                                  reinterpret_cast<sockaddr*>(&from), &from_len);

    if (received <= 0) return false; // EAGAIN/EWOULDBLOCK or error

    packet.data.assign(buffer, buffer + received);

    char addr_str[INET_ADDRSTRLEN];
    ::inet_ntop(AF_INET, &from.sin_addr, addr_str, sizeof(addr_str));
    packet.sender_address = addr_str;
    packet.sender_port = ntohs(from.sin_port);

    return true;
}

void UDPSocket::close() {
    if (m_socket >= 0) {
        ::close(m_socket);
        m_socket = -1;
    }
}

bool UDPSocket::is_open() const {
    return m_socket >= 0;
}

// -- serialization helpers --

static void write_bytes(std::vector<u8>& out, const void* src, size_t n) {
    const u8* p = static_cast<const u8*>(src);
    out.insert(out.end(), p, p + n);
}

static bool read_bytes(const u8*& ptr, size_t& remaining, void* dst, size_t n) {
    if (remaining < n) return false;
    std::memcpy(dst, ptr, n);
    ptr += n;
    remaining -= n;
    return true;
}

void serialize_state(const NetEntityState& state, std::vector<u8>& out) {
    // u32 entity_id + 2 floats position + 2 floats velocity + float rotation = 24 bytes
    out.reserve(out.size() + 24);
    write_bytes(out, &state.entity_id, sizeof(u32));
    write_bytes(out, &state.position.x, sizeof(float));
    write_bytes(out, &state.position.y, sizeof(float));
    write_bytes(out, &state.velocity.x, sizeof(float));
    write_bytes(out, &state.velocity.y, sizeof(float));
    write_bytes(out, &state.rotation, sizeof(float));
}

bool deserialize_state(const u8* data, size_t size, NetEntityState& out) {
    const u8* ptr = data;
    size_t remaining = size;

    if (!read_bytes(ptr, remaining, &out.entity_id, sizeof(u32))) return false;
    if (!read_bytes(ptr, remaining, &out.position.x, sizeof(float))) return false;
    if (!read_bytes(ptr, remaining, &out.position.y, sizeof(float))) return false;
    if (!read_bytes(ptr, remaining, &out.velocity.x, sizeof(float))) return false;
    if (!read_bytes(ptr, remaining, &out.velocity.y, sizeof(float))) return false;
    if (!read_bytes(ptr, remaining, &out.rotation, sizeof(float))) return false;

    return true;
}

} // namespace kairo
