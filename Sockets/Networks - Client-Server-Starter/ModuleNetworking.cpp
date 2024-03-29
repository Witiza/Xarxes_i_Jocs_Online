#include "Networks.h"
#include "ModuleNetworking.h"


static uint8 NumModulesUsingWinsock = 0;



void ModuleNetworking::reportError(const char* inOperationDesc)
{
	LPVOID lpMsgBuf;
	DWORD errorNum = WSAGetLastError();

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER |
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		errorNum,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0, NULL);

	ELOG("Error %s: %d- %s", inOperationDesc, errorNum, lpMsgBuf);
}

void ModuleNetworking::disconnect()
{
	for (SOCKET socket : sockets)
	{
		shutdown(socket, 2);
		closesocket(socket);
	}

	sockets.clear();
	messages.clear();
}

bool ModuleNetworking::init()
{
	if (NumModulesUsingWinsock == 0)
	{
		NumModulesUsingWinsock++;

		WORD version = MAKEWORD(2, 2);
		WSADATA data;
		if (WSAStartup(version, &data) != 0)
		{
			reportError("ModuleNetworking::init() - WSAStartup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::preUpdate()
{
	if (sockets.empty()) return true;

	// NOTE(jesus): You can use this temporary buffer to store data from recv()
	InputMemoryStream packet;

	// TODO(jesus): select those sockets that have a read operation available
	fd_set read_fd;
	FD_ZERO(&read_fd);
	for (auto s : sockets)
	{
		FD_SET(s, &read_fd);
	}

	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 0;

	int result = select(0, &read_fd, nullptr, nullptr, &timeout);
	if (result == SOCKET_ERROR)
	{
		printWSErrorAndExit("Error during the SELECT function");
	}

	for (auto s : sockets)
	{
		if (FD_ISSET(s, &read_fd))
		{
			if (isListenSocket(s))
			{
				int size = sizeof(address);
				SOCKET new_s = accept(s, (sockaddr*)&address, &size);
				if (new_s != INVALID_SOCKET)
				{
					onSocketConnected(new_s, address);
				}
				else
					printWSErrorAndContinue("Could not accept a socket");
			}
			else
			{
				int result = recv(s, packet.GetBufferPtr(), packet.GetCapacity(),0);
				if (result > 0)
				{
					packet.SetSize((uint32)result);
					onSocketReceivedData(s, packet);
				}
				else if (result == 0 || WSAGetLastError() == WSAECONNRESET)
				{
					//Remove the socket from the list
					onSocketDisconnected(s);
					auto socket_to_remove = std::find(sockets.begin(), sockets.end(), s);
					sockets.erase(socket_to_remove);
				}
				else
					printWSErrorAndContinue("Error when receiving data from a socket");
			}
		}
	}
	// TODO(jesus): for those sockets selected, check wheter or not they are
	// a listen socket or a standard socket and perform the corresponding
	// operation (accept() an incoming connection or recv() incoming data,
	// respectively).
	// On accept() success, communicate the new connected socket to the
	// subclass (use the callback onSocketConnected()), and add the new
	// connected socket to the managed list of sockets.
	// On recv() success, communicate the incoming data received to the
	// subclass (use the callback onSocketReceivedData()).

	// TODO(jesus): handle disconnections. Remember that a socket has been
	// disconnected from its remote end either when recv() returned 0,
	// or when it generated some errors such as ECONNRESET.
	// Communicate detected disconnections to the subclass using the callback
	// onSocketDisconnected().

	// TODO(jesus): Finally, remove all disconnected sockets from the list
	// of managed sockets.

	return true;
}

bool ModuleNetworking::cleanUp()
{
	disconnect();

	NumModulesUsingWinsock--;
	if (NumModulesUsingWinsock == 0)
	{

		if (WSACleanup() != 0)
		{
			reportError("ModuleNetworking::cleanUp() - WSACleanup");
			return false;
		}
	}

	return true;
}

bool ModuleNetworking::sendPacket(const OutputMemoryStream& packet, SOCKET socket)
{
	bool ret = false;
	if (send(socket, packet.GetBufferPtr(), packet.GetSize(), 0) != SOCKET_ERROR)
	{
		ret = true;
	}
	else
		reportError("Error sending a packet");

	return ret;
}

void ModuleNetworking::addSocket(SOCKET socket)
{
	sockets.push_back(socket);
}
