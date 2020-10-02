#ifndef SERVER_HXX
#define SERVER_HXX

namespace HTTPServer {
void initialize();

void start();

void stop();

void sendUpdate();
}  // namespace HTTPServer

#endif  // SERVER_HXX
