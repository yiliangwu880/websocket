/* 
部分代码抄 http://www.zaphoyd.com/websocketpp/
*/

#include <string>
#include "unit_test.h"
#include "../ws/WebSocket.h"
#include <random>

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
	virtual void OnRevMsg(WebSocketFrameType type, const char *msg, size_t len) 
	{
		//printf("msg len=%ld\n", len);
		m_frame.assign(msg, len);
		m_frameType = type;
	}
	virtual void OnSendBuf(const char *buf, size_t len)
	{
		m_sendBuf.append(buf, len);
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
	virtual void OnRevMsg(WebSocketFrameType type, const char *msg, size_t len)
	{
		m_frame.assign(msg, len);
		m_frameType = type;
	}
	virtual void OnSendBuf(const char *buf, size_t len)
	{
		m_sendBuf.append(buf, len);
	}
};

namespace{
//void PrintBinary2Hex(const string &bin)
//{
//	printf("\n");
//	for ( const char &v : bin)
//	{ 
//		printf("%x", (uint8_t)v);
//	}
//	printf("\n");
//}
}

UNITTEST(basic_websocket_request)
{
	TestSvr svr;
	std::string input = "GET / HTTP/1.1\r\nHost: www.example.com\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\nOrigin: http://www.example.com\r\n\r\n";
	std::string output = "HTTP/1.1 101 Switching Protocols\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: s3pPLMBiTxaQ9kYGzzhZRbK+xOo=\r\nServer: ";
	output += websocketpp::user_agent;
	output += "\r\nUpgrade: websocket\r\n\r\n";

	UNIT_ASSERT(!svr.IsCon());
	svr.RevHandshakeBuf(input.c_str(), input.length());

	UNIT_ASSERT(svr.m_sendBuf == output);
	UNIT_ASSERT(svr.IsCon());
}
//发送 //接收
UNITTEST(svrSendFrame)
{
	TestSvr svr;
	TestClient client;

	client.SendHandshakeReq("/", "www.example.com");
	svr.RevHandshakeBuf(client.m_sendBuf.c_str(), client.m_sendBuf.length());
	UNIT_ASSERT(svr.IsCon());

	client.RevHandshakeBuf(svr.m_sendBuf.c_str(), svr.m_sendBuf.length());
	UNIT_ASSERT(client.IsCon());
	UNIT_ASSERT(client.m_isCallOnCon);

	//client send
	{
		client.m_sendBuf.clear();
		string sendStr = "abc";
		UNIT_INFO("sendStr = %s %d", sendStr.c_str(), sendStr.length());
		client.Send(WebSocketFrameType::BINARY_FRAME, sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = svr.RevFrameBuf(client.m_sendBuf.c_str(), client.m_sendBuf.length(), frameLen);
		UNIT_INFO("%d %d", client.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(client.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_INFO("frame = %s %d", svr.m_frame.c_str(), svr.m_frame.length());
		UNIT_ASSERT(svr.m_frame == sendStr);
	}

	{
		client.m_sendBuf.clear();
		string sendStr = "cc";
		client.Send(WebSocketFrameType::BINARY_FRAME, sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = svr.RevFrameBuf(client.m_sendBuf.c_str(), client.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(client.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_ASSERT(svr.m_frame == sendStr);
	}

	//svr send
	{
		svr.m_sendBuf.clear();
		string sendStr = "12";
		svr.Send(WebSocketFrameType::TEXT_FRAME, sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = client.RevFrameBuf(svr.m_sendBuf.c_str(), svr.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(svr.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_ASSERT(client.m_frame == sendStr);
		UNIT_INFO("frame = %s", client.m_frame.c_str());
	}
	{
		svr.m_sendBuf.clear();
		string sendStr = "123";
		svr.Send(WebSocketFrameType::BINARY_FRAME, sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = client.RevFrameBuf(svr.m_sendBuf.c_str(), svr.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(svr.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_ASSERT(client.m_frame == sendStr);
		UNIT_INFO("frame = %s", client.m_frame.c_str());
	}
}

//粘包 
UNITTEST(Unpack)
{
	TestSvr svr;
	TestClient client;

	client.SendHandshakeReq("/", "www.example.com");
	svr.RevHandshakeBuf(client.m_sendBuf.c_str(), client.m_sendBuf.length());
	UNIT_ASSERT(svr.IsCon());

	client.RevHandshakeBuf(svr.m_sendBuf.c_str(), svr.m_sendBuf.length());
	UNIT_ASSERT(client.IsCon());
	UNIT_ASSERT(client.m_isCallOnCon);

	//svr send
	for (int nouse = 0; nouse < 1 ; nouse++)
	{
		svr.m_sendBuf.clear();
		string sendStr1 = "12";
		svr.Send(WebSocketFrameType::TEXT_FRAME, sendStr1.c_str(), sendStr1.length());
		size_t bufLen1 = svr.m_sendBuf.length();
		string sendStr2 = "abc";
		svr.Send(WebSocketFrameType::BINARY_FRAME, sendStr2.c_str(), sendStr2.length());
		size_t bufLen2 = svr.m_sendBuf.length()- bufLen1;

		unsigned seed = (uint32_t)time(0);
		std::default_random_engine generator(seed);
		std::uniform_int_distribution<int> distribution(1, 6);
		size_t totalRevLen = 0;
		for (int i = 0; i < 1000 ; i++)
		{
			if (0== svr.m_sendBuf.length())
			{
				break;
			}
			size_t onceLen = distribution(generator);  // generates number in the range 1..6
			if (totalRevLen+onceLen >= svr.m_sendBuf.length())
			{
				onceLen = svr.m_sendBuf.length() - totalRevLen;
			}
			totalRevLen += onceLen;
			UNIT_INFO("once_len=%d totalRevLen=%d total=%ld", onceLen, totalRevLen, svr.m_sendBuf.length());

			const char *pBuf = svr.m_sendBuf.c_str();
			size_t frameLen = 0;
			WsFrameRet ret = client.RevFrameBuf(pBuf, totalRevLen, frameLen);
			if (WsFrameRet::COMPLITE == ret)
			{
				UNIT_ASSERT(bufLen1 == frameLen || bufLen2 == frameLen);
				UNIT_ASSERT(client.m_frame == sendStr1 || client.m_frame == sendStr2);
				UNIT_INFO("frame = %s", client.m_frame.c_str());
				string remainBuf(svr.m_sendBuf.begin() + frameLen, svr.m_sendBuf.end());
				svr.m_sendBuf = remainBuf;
				UNIT_INFO("remainbuf len=", svr.m_sendBuf.length());
			}	
		}
	}
}


//16字节长度frame
UNITTEST(frameLen16bytes)
{
	TestSvr svr;
	TestClient client;

	client.SendHandshakeReq("/", "www.example.com");
	svr.RevHandshakeBuf(client.m_sendBuf.c_str(), client.m_sendBuf.length());
	UNIT_ASSERT(svr.IsCon());

	client.RevHandshakeBuf(svr.m_sendBuf.c_str(), svr.m_sendBuf.length());
	UNIT_ASSERT(client.IsCon());
	UNIT_ASSERT(client.m_isCallOnCon);

	//client send
	{
		client.m_sendBuf.clear();
		string sendStr = "abc";
		sendStr.append(1024, '0');
		sendStr.append("e");
		client.Send(WebSocketFrameType::BINARY_FRAME, sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = svr.RevFrameBuf(client.m_sendBuf.c_str(), client.m_sendBuf.length(), frameLen);
		UNIT_INFO("%d %d", client.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(client.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_ASSERT('e' == svr.m_frame.back());
		UNIT_ASSERT(svr.m_frame == sendStr);
	}


	//svr send
	{
		svr.m_sendBuf.clear();
		string sendStr = "12";
		sendStr.append(1024, '0');
		sendStr.append("e");
		svr.Send(WebSocketFrameType::TEXT_FRAME, sendStr.c_str(), sendStr.length());

		size_t frameLen = 0;
		WsFrameRet ret = client.RevFrameBuf(svr.m_sendBuf.c_str(), svr.m_sendBuf.length(), frameLen);
		UNIT_ASSERT(svr.m_sendBuf.length() == frameLen);
		UNIT_ASSERT(WsFrameRet::COMPLITE == ret);
		UNIT_ASSERT(client.m_frame == sendStr);
	}
}

//ping
//pong