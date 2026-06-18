// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 377 $

#include "MessageBuilder.h"

// default constructor for a new message
MessageBuilder::MessageBuilder()
{
	// reset ready for values
	Reset();
}

// resets the instance to be ready to have values written to the buffer
void MessageBuilder::Reset()
{
	// we allow a header sized gap at the start of the buffer which is completed when the message is built
	_writeIndex = IPCComms::HeaderLen;

	// init the message length to zero until built
	_messageLen = 0;

	// init to not yet built for sending
	_built = false;
}

// writes the specified 4 byte unsigned int value to the buffer and moves along to the next space if available
void MessageBuilder::WriteUInt32(uint32 value)
{
	// we are pretty controlled in terms of use so we don't do anything more about message length other than make sure we don't overrun the buffer
	if (!_built && ((_writeIndex + 4) <= (IPCComms::HeaderLen + IPCComms::MaxMessageLen)))
	{
		// store the shifted 4 bytes in msb order
		_message[_writeIndex++] = (value >> 24) & 0xFF;
		_message[_writeIndex++] = (value >> 16) & 0xFF;
		_message[_writeIndex++] = (value >> 8) & 0xFF;
		_message[_writeIndex++] = value & 0xFF;
	}
}

// writes the specified 4 byte signed int value to the buffer and moves along to the next space if available
void MessageBuilder::WriteInt32(int32 value)
{
	// we are pretty controlled in terms of use so we don't do anything more about message length other than make sure we don't overrun the buffer
	if (!_built && ((_writeIndex + 4) <= (IPCComms::HeaderLen + IPCComms::MaxMessageLen)))
	{
		// store the shifted 4 bytes in msb order
		_message[_writeIndex++] = (value >> 24) & 0xFF;
		_message[_writeIndex++] = (value >> 16) & 0xFF;
		_message[_writeIndex++] = (value >> 8) & 0xFF;
		_message[_writeIndex++] = value & 0xFF;
	}
}

// writes the specified 4 byte float value to the buffer and moves along to the next space if available
void MessageBuilder::WriteFloat32(float value)
{
	// guard against writing to a built message
	if (!_built)
	{
		volatile float	fValue = value;
		volatile uint32* iValuePtr = (volatile uint32 *) &fValue;

		// the above is a way to pack/upack the bytes of the float value via the uint writer for the raw bits
		WriteUInt32(*iValuePtr);
	}
}

// writes the given ascii text to the buffer with a 4 byte length prefix
void MessageBuilder::WriteString(const char* string)
{
	// guard against writing to a built message
	if (!_built)
	{
		// safely handle null strings
		int strLen = string ? strlen(string) : 0;

		// we are pretty controlled in terms of use so we don't do anything more about message length other than make sure we don't overrun the buffer
		if ((_writeIndex + 4 + strLen) <= (IPCComms::HeaderLen + IPCComms::MaxMessageLen))
		{
			// start with the length of the text
			WriteUInt32(strLen);
			// we only need to write any more if there is actual content
			if (strLen > 0)
			{
				// copy the raw data to the next spot in the buffer and adjust the write position accordingly
				memcpy(_message + _writeIndex, string, strLen);
				_writeIndex += strLen;
			}
		}
	}
}

// writes the given fstring referenced text to the buffer with a 4 byte length prefix
void MessageBuilder::WriteString(const FString& string)
{
	// use the sister function converting the fstring to ansi
	WriteString(TCHAR_TO_ANSI(*string));
}

// writes the given fname text to the buffer with a 4 byte length prefix
void MessageBuilder::WriteString(const FName string)
{
	// use the sister function converting the fname to fstring
	WriteString(string.ToString());
}

// called when the message has been completed, this finalises the message ready to be sent by populating the message header, it sets
// the message length field and invalidates the write index so no more values may be written until the message instance is reset 
void MessageBuilder::Build()
{
	// guard against building a message twice
	if (!_built)
	{
		// we get the length of the buffer content less the header bytes to set as the message payload length (the length never includes the header bytes themselves)
		uint32 payloadLen = _writeIndex - IPCComms::HeaderLen;

		// store the full buffer size as the actual message length (including the header) as this is what we will to send to the hub
		_messageLen = _writeIndex;

		// we use the uint writer to insert the header value at the start of the buffer so reset the write index
		_writeIndex = 0;
		WriteUInt32(payloadLen);

		// flag as a valid built message
		_built = true;
	}
}

// called to get the results of building a message, returns true only if a call to build the message has been made and the returned data is valid to send
bool MessageBuilder::GetMessage(uint8* &msg, int32 &msgLen)
{
	// populate the message details only if built
	if (_built)
	{
		// a message is ready to go, pass back the details
		msg = _message;
		msgLen = _messageLen;
	}

	// return the built status, if true the above values may be used to send
	return _built;
}
