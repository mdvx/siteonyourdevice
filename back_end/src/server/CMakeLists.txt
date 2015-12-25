CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)
PROJECT(site_on_your_device_server CXX)
STRING(TOLOWER ${PROJECT_NAME} PROJECT_NAME_LOWERCASE)

ADD_DEFINITIONS(-DCONFIG_FILE_PATH="/etc/${PROJECT_NAME_LOWERCASE}.conf")
ADD_DEFINITIONS(-DPROJECT_NAME_SERVER_TITLE="${PROJECT_NAME}")
ADD_DEFINITIONS(-DPROJECT_NAME_SERVER="${PROJECT_NAME_LOWERCASE}")

INCLUDE_DIRECTORIES(../third-party/redis/deps)

ADD_EXECUTABLE(${PROJECT_NAME}
                main.cpp
                relay_server.h relay_server.cpp
                http_server_host.h http_server_host.cpp
                inner/inner_tcp_server.h inner/inner_tcp_server.cpp
                inner/inner_tcp_client.h inner/inner_tcp_client.cpp
                websocket_server.h websocket_server.cpp
                redis_helpers.h redis_helpers.cpp
                server_commands.h
)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} ${PROJECT_CORE_LIBRARY} hiredis)
