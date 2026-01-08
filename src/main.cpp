#include "HttpServer.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <thread>
#include <chrono>

static HttpServer* g_server = nullptr;

void signal_handler(int signal) {
    if (signal == SIGINT || signal == SIGTERM) {
        std::cout << "\nSignal d'arrêt reçu..." << std::endl;
        if (g_server) {
            g_server->stop();
            exit(0);
        }
    }
}

int main(int argc, char* argv[]) {
    // Port par défaut : 8080
    int port = 8080;
    size_t thread_pool_size = std::thread::hardware_concurrency();
    
    // Parser les arguments
    if (argc > 1) {
        port = std::atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            std::cerr << "Port invalide: " << argv[1] << std::endl;
            return 1;
        }
    }
    
    if (argc > 2) {
        thread_pool_size = std::atoi(argv[2]);
        if (thread_pool_size == 0) {
            thread_pool_size = std::thread::hardware_concurrency();
        }
    }

    // Configurer les handlers de signal
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); // Ignorer SIGPIPE

    // Créer et démarrer le serveur
    HttpServer server(port, thread_pool_size, 10000);
    g_server = &server;
    
    server.start();
    
    // Maintenir le thread principal en vie
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
