#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
using namespace std;
#pragma comment(lib, "Ws2_32.lib")
#include <winsock2.h>
#include <string.h>
#include <string>
#include <time.h>


struct SocketState
{
	SOCKET id;
	int	recv;
	int	send;
	int sendSubType;
	char buffer[4096];
	int len;
	time_t lastActivity;
};

const int TIME_PORT = 27015;
const int MAX_SOCKETS = 60;
const int EMPTY = 0;
const int LISTEN = 1;
const int RECEIVE = 2;
const int IDLE = 3;
const int SEND = 4;
const int SEND_TIME = 1;
const int SEND_SECONDS = 2;

bool addSocket(SOCKET id, int what);
void removeSocket(int index);
void acceptConnection(int index);
void receiveMessage(int index);
void sendMessage(int index);

struct SocketState sockets[MAX_SOCKETS] = { 0 };
int socketsCount = 0;


int main()
{
	// Initialize Winsock (Windows Sockets).
	WSAData wsaData;

	if (NO_ERROR != WSAStartup(MAKEWORD(2, 2), &wsaData))
	{
	cout << "Web Server: Error at WSAStartup()\n";
		return 1;
	}

	// Create listening socket.
	SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (INVALID_SOCKET == listenSocket)
	{
		cout << "Web Server: Error at socket(): "
			<< WSAGetLastError() << endl;

		WSACleanup();
		return 1;
	}

	// Server address.
	sockaddr_in serverService;

	serverService.sin_family = AF_INET;
	serverService.sin_addr.s_addr = INADDR_ANY;
	serverService.sin_port = htons(TIME_PORT);

	// Bind socket.
	if (SOCKET_ERROR == bind(listenSocket,
		(SOCKADDR*)&serverService,
		sizeof(serverService)))
	{
		cout << "Web Server: Error at bind(): "
			<< WSAGetLastError() << endl;

		closesocket(listenSocket);
		WSACleanup();

		return 1;
	}

	// Listen.
	if (SOCKET_ERROR == listen(listenSocket, 5))
	{
		cout << "Web Server: Error at listen(): "
			<< WSAGetLastError() << endl;

		closesocket(listenSocket);
		WSACleanup();

		return 1;
	}

	addSocket(listenSocket, LISTEN);

	// Main server loop.
	while (true)
	{
		fd_set waitRecv;
		FD_ZERO(&waitRecv);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if ((sockets[i].recv == LISTEN) ||
				(sockets[i].recv == RECEIVE))
			{
				FD_SET(sockets[i].id, &waitRecv);
			}
		}

		fd_set waitSend;
		FD_ZERO(&waitSend);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].send == SEND)
			{
				FD_SET(sockets[i].id, &waitSend);
			}
		}

		// Timeout for stuck connections.
		timeval timeout;
		timeout.tv_sec = 1;
		timeout.tv_usec = 0;

		int nfd = select(0,
			&waitRecv,
			&waitSend,
			NULL,
			&timeout);

		if (nfd == SOCKET_ERROR)
		{
			cout << "Web Server: Error at select(): "
				<< WSAGetLastError() << endl;

			WSACleanup();

			return 1;
		}

		// Close stuck connections after 2 minutes.
		time_t now = time(NULL);

		for (int i = 0; i < MAX_SOCKETS; i++)
		{
			if (sockets[i].recv == RECEIVE &&
				difftime(now, sockets[i].lastActivity) > 120)
			{
				cout << "Closing stuck connection after 2 minutes."
					<< endl;

				closesocket(sockets[i].id);

				removeSocket(i);
			}
		}

		// Receive events.
		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitRecv))
			{
				nfd--;

				switch (sockets[i].recv)
				{
				case LISTEN:
					acceptConnection(i);
					break;

				case RECEIVE:
					receiveMessage(i);
					break;
				}
			}
		}

		// Send events.
		for (int i = 0; i < MAX_SOCKETS && nfd > 0; i++)
		{
			if (FD_ISSET(sockets[i].id, &waitSend))
			{
				nfd--;

				switch (sockets[i].send)
				{
				case SEND:
					sendMessage(i);
					break;
				}
			}
		}
	}

	cout << "Web Server: Closing Connection.\n";

	closesocket(listenSocket);

	WSACleanup();

	return 0;
}

bool addSocket(SOCKET id, int what)
{
	for (int i = 0; i < MAX_SOCKETS; i++)
	{
		if (sockets[i].recv == EMPTY)
		{
			sockets[i].id = id;
			sockets[i].recv = what;
			sockets[i].send = IDLE;
			sockets[i].len = 0;
			sockets[i].lastActivity = time(NULL);

			socketsCount++;

			return (true);
		}
	}
	return (false);
}

void removeSocket(int index)
{
	sockets[index].recv = EMPTY;
	sockets[index].send = EMPTY;
	socketsCount--;
}

void acceptConnection(int index)
{
	SOCKET id = sockets[index].id;
	struct sockaddr_in from;		// Address of sending partner
	int fromLen = sizeof(from);

	SOCKET msgSocket = accept(id, (struct sockaddr*)&from, &fromLen);
	if (INVALID_SOCKET == msgSocket)
	{
		cout << "Web Server: Error at accept(): " << WSAGetLastError() << endl;
		return;
	}
	cout << "Web Server: Client " << inet_ntoa(from.sin_addr) << ":" << ntohs(from.sin_port) << " is connected." << endl;

	//
	// Set the socket to be in non-blocking mode.
	//
	unsigned long flag = 1;
	if (ioctlsocket(msgSocket, FIONBIO, &flag) != 0)
	{
		cout << "Web Server: Error at ioctlsocket(): " << WSAGetLastError() << endl;
	}

	if (addSocket(msgSocket, RECEIVE) == false)
	{
	cout << "\t\tToo many connections, dropped!\n";
	closesocket(msgSocket);
	}
	return;
}

void receiveMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int len = sockets[index].len;
	int bytesRecv = recv(msgSocket, &sockets[index].buffer[len], sizeof(sockets[index].buffer) - len - 1, 0);

	if (SOCKET_ERROR == bytesRecv)
	{
		cout << "Web Server: Error at recv(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}

	if (bytesRecv == 0)
	{
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}

	sockets[index].buffer[len + bytesRecv] = '\0';

	sockets[index].lastActivity = time(NULL);
	cout << "================ REQUEST START ================" << endl;
	cout << sockets[index].buffer << endl;
	cout << "================ REQUEST END ==================" << endl;

	sockets[index].len += bytesRecv;

	string request = sockets[index].buffer;
	string html;

	if (request.rfind("GET ", 0) == 0)
	{
		cout << "GET request received" << endl;

		if (request.find("lang=he") != string::npos)
		{
			html = "<html><body><h1>Hebrew page</h1><p>Shalom</p></body></html>";
		}
		else if (request.find("lang=fr") != string::npos)
		{
			html = "<html><body><h1>Bonjour</h1><p>Page en francais</p></body></html>";
		}
		else if (request.find("lang=en") != string::npos)
		{
			html = "<html><body><h1>Hello</h1><p>English page</p></body></html>";
		}
		else
		{
			html = "<html><body><h1>Hello</h1><p>English page</p></body></html>";
		}
	}
	else if (request.rfind("POST ", 0) == 0)
	{
		cout << "POST request received" << endl;

		size_t bodyPos = request.find("\r\n\r\n");
		if (bodyPos != string::npos)
		{
			string body = request.substr(bodyPos + 4);
			cout << "POST body: " << body << endl;
		}

		html = "<html><body><h1>POST request received</h1></body></html>";
	}
	else if (request.rfind("HEAD ", 0) == 0)
	{
		cout << "HEAD request received" << endl;
		html = "";
	}
	else if (request.rfind("OPTIONS ", 0) == 0)
	{
		cout << "OPTIONS request received" << endl;
		html = "<html><body><h1>OPTIONS request</h1><p>Allowed: OPTIONS, GET, HEAD, POST, PUT, DELETE, TRACE</p></body></html>";
	}
	else if (request.rfind("PUT ", 0) == 0)
	{
		cout << "PUT request received" << endl;
		html = "<html><body><h1>PUT request</h1></body></html>";
	}
	else if (request.rfind("DELETE ", 0) == 0)
	{
		cout << "DELETE request received" << endl;
		html = "<html><body><h1>DELETE request</h1></body></html>";
	}
	else if (request.rfind("TRACE ", 0) == 0)
	{
		cout << "TRACE request received" << endl;
		html = "<html><body><h1>TRACE request</h1></body></html>";
	}
	else
	{
		cout << "Unknown request" << endl;
		html = "<html><body><h1>Other request</h1></body></html>";
	}

	string response =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: close\r\n"
		"Content-Length: " + to_string(html.length()) + "\r\n"
		"\r\n" +
		html;

	memcpy(sockets[index].buffer, response.c_str(), response.length());

	sockets[index].len = (int)response.length();
	sockets[index].send = SEND;
}


void sendMessage(int index)
{
	SOCKET msgSocket = sockets[index].id;

	int bytesSent = send(msgSocket, sockets[index].buffer, sockets[index].len, 0);

	if (SOCKET_ERROR == bytesSent)
	{
		cout << "Web Server: Error at send(): " << WSAGetLastError() << endl;
		closesocket(msgSocket);
		removeSocket(index);
		return;
	}

	cout << "Web Server: Sent: " << bytesSent << " bytes.\n";

	sockets[index].len = 0;
	sockets[index].buffer[0] = '\0';

	sockets[index].send = IDLE;
	closesocket(msgSocket);
	removeSocket(index);
}