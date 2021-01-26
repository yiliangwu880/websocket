

#include "WebSocket.h"

#include "base64/base64.h"
#include "sha1/sha1.h"

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <string.h>
#include <random>
#include <sstream> 

using namespace std;

static char const user_agent[] = "WebSocket++/0.8.2";

union WsHead {
	struct {
		uint8_t OPCODE : 4;
		uint8_t RSV : 3;
		uint8_t FIN : 1; //1表示完整帧， 0表示部分帧

		uint8_t payload_length7 : 7;
		uint8_t MASK : 1;
	};
	uint16_t d;
};
class WsHandshake
{
	string resource;
	string host;
	string origin;
	string protocol;
	string key;
	bool m_isRevReqHandshake = false;
public:

	WsHandshake();

	/**
	 * @param input_frame .in. pointer to input frame
	 * @param input_len .in. length of input frame
	 * @return [WS_INCOMPLETE_FRAME, WS_ERROR_FRAME, WS_OPENING_FRAME]
	 */
	WebSocketFrameType ParseHandshake(const unsigned char* input_frame, int input_len, string &rsp);

private:
	string AnswerHandshake();

	string Trim(string str);
	vector<string> Explode(string theString, string theDelimiter, bool theIncludeEmptyStrings = false);
};

WsHandshake::WsHandshake() {
	
}

WebSocketFrameType WsHandshake::ParseHandshake(const unsigned char* input_frame, int input_len, string &rsp)
{
	if (0 != memcmp(&input_frame[input_len-4], "\r\n\r\n", 4))
	{
		return INCOMPLETE_FRAME;
	}

	// 1. copy char*/len into string
	// 2. try to parse headers until \r\n occurs
	string headers((char*)input_frame, input_len); 


	size_t header_end = headers.find("\r\n\r\n");

	if(header_end == string::npos) { // end-of-headers not found - do not parse
		return INCOMPLETE_FRAME;
	}

	headers.resize(header_end); // trim off any data we don't need after the headers
	vector<string> headers_rows = Explode(headers, string("\r\n"));
	for(size_t i=0; i<headers_rows.size(); i++) {
		string& header = headers_rows[i];
		if(header.find("GET") == 0) {
			vector<string> get_tokens = Explode(header, string(" "));
			if(get_tokens.size() >= 2) {
				this->resource = get_tokens[1];
			}
		}
		else {
			size_t pos = header.find(":");
			if(pos != string::npos) {
				string header_key(header, 0, pos);
				string header_value(header, pos+1);
				header_value = Trim(header_value);
				if(header_key == "Host") this->host = header_value;
				else if(header_key == "Origin") this->origin = header_value;
				else if(header_key == "Sec-WebSocket-Key") this->key = header_value;
				else if(header_key == "Sec-WebSocket-Protocol") this->protocol = header_value;
			}
		}
	}

	//this->key = "dGhlIHNhbXBsZSBub25jZQ==";
	//printf("PARSED_KEY:%s \n", this->key.data());

	//return FrameType::OPENING_FRAME;
	m_isRevReqHandshake = true;
	rsp = AnswerHandshake();
	return OPENING_FRAME;
}

string WsHandshake::Trim(string str) 
{
	//printf("TRIM\n");
	const char* whitespace = " \t\r\n";
	string::size_type pos = str.find_last_not_of(whitespace);
	if(pos != string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(whitespace);
		if(pos != string::npos) str.erase(0, pos);
	}
	else {
		return string();
	}
	return str;
}

vector<string> WsHandshake::Explode(	
	string  theString,
    string  theDelimiter,
    bool    theIncludeEmptyStrings)
{
	//printf("EXPLODE\n");
	//UASSERT( theDelimiter.size(), >, 0 );
	
	vector<string> theStringVector;
	size_t  start = 0, end = 0, length = 0;

	while ( end != string::npos )
	{
		end = theString.find( theDelimiter, start );

		// If at end, use length=maxLength.  Else use length=end-start.
		length = (end == string::npos) ? string::npos : end - start;

		if (theIncludeEmptyStrings
			|| (   ( length > 0 ) /* At end, end == length == string::npos */
            && ( start  < theString.size() ) ) )
		theStringVector.push_back( theString.substr( start, length ) );

		// If at end, use start=maxSize.  Else use start=end+delimiter.
		start = (   ( end > (string::npos - theDelimiter.size()) )
              ?  string::npos  :  end + theDelimiter.size()     );
	}
	return theStringVector;
}

string WsHandshake::AnswerHandshake() 
{
	if (!m_isRevReqHandshake)
	{
		return "have not handle request";
	}
    unsigned char digest[20]; // 160 bit sha1 digest

	string answer;
	answer += "HTTP/1.1 101 Switching Protocols\r\n";
	answer += "Connection: Upgrade\r\n";
	if(this->key.length() > 0) {
		string accept_key;
		accept_key += this->key;
		accept_key += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"; //RFC6544_MAGIC_KEY

		//printf("INTERMEDIATE_KEY:(%s)\n", accept_key.data());

		SHA1 sha;
		sha.Input(accept_key.data(), accept_key.size());
		sha.Result((unsigned*)digest);
		
		//printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]); printf("\n");

		//little endian to big endian
		for(int i=0; i<20; i+=4) {
			unsigned char c;

			c = digest[i];
			digest[i] = digest[i+3];
			digest[i+3] = c;

			c = digest[i+1];
			digest[i+1] = digest[i+2];
			digest[i+2] = c;
		}

		//printf("DIGEST:"); for(int i=0; i<20; i++) printf("%02x ",digest[i]); printf("\n");

		accept_key = base64_encode((const unsigned char *)digest, 20); //160bit = 20 bytes/chars

		answer += "Sec-WebSocket-Accept: "+(accept_key)+"\r\n";
		answer += "Server: " + string(user_agent) + "\r\n";
		answer += "Upgrade: websocket\r\n";
	}
	if(this->protocol.length() > 0) {
		answer += "Sec-WebSocket-Protocol: "+(this->protocol)+"\r\n";
	}
	answer += "\r\n";
	return answer;

	//return WS_OPENING_FRAME;
}

void BaseWsSvr::Send(uint8_t opcode, unsigned char* msg, int msg_length)
{
	unsigned char buffer[50];
	int pos = 0;
	int size = msg_length; 
	buffer[pos++] = (unsigned char)opcode; // text frame

	if(size <= 125) {
		buffer[pos++] = size;
	}
	else if(size <= 65535) {
		buffer[pos++] = 126; //16 bit length follows
		
		buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
		buffer[pos++] = size & 0xFF;
	}
	else { // >2^16-1 (65535)
		buffer[pos++] = 127; //64 bit length follows
		
		// write 8 bytes length (significant first)
		
		// since msg_length is int it can be no longer than 4 bytes = 2^32-1
		// padd zeroes for the first 4 bytes
		for(int i=3; i>=0; i--) {
			buffer[pos++] = 0;
		}
		// write the actual 32bit msg_length in the next 4 bytes
		for(int i=3; i>=0; i--) {
			buffer[pos++] = ((size >> 8*i) & 0xFF);
		}
	}
	//memcpy((void*)(buffer + pos), msg, size);
	OnSendBuf(buffer, pos);
	OnSendBuf(msg, size);
}

WsFrameRet BaseWsSvr::GetFrame(uint8_t* in_buffer, size_t in_length, size_t &frameLen)
{
	//printf("getTextFrame()\n");
	if(in_length < 3) return WsFrameRet::INCOMPLETE_LEN;

	WsHead *pHead = (WsHead *)&in_buffer[0];
	//unsigned char msg_opcode = in_buffer[0] & 0x0F;
	//unsigned char msg_fin = (in_buffer[0] >> 7) & 0x01;
	//unsigned char msg_masked = (in_buffer[1] >> 7) & 0x01;

	//printf("%x %x", in_buffer[0], in_buffer[1]); //82  1000 0010 | 83 1000 0011
	//printf("\n%d %d %d %d", pHead->FIN, pHead->RSV, pHead->OPCODE, pHead->MASK pHead->payload_length7);//0 1 8 1
	//printf("\n%d %d\n", pHead->MASK, pHead->MASK);
	
	// *** message decoding 

	size_t payload_length = 0;
	int pos = 2;
	//int length_field = in_buffer[1] & (~0x80);
	unsigned int mask = 0;

	//printf("IN:"); for(int i=0; i<20; i++) printf("%02x ",buffer[i]); printf("\n");

	if(pHead->payload_length7 <= 125) {
		payload_length = pHead->payload_length7;
	}
	else if(pHead->payload_length7 == 126) { //msglen is 16bit!

		if (in_length < 4) return WsFrameRet::INCOMPLETE_LEN;
		//payload_length = in_buffer[2] + (in_buffer[3]<<8);
		payload_length = (
			(in_buffer[2] << 8) | 
			(in_buffer[3])
		);
		pos += 2;
	}
	else if (pHead->payload_length7 == 127) { //msglen is 64bit!
		if (in_length < 10) return WsFrameRet::INCOMPLETE_LEN;
		payload_length = (uint32_t)(
			((uint64_t)(in_buffer[2]) << 56) |
			((uint64_t)(in_buffer[3]) << 48) |
			((uint64_t)(in_buffer[4]) << 40) |
			((uint64_t)(in_buffer[5]) << 32) |
			((uint64_t)(in_buffer[6]) << 24) |
			((uint64_t)(in_buffer[7]) << 16) |
			((uint64_t)(in_buffer[8]) << 8) |
			((uint64_t)(in_buffer[9]))
		); 
		pos += 8;
	}
	
	frameLen = payload_length + pos + 4; //加mask 4 字节
	//printf("payload_length=%ld pos=%d\n", payload_length, pos);
	if(in_length < payload_length+pos) {
		return WsFrameRet::INCOMPLETE_FRAME;
	}

	if (!pHead->MASK) {
		OnError("message from client not masked");
		return WsFrameRet::ERROR;
	}
	mask = *((unsigned int*)(in_buffer+pos));
	//printf("MASK: %08x\n", mask);
	pos += 4;


	uint8_t out_buffer[MAX_FRAME_LEN];
	size_t out_size = sizeof(out_buffer);
	if (payload_length > out_size) {
		OnError("message from client not masked");
		return WsFrameRet::ERROR;
	}
	// unmask data:
	unsigned char* c = in_buffer+pos;
	for(size_t i=0; i<payload_length; i++) {
		out_buffer[i] = c[i] ^ ((unsigned char*)(&mask))[i%4];
	}
	WebSocketFrameType type;
	if(pHead->OPCODE == 0x0)	  type = (pHead->FIN)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME; // continuation frame ?
	else if(pHead->OPCODE == 0x1) type = (pHead->FIN)?TEXT_FRAME:INCOMPLETE_TEXT_FRAME;
	else if(pHead->OPCODE == 0x2) type = (pHead->FIN)?BINARY_FRAME:INCOMPLETE_BINARY_FRAME;
	else if (pHead->OPCODE == 0x9)
	{
		type = PING_FRAME;
		OnRevPing();
		Send(PONG_FRAME, NULL, 0);
		return WsFrameRet::COMPLITE;
	}
	else if (pHead->OPCODE == 0xA) {
		type = PONG_FRAME;
		OnRevPong();
		return WsFrameRet::COMPLITE;
	}
	else
	{
		return WsFrameRet::ERROR;
	}
	OnRevMsg(type, out_buffer, payload_length);
	return WsFrameRet::COMPLITE;
}




void BaseWsSvr::RevHandshakeBuf(uint8_t *in, size_t len)
{
	const size_t MAX_STR_LEN = 5000; //最大解析长度，超了认为错误
	if (m_isConnect)
	{
		OnError("already handshake. repeated call RevHandshakeBuf");
		return ;
	}
	if (len < 4)
	{
		return;
	}
	if (len >= MAX_STR_LEN)
	{
		OnError("parse handshake str is too long");
		return;
	}
	if (0 != memcmp(&in[len - 4], "\r\n\r\n", 4))
	{
		return;
	}

	WsHandshake hs;
	string rsp;
	WebSocketFrameType ret = hs.ParseHandshake(in, len, rsp);
	if (ret != OPENING_FRAME)
	{
		OnError("parse handshake error");
		return;
	}
	OnSendBuf((const uint8_t *)rsp.c_str(), rsp.length());
	m_isConnect = true;
}

WsFrameRet BaseWsSvr::RevFrameBuf(uint8_t *in, size_t len, size_t &frameLen)
{
	if (!m_isConnect)
	{
		OnError("have not  handshake.");
		return WsFrameRet::ERROR;
	} 

	WsFrameRet ret = GetFrame(in, len, frameLen);
	if (ret == WsFrameRet::ERROR)
	{
		OnError("svr rev error frame\n");
	}
	return ret;
}


void BaseWsSvr::OnError(const char *errInfo)
{
	printf("%s", errInfo);
}

void BaseWsClient::SendHandshakeReq(const string &path, const string &host)
{
	std::stringstream request;
	request << "GET " << path << " HTTP/1.1" << "\r\n";
	request << "Host: " << host << "\r\n";
	request << "Connection: Upgrade\r\n";
	request << "Upgrade: websocket\r\n";
	request << "Sec-WebSocket-Version: 13\r\n";

	// Make random 16-byte nonce
	std::string nonce;
	nonce.reserve(16);
	std::uniform_int_distribution<unsigned short> dist(0, 255);
	std::random_device rd;
	for (std::size_t c = 0; c < 16; c++)
		nonce += static_cast<char>(dist(rd));

	m_nonce_base64 = base64_encode((const uint8_t *)nonce.c_str(), nonce.length());
	request << "Sec-WebSocket-Key: " << m_nonce_base64 << "\r\n";
	//for (auto &header_field : config.header)
	//	request << header_field.first << ": " << header_field.second << "\r\n";
	request << "\r\n";
	string s = request.str();
	OnSendBuf((const uint8_t*)s.c_str(), s.length());
}

void BaseWsClient::RevHandshakeBuf(uint8_t *in, size_t len)
{
	const size_t MAX_STR_LEN = 5000; //最大解析长度，超了认为错误
	if (m_isConnect)
	{
		OnError("already handshake. repeated call RevHandshakeBuf");
		return;
	}
	if (len < 4)
	{
		return;
	}
	if (len >= MAX_STR_LEN)
	{
		OnError("parse handshake str is too long");
		return;
	}
	if (0 != memcmp(&in[len - 4], "\r\n\r\n", 4))
	{
		return;
	}

	// 1. copy char*/len into string
	// 2. try to parse headers until \r\n occurs
	string headers((char*)in, len);

	vector<string> headers_rows = Explode(headers, string("\r\n"));
	for (const string &header : headers_rows)
	{
		size_t pos = header.find(":");
		if (pos != string::npos) {
			string header_key(header, 0, pos);
			string header_value(header, pos + 1);
			header_value = Trim(header_value);
			if (header_key == "Sec-WebSocket-Accept")
			{

				unsigned char digest[20]; // 160 bit sha1 digest
				string s = m_nonce_base64 + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
				SHA1 sha;
				sha.Input(s.c_str(), s.length());
				sha.Result((unsigned*)digest);
				//little endian to big endian
				for (int i = 0; i < 20; i += 4) {
					unsigned char c;

					c = digest[i];
					digest[i] = digest[i + 3];
					digest[i + 3] = c;

					c = digest[i + 1];
					digest[i + 1] = digest[i + 2];
					digest[i + 2] = c;
				}

				if (base64_decode(header_value) == std::string((const char*)&digest[0], sizeof(digest)))
				{
					m_isConnect = true;
					OnConnect();
				}
				else
				{
					OnError("key error");
				}
			}
		}
	}
}

WsFrameRet BaseWsClient::RevFrameBuf(uint8_t *in, size_t len, size_t &frameLen)
{
	if (!m_isConnect)
	{
		OnError("have not  handshake.");
		return WsFrameRet::ERROR;
	}

	frameLen = 0;
	WsFrameRet ret = GetFrame(in, len, frameLen);
	if (ret == WsFrameRet::ERROR)
	{
		OnError("client rev error frame\n");
	}
	return ret;
}

string BaseWsClient::Trim(string str)
{
	//printf("TRIM\n");
	const char* whitespace = " \t\r\n";
	string::size_type pos = str.find_last_not_of(whitespace);
	if (pos != string::npos) {
		str.erase(pos + 1);
		pos = str.find_first_not_of(whitespace);
		if (pos != string::npos) str.erase(0, pos);
	}
	else {
		return string();
	}
	return str;
}

vector<string> BaseWsClient::Explode(
	string  theString,
	string  theDelimiter,
	bool    theIncludeEmptyStrings)
{
	//printf("EXPLODE\n");
	//UASSERT( theDelimiter.size(), >, 0 );

	vector<string> theStringVector;
	size_t  start = 0, end = 0, length = 0;

	while (end != string::npos)
	{
		end = theString.find(theDelimiter, start);

		// If at end, use length=maxLength.  Else use length=end-start.
		length = (end == string::npos) ? string::npos : end - start;

		if (theIncludeEmptyStrings
			|| ((length > 0) /* At end, end == length == string::npos */
				&& (start < theString.size())))
			theStringVector.push_back(theString.substr(start, length));

		// If at end, use start=maxSize.  Else use start=end+delimiter.
		start = ((end > (string::npos - theDelimiter.size()))
			? string::npos : end + theDelimiter.size());
	}
	return theStringVector;
}

void BaseWsClient::OnError(const char *errInfo)
{
	printf(errInfo);
}

WsFrameRet BaseWsClient::GetFrame(uint8_t* in_buffer, size_t in_length, size_t &frameLen)
{
	//printf("getTextFrame()\n");
	if (in_length < 3) return WsFrameRet::INCOMPLETE_LEN;

	WsHead *pHead = (WsHead*)in_buffer;
	//pHead->FIN, pHead->RSV, pHead->OPCODE, pHead->MASK pHead->payload_length7


	// *** message decoding 

	size_t payload_length = 0;
	int pos = 2;

	//printf("IN:"); for(int i=0; i<20; i++) printf("%02x ",buffer[i]); printf("\n");

	if (pHead->payload_length7 <= 125) {
		payload_length = pHead->payload_length7;
	}
	else if (pHead->payload_length7 == 126) { //msglen is 16bit!

		if (in_length < 4) return WsFrameRet::INCOMPLETE_LEN;
		//payload_length = in_buffer[2] + (in_buffer[3]<<8);
		payload_length = (
			(in_buffer[2] << 8) |
			(in_buffer[3])
			);
		pos += 2;
	}
	else if (pHead->payload_length7 == 127) { //msglen is 64bit!
		if (in_length < 10) return WsFrameRet::INCOMPLETE_LEN;
		payload_length = (uint32_t)(
			((uint64_t)(in_buffer[2]) << 56) |
			((uint64_t)(in_buffer[3]) << 48) |
			((uint64_t)(in_buffer[4]) << 40) |
			((uint64_t)(in_buffer[5]) << 32) |
			((uint64_t)(in_buffer[6]) << 24) |
			((uint64_t)(in_buffer[7]) << 16) |
			((uint64_t)(in_buffer[8]) << 8) |
			((uint64_t)(in_buffer[9]))
			);
		pos += 8;
	}

	frameLen = payload_length + pos;
	//printf("PAYLOAD_LEN: %08x\n", payload_length);
	if (in_length < payload_length + pos) {
		return WsFrameRet::INCOMPLETE_FRAME;
	}

	if (pHead->MASK) {
		OnError("message from svr have masked\n");
		return WsFrameRet::ERROR;
	}

	WebSocketFrameType type;
	if (pHead->OPCODE == 0x0)	   type = (pHead->FIN) ? TEXT_FRAME : INCOMPLETE_TEXT_FRAME; // continuation frame ?
	else if (pHead->OPCODE == 0x1) type = (pHead->FIN) ? TEXT_FRAME : INCOMPLETE_TEXT_FRAME;
	else if (pHead->OPCODE == 0x2) type = (pHead->FIN) ? BINARY_FRAME : INCOMPLETE_BINARY_FRAME;
	else if (pHead->OPCODE == 0x9) 
	{
		type = PING_FRAME;
		OnRevPing();
		Send(PONG_FRAME, NULL, 0);
		return WsFrameRet::COMPLITE;
	}
	else if (pHead->OPCODE == 0xA) {
		type = PONG_FRAME;
		OnRevPong();
		return WsFrameRet::COMPLITE;
	}
	else
	{
		return WsFrameRet::ERROR;
	}
	OnRevMsg(type, (in_buffer + pos), payload_length);
	return WsFrameRet::COMPLITE;
}



void BaseWsClient::Send(uint8_t opcode, const unsigned char* msg, size_t msg_len)
{
	//WsHead head;
	//head.d = 0;
	//head.OPCODE |= 

	unsigned char buffer[1024];
	int pos = 0;
	int size = msg_len;

	buffer[pos++] = (unsigned char)opcode; // text frame

	if (size <= 125) {
		buffer[pos++] = size;
	}
	else if (size <= 65535) {
		buffer[pos++] = 126; //16 bit length follows

		buffer[pos++] = (size >> 8) & 0xFF; // leftmost first
		buffer[pos++] = size & 0xFF;
	}
	else { // >2^16-1 (65535)
		buffer[pos++] = 127; //64 bit length follows

		// write 8 bytes length (significant first)

		// since msg_length is int it can be no longer than 4 bytes = 2^32-1
		// padd zeroes for the first 4 bytes
		for (int i = 3; i >= 0; i--) {
			buffer[pos++] = 0;
		}
		// write the actual 32bit msg_length in the next 4 bytes
		for (int i = 3; i >= 0; i--) {
			buffer[pos++] = ((size >> 8 * i) & 0xFF);
		}
	}
	buffer[1] |= 0x80; //set MASK
	//Masking-key 32 bit
			// Create mask
	std::array<unsigned char, 4> mask;
	std::uniform_int_distribution<unsigned short> dist(0, 255);
	std::random_device rd;
	for (std::size_t c = 0; c < 4; c++)
		mask[c] = static_cast<unsigned char>(dist(rd));

	for (std::size_t c = 0; c < 4; c++)
		buffer[pos++] = mask[c];
	//printf("OnSendBuf pos=%d\n", pos);
	OnSendBuf(buffer, pos);

	pos = 0;
	for (std::size_t c = 0; c < msg_len; c++)
	{
		buffer[pos++] = msg[c] ^ mask[c % 4];
		if (pos == sizeof(buffer))
		{
			OnSendBuf(buffer, pos);
			pos = 0;
		}
	}
	OnSendBuf(buffer, pos);
	//printf("OnSendBuf msg_length=%ld\n", msg_len);
}