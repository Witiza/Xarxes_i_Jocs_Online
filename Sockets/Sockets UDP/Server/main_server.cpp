#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include "Windows.h"
#include "WinSock2.h"
#include "Ws2tcpip.h"

#include <iostream>
#include <cstdlib>

#pragma comment(lib, "ws2_32.lib")

#define SERVER_PORT 8888

#define PAUSE_AND_EXIT() system("pause"); exit(-1)

void printWSErrorAndExit(const char *msg)
{
	wchar_t *s = NULL;
	FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPWSTR)&s, 0, NULL);
	fprintf(stderr, "%s: %S\n", msg, s);
	LocalFree(s);
	PAUSE_AND_EXIT();
}

void server(int port)
{

	// TODO-1: Winsock init
	WSADATA wsaData;
	int iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iResult != NO_ERROR)
	{
		// Log and handle error
		printWSErrorAndExit("Error initializing Server Winsock");
		return;
	}

	// TODO-2: Create socket (IPv4, datagrams, UDP
	SOCKET server_socket = socket(AF_INET, SOCK_DGRAM, 0);

	// TODO-3: Force address reuse
	sockaddr_in socket_address;
	socket_address.sin_family = AF_INET;
	socket_address.sin_port = htons(port);
	socket_address.sin_addr.S_un.S_addr = INADDR_ANY;

	// TODO-4: Bind to a local address
	//bind(server_socket, (sockaddr*)&socket_address, sizeof(socket_address));

	sockaddr_in from;
	while (true)
	{
		char buffer[64];
		// TODO-5:
		// - Receive 'ping' packet from a remote host
		// - Answer with a 'pong' packet
		// - Control errors in both cases
				// - Control errors in both cases
		int tolen = sizeof(socket_address);
		int received = recvfrom(server_socket, buffer, 64, 0, (sockaddr*)&socket_address, &tolen);

			printf(buffer);
			sendto(server_socket, "yoot", sizeof("yoot"), 0, (sockaddr*)&socket_address, sizeof(socket_address));
		
		Sleep(500);
	}

	// TODO-6: Close socket
	closesocket(server_socket);
	// TODO-7: Winsock shutdown
	iResult = WSACleanup();
	if (iResult != NO_ERROR)
	{
		// Log and handle error
		printWSErrorAndExit("Error initializing Server Winsock");
		return;
	}
}

int main(int argc, char **argv)
{
	server(SERVER_PORT);

	PAUSE_AND_EXIT();
}
