#pragma once

#include "ModuleNetworking.h"

class ModuleNetworkingServer : public ModuleNetworking
{
public:

	//////////////////////////////////////////////////////////////////////
	// ModuleNetworkingServer public methods
	//////////////////////////////////////////////////////////////////////

	bool start(int port);

	bool isRunning() const;



private:

	//////////////////////////////////////////////////////////////////////
	// Module virtual methods
	//////////////////////////////////////////////////////////////////////

	bool update() override;

	bool gui() override;

	void PrintMessages() override;
	bool sendMessage(MessageType type, SOCKET s, const char * message, std::string user = "SYSTEM");
	bool broadcastPacket(OutputMemoryStream &packet);
	//////////////////////////////////////////////////////////////////////
	// ModuleNetworking virtual methods
	//////////////////////////////////////////////////////////////////////

	bool isListenSocket(SOCKET socket) const override;

	void onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress) override;

	void onSocketReceivedData(SOCKET socket, const InputMemoryStream& packet) override;

	void onSocketDisconnected(SOCKET socket) override;

	bool checkAvailability(std::string& name);

	void HandleCommands(Message* message);

	void SendMessageToUser(std::string& user, std::string message,MessageType type,std::string sender = "SYSTEM");
	//////////////////////////////////////////////////////////////////////
	// State
	//////////////////////////////////////////////////////////////////////

	enum class ServerState
	{
		Stopped,
		Listening
	};

	ServerState state = ServerState::Stopped;

	SOCKET listenSocket;

	struct ConnectedSocket
	{
		sockaddr_in address;
		SOCKET socket;
		std::string playerName;
	};

	std::vector<ConnectedSocket> connectedSockets;
};

