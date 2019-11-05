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
		OutputMemoryStream packet;
		packet << playerName;
		packet << CL_HELLO;
		packet << "Hello";

		if (sendPacket(packet, client_socket))
		{
			state = ClientState::Logging;
		}
		else
		{
			printWSErrorAndContinue("Error sending name %s to the server, continuing...", playerName.c_str());
			disconnect();
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

		ImGuiWindowFlags window_flags = ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar | ImGuiWindowFlags_AlwaysHorizontalScrollbar;
		ImGui::BeginChild("Child1", ImVec2(ImGui::GetWindowContentRegionWidth(), ImGui::GetWindowContentRegionHeight() - (texSize.y+30)), false, window_flags);
		PrintMessages();
		ImGui::EndChild();

		static char message[256];
		if (ImGui::InputText("", message, 256, ImGuiInputTextFlags_EnterReturnsTrue) || ImGui::Button("Send"))
		{
			sendMessage(CL_STANDARD_MESSAGE, message);
			memset(message, 0, sizeof(message));
		}
		if (ImGui::IsItemHovered() || (ImGui::IsRootWindowOrAnyChildFocused() && !ImGui::IsAnyItemActive() && !ImGui::IsMouseClicked(0)))
			ImGui::SetKeyboardFocusHere(-1); // Auto focus previous widget
		ImGui::End();
	}

	return true;
}

void ModuleNetworkingClient::PrintMessages()
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
		case SE_WELCOME:
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.5f, 1.0f));
			ImGui::Text("%s: %s", (*item)->name.c_str(), (*item)->message.c_str());

			ImGui::PopStyleColor();
			break;
		}
		case CL_WHISPER:
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, .5f, 1.0f, 1.0f));
			ImGui::Text("%s whispers: %s", (*item)->name.c_str(), (*item)->message.c_str());
			ImGui::PopStyleColor();
			break;
		}
		case SE_SYSTEM_MSG:
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, .5f, .5f, 1.0f));
			ImGui::Text("%s: %s", (*item)->name.c_str(), (*item)->message.c_str());
			ImGui::PopStyleColor();
			break;
		}
		case SE_ERROR:
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, .0f, .0f, 1.0f));
			ImGui::Text("%s: %s", (*item)->name.c_str(), (*item)->message.c_str());
			ImGui::PopStyleColor();
			break;
		}
		}

	}
}

bool ModuleNetworkingClient::sendMessage(MessageType type, char * message, ...)
{
	bool ret = true;
	std::string _message = message;
	OutputMemoryStream packet;
	packet << playerName;
	if (message[0] == '/')
		packet << CL_COMMAND;
	else
		packet << type;

	packet << _message;


	if (!sendPacket(packet, client_socket))
	{
		reportError("Error sending the message");
		ret = false;
	}
	return ret;
}

void ModuleNetworkingClient::onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet)
{
	Message* message = new Message;
	packet >> message->name;
	packet >> message->type;
	packet >> message->message;

	if (message->type == SE_UNABLE_TO_CONNECT)
	{
		ELOG("Unable to connect to server, invalid name");
		state = ClientState::Stopped;
		disconnect();
	}

	messages.push_back(message);
}

void ModuleNetworkingClient::onSocketDisconnected(SOCKET socket)
{
	state = ClientState::Stopped;
}


