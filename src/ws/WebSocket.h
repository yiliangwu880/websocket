// WebSocket, v1.00 2012-09-13
//
// Description: WebSocket RFC6544 codec, written in C++.
// Homepage: http://katzarsky.github.com/WebSocket
// Author: katzarsky@gmail.com

#pragma once

#include <assert.h>
#include <stdint.h> /* uint8_t */
#include <stdio.h> /* sscanf */
#include <ctype.h> /* isdigit */
#include <stddef.h> /* int */

#include <vector> 
#include <string> 
#include <functional>

using namespace std;

enum WebSocketFrameType {
	ERROR_FRAME=0xFF00,
	INCOMPLETE_FRAME=0xFE00,

	OPENING_FRAME=0x3300,
	CLOSING_FRAME=0x3400,

	INCOMPLETE_TEXT_FRAME=0x01,
	INCOMPLETE_BINARY_FRAME=0x02,

	TEXT_FRAME=0x81,
	BINARY_FRAME=0x82,

	PING_FRAME=0x19,
	PONG_FRAME=0x1A
};

enum class WsFrameRet {
	INCOMPLETE_LEN, //帧头长度信息未接收完整
	INCOMPLETE_FRAME, // 未接收完整帧
	COMPLITE,
	ERROR,
};

const size_t MAX_FRAME_LEN = 1024 * 10; //server 最大接收帧长度
class BaseWsSvr
{
	bool m_isConnect = false; //true表示通过 ws握手协议

public:
	//接收握手协议
	void RevHandshakeBuf(const char *in, size_t len);
	//接收数据帧消息，会修改in指向内存
	//@param frameLen return COMPLITE 的情况，表示处理完in长度. 通常用来释放来源 in内存用
	//			      return INCOMPLETE_FRAME 的情况，表示 in需要多少字节才能完成一帧处理。 通常用来控制等in接收完整再调用本函数，提高效率。
	WsFrameRet RevFrameBuf(const char *in, size_t len, size_t &frameLen);
	void Send(uint8_t opcode, const char* msg, int msg_len);
	inline bool IsCon() { return m_isConnect; }
	 
protected:
	virtual void OnRevMsg(WebSocketFrameType type, const char *msg, size_t len) = 0;
	virtual void OnSendBuf(const char *buf, size_t len) = 0;
	virtual void OnError(const char *errInfo);
	virtual void OnRevPing(){};
	virtual void OnRevPong(){};


private:
	WsFrameRet GetFrame(const char* in_buffer, size_t in_length, size_t &frameLen);
};

class BaseWsClient
{
	bool m_isConnect = false; //true表示通过 ws握手协议
	std::string m_nonce_base64;
public:
	//发送请求握手协议
	void SendHandshakeReq(const string &path, const string &host);
	//接收握手协议, return true表示握手成功
	bool RevHandshakeBuf(const char *in, size_t len);
	//@param Send 
	void Send(uint8_t opcode, const char* msg, size_t msg_len);
	//接收数据帧消息，会修改in指向内存
	//@param frameLen return COMPLITE 的情况，表示处理完in长度. 通常用来释放来源 in内存用
	//			      return INCOMPLETE_FRAME 的情况，表示 in需要多少字节才能完成一帧处理。 通常用来控制等in接收完整再调用本函数，提高效率。
	WsFrameRet RevFrameBuf(const char *in, size_t len, size_t &frameLen);
	inline bool IsCon() { return m_isConnect; }

protected:
	virtual void OnConnect()=0;
	virtual void OnRevMsg(WebSocketFrameType type, const char *msg, size_t len) = 0;
	virtual void OnSendBuf(const char *buf, size_t len) = 0;
	virtual void OnError(const char *errInfo);
	virtual void OnRevPing() {};
	virtual void OnRevPong() {};

private:
	WsFrameRet GetFrame(const char* in_buffer, size_t in_length, size_t &frameLen);

	string Trim(string str);
	vector<string> Explode(string theString, string theDelimiter, bool theIncludeEmptyStrings = false);
};