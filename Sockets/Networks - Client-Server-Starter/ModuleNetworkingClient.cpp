#include "ModuleNetworkingClient.h"


bool  ModuleNetworkingClient::start(const char * serverAddressStr, int serverPort, const char *pplayerName)
{
	playerName = pplayerName;

	// TODO(jesus): TCP connection stuff
	// - Create the socket
	// - Create the remote address object
	// - Connect to the remote address
	// - Add the created socket to the managed list of sockets using addSocket()

	client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == INVALID_SOCKET) {
		printWSErrorAndExit("socket");
	}
	LOG("socket done");

	const int serverAddrLen = sizeof(address);
	address.sin_family = AF_INET; // IPv4
	inet_pton(AF_INET, serverAddressStr, &address.sin_addr);
	address.sin_port = htons(serverPort); // Port

	int connectRes = connect(client_socket, (const sockaddr*)&address, serverAddrLen);
	if (connectRes != SOCKET_ERROR) {
		LOG("Connection Done");
		state = ClientState::Start;
		addSocket(client_socket);
	}
	else
	{
		printWSErrorAndContinue("Error connectiong to server");

	}
	


	// If everything was ok... change the state

	return true;
}

bool ModuleNetworkingClient::isRunning() const
{
	return state != ClientState::Stopped;
}

bool ModuleNetworkingClient::update()
{
	if (state == ClientState::Start)
	{
		// TODO(jesus): Send the player name to the server
		int result = send(client_socket, playerName.c_str(), playerName.size() + 1, 0);
		if (result > 0)
		{
			LOG("Name: %s Sent",playerName.c_str());
			state = ClientState::Logging;
		}
		else
		{
			int err = WSAGetLastError();
			printWSErrorAndContinue("Error sending name %s to the server, continuing...", playerName.c_str());
		}
	}
	

	return true;
}

bool ModuleNetworkingClient::gui()
{
	if (state != ClientState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Client Window");

		Texture *tex = App->modResources->client;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("%s connected to the server...", playerName.c_str());

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, byte * data)
{
	state = ClientState::Stopped;
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}

