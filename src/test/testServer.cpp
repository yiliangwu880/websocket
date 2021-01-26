/* 
部分代码抄 http://www.zaphoyd.com/websocketpp/
*/

#include <string>
#include "test/unit_test.h"
#include "WebSocket.h"

using namespace std;

namespace websocketpp{ //兼容抄来的代码
static char const user_agent[] = "WebSocket++/0.8.2";
}

class TestSvr : public BaseWsSvr
{
public:
	string m_frame; //接收 frame内容
	WebSocketFrameType m_frameType; //接收 frame type
	string m_sendBuf; //发送到网络的缓存
	virtual void OnRevMsg(WebSocketFrameType type, const uint8_t *msg, size_t len) 
	{
		//printf("msg len=%ld\n", len);
		m_frame.assign((const char*)msg, len);
		m_frameType = type;
	}
	virtual void OnSendBuf(const uint8_t *buf, size_t len)
	{
		m_sendBuf.append((const char*)buf, len);
	}
};

class TestClient : public BaseWsClient
{
public:
	string m_frame; //接收 frame内容
	WebSocketFrameType m_frameType; //接收 frame type
	string m_sendBuf; //发送到网络的缓存

	bool m_isCallOnCon = false;
	virtual void OnConnect()
	{
		m_isCallOnCon = true;
	}
	virtual void OnRevMsg(WebSocketFrameType type, const uint8_t *msg, size_t len)
	{
		m_frame.assign((const char*)msg, len);
		m_frameType = type;
	}
	virtual void OnSendBuf(const uint8_t *buf, size_t len)
	{
		m_sendBuf.append((const char*)buf, len);
	}
};


void PrintBinary2Hex(const string &bin)
{
	printf("\n");
	for ( const char &v : bin)
	{ 
		printf("%x", (uint8_t)v);
	}
	printf("\n");
}

UNITTEST(basic_websocket_request)
{
	TestSvr svr;
	std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";
	std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: ";
	output += websocketpp::user_agent;
	output += "\r\nUpgrade: websocket\r\n\r\n";

	UNIT_ASSERT(!svr.IsCon());
	svr.RevHandshakeBuf((uint8_t *)input.c_str(), input.length());

	UNIT_ASSERT(svr.m_sendBuf == output);
	UNIT_ASSERT(svr.IsCon());
}
//发送
UNITTEST(svrSendFrame)
{
	TestSvr svr;
	TestClient client;

	client.SendHandshakeReq("/", "www.example.com");
	svr.RevHandshakeBuf((uint8_t *)client.m_sendBuf.c_str(), client.m_sendBuf.length());
	UNIT_ASSERT(svr.IsCon());

	client.RevHandshakeBuf((uint8_t *)svr.m_sendBuf.c_str(), svr.m_sendBuf.length());
	UNIT_ASSERT(client.IsCon());
	UNIT_ASSERT(client.m_isCallOnCon);

	//client send
	{
		client.m_sendBuf.clear();
		string sendStr = "abc";
		UNIT_INFO("sendStr = %s %d", sendStr.c_str(), sendStr.length());
		client.Send(WebSocketFrameType::BINARY_FRAME, (uint8_t *)sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = svr.RevFrameBuf((uint8_t *)client.m_sendBuf.c_str(), client.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(client.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_INFO("frame = %s %d", svr.m_frame.c_str(), svr.m_frame.length());
		UNIT_ASSERT(svr.m_frame == sendStr);
	}

	{
		client.m_sendBuf.clear();
		string sendStr = "cc";
		client.Send(WebSocketFrameType::BINARY_FRAME, (uint8_t *)sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = svr.RevFrameBuf((uint8_t *)client.m_sendBuf.c_str(), client.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(client.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_ASSERT(svr.m_frame == sendStr);
	}

	//svr send
	{
		svr.m_sendBuf.clear();
		string sendStr = "12";
		svr.Send(WebSocketFrameType::TEXT_FRAME, (uint8_t *)sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = client.RevFrameBuf((uint8_t *)svr.m_sendBuf.c_str(), svr.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(svr.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_ASSERT(client.m_frame == sendStr);
		UNIT_INFO("frame = %s", client.m_frame.c_str());
	}
	{
		svr.m_sendBuf.clear();
		string sendStr = "123";
		svr.Send(WebSocketFrameType::BINARY_FRAME, (uint8_t *)sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = client.RevFrameBuf((uint8_t *)svr.m_sendBuf.c_str(), svr.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(svr.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_ASSERT(client.m_frame == sendStr);
		UNIT_INFO("frame = %s", client.m_frame.c_str());
	}
}

//接收

//ping
//pong