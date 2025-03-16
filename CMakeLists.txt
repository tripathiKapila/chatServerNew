cmake_minimum_required(VERSION 3.10)
project(ChatServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-D_WIN32_WINNT=0x0601)
endif()

# Find Boost (adjust components as needed)
find_package(Boost REQUIRED COMPONENTS system thread)

# Find SQLite3 (if required, adjust find module or use pkg-config)
find_package(SQLite3 REQUIRED)

# Include directories
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${SQLite3_INCLUDE_DIRS})
include_directories(${PROJECT_SOURCE_DIR})

# List all source files
set(SOURCES
    main.cpp
    Server.cpp
    Session.cpp
    CommandRouter.cpp
    AuthManager.cpp
    UserManager.cpp
    SessionManager.cpp
    ChatHistoryCache.cpp
    Logger.cpp
    Config.cpp
    Database.cpp
)

# Create the executable
add_executable(ChatServer ${SOURCES})

# Link libraries
target_link_libraries(ChatServer ${Boost_LIBRARIES} ${SQLite3_LIBRARIES}) 