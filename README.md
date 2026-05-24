C Multi-User Chat
A lightweight, concurrent command-line application built with C that implements a multi-user chat room using the Client/Server model. The app operates efficiently at the system level, utilizing POSIX threads and strict synchronization rules to handle multiple connections and prevent deadlocks.

Features
Concurrent Architecture: Built with POSIX threads to seamlessly handle multiple users connecting and broadcasting simultaneously.
Thread-Safe Logging: Records all chat history to a dynamically generated text file, utilizing Mutexes to protect the shared resource.
Dynamic User Management: Validates active usernames in real-time and gracefully handles client connections and disconnections.
System-Level Networking: Runs entirely on native UNIX-like socket programming for raw TCP communication.

Tech Stack
C (Standard programming language)
<sys/socket.h> (POSIX network sockets API)
<pthread.h> (Thread creation and Mutex synchronization)
<time.h> (Dynamic timestamp generation for files)
