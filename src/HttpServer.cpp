#include "HttpServer.h"
#include <unistd.h>
#include <fcntl.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <signal.h>
#include <errno.h>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <exception>

HttpServer::HttpServer(int port, size_t thread_pool_size, size_t max_connections)
    : port_(port), server_fd_(-1), epoll_fd_(-1), running_(false),
      thread_pool_(std::make_unique<ThreadPool>(thread_pool_size)),
      max_connections_(max_connections) {
}

HttpServer::~HttpServer() {
    stop();
}

bool HttpServer::setup_server_socket() {
    // Créer le socket
    server_fd_ = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd_ < 0) {
        std::cerr << "Erreur: impossible de créer le socket" << std::endl;
        return false;
    }

    // Options du socket pour réutiliser l'adresse
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)) < 0) {
        std::cerr << "Erreur: setsockopt échoué" << std::endl;
        return false;
    }

    // Configurer l'adresse
    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_);

    // Bind
    if (bind(server_fd_, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Erreur: bind échoué sur le port " << port_ << std::endl;
        return false;
    }

    // Listen avec une grande backlog pour supporter C10k
    if (listen(server_fd_, 4096) < 0) {
        std::cerr << "Erreur: listen échoué" << std::endl;
        return false;
    }

    std::cout << "Serveur HTTP démarré sur le port " << port_ << std::endl;
    return true;
}

bool HttpServer::setup_epoll() {
    epoll_fd_ = epoll_create1(0);
    if (epoll_fd_ < 0) {
        std::cerr << "Erreur: epoll_create1 échoué" << std::endl;
        return false;
    }

    // Ajouter le socket serveur à epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET; // Edge-triggered mode
    ev.data.fd = server_fd_;
    
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, server_fd_, &ev) < 0) {
        std::cerr << "Erreur: epoll_ctl échoué" << std::endl;
        return false;
    }

    return true;
}

void HttpServer::accept_connection() {
    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    
    // Accepter toutes les connexions en attente (edge-triggered)
    while (true) {
        int client_fd = accept4(server_fd_, (struct sockaddr*)&client_addr, 
                               &client_addr_len, SOCK_NONBLOCK);
        
        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Plus de connexions en attente
                break;
            }
            std::cerr << "Erreur: accept échoué" << std::endl;
            break;
        }

        // Vérifier la limite de connexions
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            if (connections_.size() >= max_connections_) {
                ::close(client_fd);
                continue;
            }

            // Créer la connexion
            auto conn = std::make_unique<Connection>(client_fd, client_addr);
            connections_[client_fd] = std::move(conn);
        }

        // Ajouter à epoll
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT; // Edge-triggered, one-shot
        ev.data.fd = client_fd;
        
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &ev) < 0) {
            std::cerr << "Erreur: epoll_ctl pour client échoué" << std::endl;
            close_connection(client_fd);
        }
    }
}

void HttpServer::handle_epoll_events() {
    const int MAX_EVENTS = 256;
    struct epoll_event events[MAX_EVENTS];
    
    while (running_) {
        int num_events = epoll_wait(epoll_fd_, events, MAX_EVENTS, 100);
        
        if (num_events < 0) {
            if (errno == EINTR) {
                continue;
            }
            std::cerr << "Erreur: epoll_wait échoué" << std::endl;
            break;
        }

        for (int i = 0; i < num_events; ++i) {
            if (events[i].data.fd == server_fd_) {
                // Nouvelle connexion
                accept_connection();
            } else {
                // Données à lire
                if (events[i].events & EPOLLIN) {
                    handle_read(events[i].data.fd);
                }
            }
        }
    }
}

void HttpServer::handle_read(int client_fd) {
    // Déléguer la lecture au thread pool
    thread_pool_->enqueue([this, client_fd]() {
        std::unique_lock<std::mutex> lock(connections_mutex_);
        auto it = connections_.find(client_fd);
        if (it == connections_.end()) {
            return;
        }
        
        Connection* conn = it->second.get();
        lock.unlock();

        // Lire les données
        ssize_t n = recv(client_fd, conn->buffer.data() + conn->bytes_read,
                        conn->buffer.size() - conn->bytes_read - 1, 0);

        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Réactiver epoll pour cette socket
                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                ev.data.fd = client_fd;
                epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
                return;
            }
            close_connection(client_fd);
            return;
        }

        if (n == 0) {
            // Connexion fermée
            close_connection(client_fd);
            return;
        }

        conn->bytes_read += n;
        conn->buffer[conn->bytes_read] = '\0';

        // Chercher la fin de la requête HTTP (double CRLF)
        std::string request_data(conn->buffer.data(), conn->bytes_read);
        size_t header_end = request_data.find("\r\n\r\n");
        
        if (header_end != std::string::npos) {
            // Requête complète reçue
            request_data = request_data.substr(0, header_end + 4);
            process_request(client_fd, request_data);
        } else if (conn->bytes_read >= static_cast<ssize_t>(conn->buffer.size() - 1)) {
            // Buffer plein sans fin de requête
            close_connection(client_fd);
            return;
        } else {
            // Réactiver epoll pour lire plus de données
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
            ev.data.fd = client_fd;
            epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
        }
    });
}

void HttpServer::process_request(int client_fd, const std::string& request_data) {
    try {
        HttpRequest request;
        
        if (!HttpRequest::parse(request_data, request)) {
            // Requête invalide
            std::string response = HttpResponse::build_response(
                HttpResponse::BAD_REQUEST, 
                "<html><body><h1>400 Bad Request</h1><p>La requête HTTP est invalide.</p></body></html>",
                false
            );
            send_response(client_fd, response);
            close_connection(client_fd);
            return;
        }

        // Générer la réponse
        std::string response_body;
        HttpResponse::StatusCode status_code = HttpResponse::OK;
        
        try {
            response_body = generate_response(request);
            
            if (response_body.empty() && request.method == "GET") {
                // Route non trouvée
                status_code = HttpResponse::NOT_FOUND;
                response_body = "<html><body><h1>404 Not Found</h1><p>La ressource demandée n'existe pas.</p></body></html>";
            } else if (response_body.empty()) {
                // Méthode non supportée (non-GET)
                status_code = HttpResponse::BAD_REQUEST;
                response_body = "<html><body><h1>405 Method Not Allowed</h1><p>La méthode HTTP n'est pas supportée.</p></body></html>";
            }
        } catch (const std::exception& e) {
            // Erreur interne du serveur
            status_code = HttpResponse::INTERNAL_ERROR;
            response_body = "<html><body><h1>500 Internal Server Error</h1><p>Une erreur interne s'est produite.</p></body></html>";
            std::cerr << "Erreur lors de la génération de la réponse: " << e.what() << std::endl;
        }
        
        std::string response = HttpResponse::build_response(
            status_code,
            response_body,
            request.keep_alive
        );

        send_response(client_fd, response);

        // Gérer keep-alive
        {
            std::lock_guard<std::mutex> lock(connections_mutex_);
            auto it = connections_.find(client_fd);
            if (it != connections_.end()) {
                it->second->keep_alive = request.keep_alive;
                it->second->reset();
                
                if (request.keep_alive) {
                    // Réactiver epoll pour cette connexion
                    struct epoll_event ev;
                    ev.events = EPOLLIN | EPOLLET | EPOLLONESHOT;
                    ev.data.fd = client_fd;
                    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, client_fd, &ev);
                } else {
                    close_connection(client_fd);
                }
            }
        }
    } catch (const std::exception& e) {
        // Erreur critique lors du traitement de la requête
        std::cerr << "Erreur critique lors du traitement de la requête: " << e.what() << std::endl;
        std::string response = HttpResponse::build_response(
            HttpResponse::INTERNAL_ERROR,
            "<html><body><h1>500 Internal Server Error</h1><p>Une erreur interne s'est produite.</p></body></html>",
            false
        );
        send_response(client_fd, response);
        close_connection(client_fd);
    }
}

void HttpServer::send_response(int client_fd, const std::string& response) {
    size_t total_sent = 0;
    size_t len = response.length();

    while (total_sent < len) {
        ssize_t n = send(client_fd, response.data() + total_sent, 
                        len - total_sent, MSG_NOSIGNAL);
        
        if (n < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Réessayer
                continue;
            }
            // Erreur d'envoi
            break;
        }
        
        total_sent += n;
    }
}

void HttpServer::close_connection(int client_fd) {
    std::lock_guard<std::mutex> lock(connections_mutex_);
    // Retirer de epoll (ignore les erreurs si déjà fermé)
    epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, client_fd, nullptr);
    connections_.erase(client_fd);
}

std::string HttpServer::generate_response(const HttpRequest& request) {
    // Support GET seulement pour l'instant
    if (request.method != "GET") {
        return ""; // Sera géré comme 405 dans process_request
    }

    // Route simple
    if (request.path == "/" || request.path == "/index.html") {
        std::ostringstream oss;
        oss << "<html><head><title>High-Performance HTTP Server</title></head>"
            << "<body><h1>Bienvenue sur le serveur HTTP haute performance</h1>"
            << "<p>Serveur optimisé pour Linux avec epoll et ThreadPool</p>"
            << "<p>Objectif: > 12 000 requêtes/seconde</p>"
            << "<p>Support HTTP/1.1 avec keep-alive</p>"
            << "</body></html>";
        return oss.str();
    }

    // 404 par défaut - sera géré par process_request
    return "";
}

void HttpServer::start() {
    if (running_) {
        return;
    }

    if (!setup_server_socket()) {
        return;
    }

    if (!setup_epoll()) {
        return;
    }

    running_ = true;
    
    // Lancer la boucle principale dans un thread dédié
    std::thread main_loop([this]() {
        handle_epoll_events();
    });

    main_loop.detach();
    
    std::cout << "Serveur démarré. Appuyez sur Ctrl+C pour arrêter." << std::endl;
}

void HttpServer::stop() {
    if (!running_) {
        return;
    }

    running_ = false;

    // Fermer toutes les connexions
    {
        std::lock_guard<std::mutex> lock(connections_mutex_);
        connections_.clear();
    }

    if (epoll_fd_ >= 0) {
        ::close(epoll_fd_);
        epoll_fd_ = -1;
    }

    if (server_fd_ >= 0) {
        ::close(server_fd_);
        server_fd_ = -1;
    }

    thread_pool_->shutdown();
    
    std::cout << "Serveur arrêté." << std::endl;
}
