#pragma once

// Add as many messages as you need depending on the
// functionalities that you decide to implement.

//enum class ClientMessage
//{
//	Hello,
//	StandardMessage
//};
//
//enum class ServerMessage
//{
//	Welcome,
//	UserDisconnected
//};

enum MessageType
{
	CL_HELLO,
	CL_STANDARD_MESSAGE,
	CL_COMMAND,
	CL_WHISPER,
	SE_WELCOME,
	SE_SYSTEM_MSG,
	SE_USER_DISCONNECTED,
	SE_ERROR,
	SE_UNABLE_TO_CONNECT
};


struct Message
{
	std::string name;
	std::string message;
	MessageType type;
};
