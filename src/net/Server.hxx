#ifndef SERVER_HXX
#define SERVER_HXX

namespace HTTPServer {
void initialize();

void start();

void stop();

void sendUpdate();

void closeConnections();
}  // namespace HTTPServer

#endif  // SERVER_HXX
