#include "HttpServer.h"
#include <iostream>
#include "crow/crow_all.h"

void HttpServer::run() {
    std::cout << "Starting Http server" << std::endl;

    crow::SimpleApp app;

    CROW_ROUTE(app, "/")([](){
        return "Hello!";
    });

    app.port(18080).multithreaded().run();
}
