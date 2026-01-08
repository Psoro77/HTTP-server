#pragma once

#include "ThreadPool.h"
#include "Connection.h"
#include "HttpRequest.h"
#include "HttpResponse.h"
#include <sys/epoll.h>
#include <atomic>
#include <memory>
#include <unordered_map>
#include <mutex>

/**
 * Serveur HTTP haute performance utilisant epoll et ThreadPool
 * Conçu pour supporter C10k et atteindre > 12 000 RPS
 */
class HttpServer {
public:
    HttpServer(int port, size_t thread_pool_size = 4, size_t max_connections = 10000);
    ~HttpServer();

    // Non-copyable, non-movable
    HttpServer(const HttpServer&) = delete;
    HttpServer& operator=(const HttpServer&) = delete;
    HttpServer(HttpServer&&) = delete;
    HttpServer& operator=(HttpServer&&) = delete;

    // Démarrer le serveur
    void start();

    // Arrêter le serveur
    void stop();

private:
    int port_;
    int server_fd_;
    int epoll_fd_;
    std::atomic<bool> running_;
    std::unique_ptr<ThreadPool> thread_pool_;
    size_t max_connections_;
    
    // Gestion des connexions
    std::unordered_map<int, std::unique_ptr<Connection>> connections_;
    std::mutex connections_mutex_;

    // Initialiser le socket serveur
    bool setup_server_socket();
    
    // Configurer epoll
    bool setup_epoll();
    
    // Accepter une nouvelle connexion
    void accept_connection();
    
    // Gérer les événements epoll
    void handle_epoll_events();
    
    // Lire les données d'une connexion
    void handle_read(int client_fd);
    
    // Traiter une requête HTTP
    void process_request(int client_fd, const std::string& request_data);
    
    // Envoyer une réponse
    void send_response(int client_fd, const std::string& response);
    
    // Fermer une connexion
    void close_connection(int client_fd);
    
    // Générer une réponse HTTP pour une requête
    std::string generate_response(const HttpRequest& request);
};
