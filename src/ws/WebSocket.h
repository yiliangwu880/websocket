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
	INCOMPLETE_LEN, //֡ͷ������Ϣδ��������
	INCOMPLETE_FRAME, // δ��������֡
	COMPLITE,
	ERROR,
};

const size_t MAX_FRAME_LEN = 1024 * 10; //server ������֡����
class BaseWsSvr
{
	bool m_isConnect = false; //true��ʾͨ�� ws����Э��

public:
	//��������Э��
	void RevHandshakeBuf(const char *in, size_t len);
	//��������֡��Ϣ�����޸�inָ���ڴ�
	//@param frameLen return COMPLITE ���������ʾ������in����. ͨ�������ͷ���Դ in�ڴ���
	//			      return INCOMPLETE_FRAME ���������ʾ in��Ҫ�����ֽڲ������һ֡���� ͨ���������Ƶ�in���������ٵ��ñ����������Ч�ʡ�
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
	bool m_isConnect = false; //true��ʾͨ�� ws����Э��
	std::string m_nonce_base64;
public:
	//������������Э��
	void SendHandshakeReq(const string &path, const string &host);
	//��������Э��, return true��ʾ���ֳɹ�
	bool RevHandshakeBuf(const char *in, size_t len);
	//@param Send 
	void Send(uint8_t opcode, const char* msg, size_t msg_len);
	//��������֡��Ϣ�����޸�inָ���ڴ�
	//@param frameLen return COMPLITE ���������ʾ������in����. ͨ�������ͷ���Դ in�ڴ���
	//			      return INCOMPLETE_FRAME ���������ʾ in��Ҫ�����ֽڲ������һ֡���� ͨ���������Ƶ�in���������ٵ��ñ����������Ч�ʡ�
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