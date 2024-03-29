#include "ModuleNetworkingServer.h"




//////////////////////////////////////////////////////////////////////
// ModuleNetworkingServer public methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::start(int port)
{
	// TODO(jesus): TCP listen socket stuff
	// - Create the listenSocket
	// - Set address reuse
	// - Bind the socket to a local interface
	// - Enter in listen mode
	// - Add the listenSocket to the managed list of sockets using addSocket()
	listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET) {
		printWSErrorAndExit("Error creating Socket:   ");
	}
	LOG("Socket Done");

	int enable = 1;
	int result = setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&enable, sizeof(int));
	if (result == SOCKET_ERROR) {
		printWSErrorAndExit("Error when using Setsockopt:   ");
	}
	LOG("setsockopt SO_REUSEADDR done");

	// Address (server)
	address.sin_family = AF_INET; // IPv4
	address.sin_addr.S_un.S_addr = INADDR_ANY; // Any address
	address.sin_port = htons(port); // Port

	// Bind socket
	int bindRes = bind(listenSocket, (struct sockaddr *)&address, sizeof(address));
	if (bindRes == SOCKET_ERROR) {
		printWSErrorAndExit("Error when binding the server socket:   ");
	}
	LOG("Socket binding done");

	// Listen
	int listenRes = listen(listenSocket, 1);
	if (listenRes == SOCKET_ERROR) {
		printWSErrorAndExit("listen");
	}
	LOG("Listen mode activated");

	addSocket(listenSocket);

	state = ServerState::Listening;

	return true;
}

bool ModuleNetworkingServer::isRunning() const
{
	return state != ServerState::Stopped;
}



//////////////////////////////////////////////////////////////////////
// Module virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::update()
{
	return true;
}

bool ModuleNetworkingServer::gui()
{
	if (state != ServerState::Stopped)
	{
		// NOTE(jesus): You can put ImGui code here for debugging purposes
		ImGui::Begin("Server Window");

		Texture *tex = App->modResources->server;
		ImVec2 texSize(400.0f, 400.0f * tex->height / tex->width);
		ImGui::Image(tex->shaderResource, texSize);

		ImGui::Text("List of connected sockets:");

		for (auto &connectedSocket : connectedSockets)
		{
			ImGui::Separator();
			ImGui::Text("Socket ID: %d", connectedSocket.socket);
			ImGui::Text("Address: %d.%d.%d.%d:%d",
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b1,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b2,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b3,
				connectedSocket.address.sin_addr.S_un.S_un_b.s_b4,
				ntohs(connectedSocket.address.sin_port));
			ImGui::Text("Player name: %s", connectedSocket.playerName.c_str());
		}

		PrintMessages();

		ImGui::End();
	}

	return true;
}

void ModuleNetworkingServer::PrintMessages()
{
	for (auto item = messages.begin(); item != messages.end(); item++)
	{
		switch ((*item)->type)
		{
		case CL_HELLO:
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
			ImGui::Text("%s Joined the server", (*item)->name.c_str());
			ImGui::PopStyleColor();
			break;
		}
		case CL_STANDARD_MESSAGE:
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
			ImGui::Text("%s: %s", (*item)->name.c_str(), (*item)->message.c_str());
			ImGui::PopStyleColor();
			break;
		}
		}

	}
}

bool ModuleNetworkingServer::sendMessage(MessageType type, SOCKET s, const char * message,std::string user)
{
	bool ret = true;

	std::string _message = message;
	OutputMemoryStream packet;
	packet << user;
	packet << type;
	packet << _message;

	if (!sendPacket(packet, s))
	{
		reportError("Error sending the message");
		ret = false;
	}
	return ret;
}

bool ModuleNetworkingServer::broadcastPacket(OutputMemoryStream& packet)
{
	bool ret = true;
	for (auto &connectedSocket : connectedSockets)
	{
		if (!sendPacket(packet, connectedSocket.socket))
			ret = false;
	}
	return ret;
}



//////////////////////////////////////////////////////////////////////
// ModuleNetworking virtual methods
//////////////////////////////////////////////////////////////////////

bool ModuleNetworkingServer::isListenSocket(SOCKET socket) const
{
	return socket == listenSocket;
}

void ModuleNetworkingServer::onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress)
{
	// Add a new connected socket to the list
	sockets.push_back(socket);

	ConnectedSocket connectedSocket;
	connectedSocket.socket = socket;
	connectedSocket.address = socketAddress;
	connectedSockets.push_back(connectedSocket);
}

void ModuleNetworkingServer::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	Message* message = new Message;
	packet >> message->name;
	packet >>message->type;
	packet >> message->message;

	if (message->type == CL_HELLO)
	{
		if (!checkAvailability(message->name))
		{
			sendMessage(SE_UNABLE_TO_CONNECT, socket, "Unable to join server");
			return;
		}
		for (auto &connectedSocket : connectedSockets)
		{
			if (connectedSocket.socket == socket)
			{
				connectedSocket.playerName = message->name;
				std::string tmp_message = "Welcome to the chat " + message->name;
				sendMessage(SE_WELCOME, connectedSocket.socket, tmp_message.c_str());
			}
		}
		OutputMemoryStream out_packet;
		out_packet << message->name;
		out_packet << message->type;
		out_packet << message->message;
		broadcastPacket(out_packet);
	}
	else if (message->type == CL_STANDARD_MESSAGE)
	{
		OutputMemoryStream out_packet;
		out_packet << message->name;
		out_packet << message->type;
		out_packet << message->message;
		broadcastPacket(out_packet);
	}
	else if (message->type == CL_COMMAND)
	{
		HandleCommands(message);
	}
	messages.push_back(message);
}

void ModuleNetworkingServer::onSocketDisconnected(SOCKET socket)
{
	// Remove the connected socket from the list
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		auto &connectedSocket = *it;
		if (connectedSocket.socket == socket)
		{
			connectedSockets.erase(it);
			break;
		}
	}
}

bool ModuleNetworkingServer::checkAvailability(std::string & name)
{
	bool ret = true;
	if (name == "SYSTEM" || name == "system" || name == "System")
		ret = false;
	for (auto connectedSocket : connectedSockets)
	{
		if (connectedSocket.playerName == name)
			ret = false;
	}
	return ret;
}

void ModuleNetworkingServer::HandleCommands(Message* message)
{
	std::string tmp_message = message->message;
	if (tmp_message.substr(1, 4) == "help")
	{
		SendMessageToUser(message->name, "/help to ask for existing commands \n/whisper to send a private message to a user ", SE_SYSTEM_MSG);
	}
	else if (tmp_message.substr(1, 8) == "whisper ")
	{
		std::string whispered = tmp_message.substr(9, tmp_message.size());
		whispered = whispered.substr(0, whispered.find_first_of(" "));
		std::string _message = tmp_message.substr(10 + whispered.size(), tmp_message.size());
		SendMessageToUser(whispered, _message, CL_WHISPER, message->name);
	}
	else
		SendMessageToUser(message->name, "Unknown Command. Type /help to check the list of avalible commands", SE_ERROR);
}

void ModuleNetworkingServer::SendMessageToUser(std::string& user, std::string message, MessageType type, std::string sender)
{
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		auto& connectedSocket = *it;
		if (connectedSocket.playerName == user)
		{
			sendMessage(type, connectedSocket.socket, message.c_str(), sender);
			return;
		}
	}
	for (auto it = connectedSockets.begin(); it != connectedSockets.end(); ++it)
	{
		auto& connectedSocket = *it;
		if (connectedSocket.playerName == sender)
		{
			std::string tmp = ("Couldn't find user %s", user.c_str());
			sendMessage(SE_ERROR, connectedSocket.socket,tmp.c_str());
			return;
		}
	}
}

