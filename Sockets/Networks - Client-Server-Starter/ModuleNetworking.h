#pragma once


class ModuleNetworking : public Module
{
private:

	//////////////////////////////////////////////////////////////////////
	// Module virtual methods
	//////////////////////////////////////////////////////////////////////

	bool init() override;

	bool preUpdate() override;

	bool cleanUp() override;



	//////////////////////////////////////////////////////////////////////
	// Socket event callbacks
	//////////////////////////////////////////////////////////////////////

	virtual bool isListenSocket(SOCKET socket) const { return false; }

	virtual void onSocketConnected(SOCKET socket, const sockaddr_in &socketAddress) { }

	virtual void onSocketReceivedData(SOCKET s, const InputMemoryStream& packet) = 0;

	virtual void onSocketDisconnected(SOCKET s) = 0;



protected:
	static bool sendPacket(const OutputMemoryStream& packet, SOCKET socket);

	virtual void PrintMessages() = 0;

	sockaddr_in address;

	std::vector<SOCKET> sockets;
	std::vector<Message*> messages;

	void addSocket(SOCKET socket);

	void disconnect();

	static void reportError(const char *Message);
};

