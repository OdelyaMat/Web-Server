# C++ Web Server

A lightweight HTTP web server implemented in C++ using Windows Sockets (Winsock).

This project was developed as part of a Computer Networks course and focuses on network communication, socket programming, HTTP request handling, and non-blocking I/O.

## Features

* TCP client-server communication
* Non-blocking sockets
* Multiple simultaneous connections using `select()`
* Support for HTTP methods:

  * GET
  * POST
  * HEAD
  * OPTIONS
  * PUT
  * DELETE
  * TRACE
* Query string handling for language selection
* HTML responses in English, Hebrew, and French
* POST request body handling
* Connection timeout handling
* HTTP response generation with content length and content type

## Technologies

* C++
* Winsock
* TCP/IP
* HTTP
* Socket Programming
* `select()`

## How It Works

The server creates a TCP listening socket and waits for incoming client connections.

Connected clients are managed using non-blocking sockets and the `select()` function, allowing the server to handle multiple connections without using multi-threading.

Incoming HTTP requests are parsed according to their method and requested parameters. The server then generates an appropriate HTML response and sends it back to the client.

### Language Selection

GET requests can include a `lang` query parameter:

* `lang=en` → English
* `lang=he` → Hebrew
* `lang=fr` → French

If no supported language is specified, the server returns the default English page.

### Supported HTTP Methods

The server handles the following HTTP methods:

`GET`, `POST`, `HEAD`, `OPTIONS`, `PUT`, `DELETE`, `TRACE`

POST requests are also processed to extract and display the request body.

## Connection Management

The server uses `select()` to monitor sockets for incoming data and outgoing responses.

Connections that remain inactive for more than two minutes are automatically closed.

## Academic Context

This project was developed as part of a Computer Networks course and was designed to practice TCP communication, socket programming, HTTP protocol handling, and network traffic analysis.
