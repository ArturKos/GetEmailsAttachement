#include "pop3_client.h"

#include <gtest/gtest.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <atomic>
#include <cstring>
#include <string>
#include <thread>

namespace {

class FakePop3Server {
public:
    FakePop3Server() = default;

    /** Start a server on an ephemeral port that runs @p script when accepting. */
    void start(std::function<void(int)> script)
    {
        listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        ASSERT_GE(listen_fd_, 0);
        int reuse = 1;
        setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port        = 0;
        ASSERT_EQ(bind(listen_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)), 0);
        ASSERT_EQ(listen(listen_fd_, 1), 0);

        socklen_t addr_len = sizeof(addr);
        ASSERT_EQ(getsockname(listen_fd_, reinterpret_cast<sockaddr *>(&addr), &addr_len), 0);
        port_ = ntohs(addr.sin_port);

        worker_ = std::thread([this, script = std::move(script)] {
            int client_fd = accept(listen_fd_, nullptr, nullptr);
            if (client_fd >= 0) {
                script(client_fd);
                close(client_fd);
            }
        });
    }

    ~FakePop3Server()
    {
        if (worker_.joinable()) {
            worker_.join();
        }
        if (listen_fd_ >= 0) {
            close(listen_fd_);
        }
    }

    std::string port_string() const { return std::to_string(port_); }

private:
    int listen_fd_ = -1;
    uint16_t port_ = 0;
    std::thread worker_;
};

/** Send literal data, ignoring partial-write edge cases (test scope). */
void server_send(int fd, const std::string &data)
{
    send(fd, data.data(), data.size(), 0);
}

/** Read until CRLF, return the line without trailing CRLF. */
std::string server_recv_line(int fd)
{
    std::string line;
    char c = 0;
    while (recv(fd, &c, 1, 0) == 1) {
        if (c == '\n') {
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            return line;
        }
        line.push_back(c);
    }
    return line;
}

} // namespace

TEST(Pop3Client, ConnectAndLoginSucceed)
{
    FakePop3Server server;
    server.start([](int fd) {
        server_send(fd, "+OK ready\r\n");
        std::string user_line = server_recv_line(fd);
        EXPECT_EQ(user_line, "USER alice");
        server_send(fd, "+OK\r\n");
        std::string pass_line = server_recv_line(fd);
        EXPECT_EQ(pass_line, "PASS secret");
        server_send(fd, "+OK logged in\r\n");
        std::string quit_line = server_recv_line(fd);
        EXPECT_EQ(quit_line, "QUIT");
        server_send(fd, "+OK bye\r\n");
    });

    int socket_fd = pop3_connect("127.0.0.1", server.port_string().c_str());
    ASSERT_GE(socket_fd, 0);
    EXPECT_TRUE(pop3_login(socket_fd, "alice", "secret"));
    pop3_quit(socket_fd);
    pop3_close(socket_fd);
}

TEST(Pop3Client, LoginFailsOnErrorResponse)
{
    FakePop3Server server;
    server.start([](int fd) {
        server_send(fd, "+OK ready\r\n");
        (void)server_recv_line(fd);
        server_send(fd, "+OK\r\n");
        (void)server_recv_line(fd);
        server_send(fd, "-ERR bad password\r\n");
    });

    int socket_fd = pop3_connect("127.0.0.1", server.port_string().c_str());
    ASSERT_GE(socket_fd, 0);
    EXPECT_FALSE(pop3_login(socket_fd, "alice", "wrong"));
    pop3_close(socket_fd);
}

TEST(Pop3Client, ConnectFailsForUnreachableServer)
{
    /* Port 1 on loopback is reserved and reliably refused. */
    EXPECT_LT(pop3_connect("127.0.0.1", "1"), 0);
}
