// Tangent Panels Plugin for Unreal Editor
// Copyright 2024 Tangent Wave Ltd.
// SVN: $Revision: 381 $

#include "RxContext.h"

// these bitwise flags are used to register a cached request for values to optimise the process
const uint32 RxContext::LocationRequest = 1;
const uint32 RxContext::RotationRequest = 2;
const uint32 RxContext::ScaleRequest = 4;

// bitwise flags for things that refer to x/y/z axes
const uint32 RxContext::AxisX = 1;
const uint32 RxContext::AxisY = 2;
const uint32 RxContext::AxisZ = 4;

// default constructor
RxContext::RxContext()
{
    // sensible defaults
    ResetForHeader();
    ResetCaches();
}

// resets the context instance to be ready to receive a message header section that defines the length of the following message payload
void RxContext::ResetForHeader()
{
    // set the flag to say we are now expecting for a 4 byte header with 4 bytes remaining to be read
    _rxHeader = true;
    _bufferLen = IPCComms::HeaderLen;
    _bytesRemaining = IPCComms::HeaderLen;
    _readIndex = 0;
}

// resets the context instance to be ready to receive a message payload section of a sepcified length
void RxContext::ResetForMessage(uint32 messsageLen)
{
    // clear the header flag to say we are waiting for the message payload, set the expected byte count with that many bytes outstanding
    _rxHeader = false;
    _bufferLen = messsageLen;
    _bytesRemaining = messsageLen;
    _readIndex = 0;
}

// resets all caches used to accumulate control value changes
void RxContext::ResetCaches()
{
    // zero all the cache stores
    MoveDeltaCache.Set(0, 0, 0);
    RotateDeltaCache.Roll = 0;
    RotateDeltaCache.Pitch = 0;
    RotateDeltaCache.Yaw = 0;
    ScaleDeltaCache.Set(0, 0, 0);
    MoveResetCache = 0;
    RotateResetCache = 0;
    ScaleResetCache = 0;
    _valueRequestCache = 0;
}

// sets the flag for the specified value request type
void RxContext::FlagValueRequest(uint32 request)
{
    // simply set the bitwise flag in the cache
    _valueRequestCache |= request;
}

// returns true if we have registered a value request for the data associated with the flag request type
bool RxContext::IsValueRequested(uint32 request)
{
    // simple bit test
    return (_valueRequestCache & request);
}

// sets the flag for the specified reset axis
void RxContext::FlagMoveReset(uint32 axis)
{
    // simply set the bitwise flag in the cache
    MoveResetCache |= axis;
}

// sets the flag for the specified reset axis
void RxContext::FlagRotateReset(uint32 axis)
{
    // simply set the bitwise flag in the cache
    RotateResetCache |= axis;
}

// sets the flag for the specified reset axis
void RxContext::FlagScaleReset(uint32 axis)
{
    // simply set the bitwise flag in the cache
    ScaleResetCache |= axis;
}

// returns true when the buffer has at least one more command to be read
bool RxContext::HasCommandToRead()
{
    // a command is 4 bytes, so check if have at least that much data
    return ((_bufferLen - _readIndex) >= 4);
}

// returns true if we are currently processing header data, if false we are working on the payload
bool RxContext::ProcessingHeader()
{
    // return the state flag
    return _rxHeader;
}

// returns true if we are expecting data to be received to complete a message
bool RxContext::WaitingForData()
{
    // if there are bytes marked as remaining then we are waiting for more data
    return (_bytesRemaining > 0);
}

// returns the current status of the buffer to receive data, it returns the point in the buffer to receive into and how many bytes we are expecting
void RxContext::GetReceiveBuffer(uint8*& buffer, int32& bytesExpected)
{
    // pass back the requested info, the base buffer pointer is adjusted by how many bytes we still need
    buffer = _buffer + _bufferLen - _bytesRemaining;
    bytesExpected = _bytesRemaining;
}

// called after some more data has been received from an external source to adjust the bytes remaining status
void RxContext::AdjustForDataReceived(uint32 bytesReceived)
{
    // tweak the remaining count bu how many bytes have just been received
    _bytesRemaining -= bytesReceived;
}

// returns the next 4 byte unsigned int value read from the buffer
uint32 RxContext::ReadUInt32()
{
    uint32 value = 0;

    // we don't deal with underruns here as the messages from the hub are really very much set in stone, so just protect against memory violations
    if ((_readIndex + 4) <= _bufferLen)
    {
        // read out and shift the required 4 bytes in msb order
        value = _buffer[_readIndex++] << 24;
        value |= _buffer[_readIndex++] << 16;
        value |= _buffer[_readIndex++] << 8;
        value |= _buffer[_readIndex++];
    }

    // return the result
    return value;
}

// returns the next 4 byte signed int value read from the buffer
uint32 RxContext::ReadInt32()
{
    int32 value = 0;

    // we don't deal with underruns here as the messages from the hub are really very much set in stone, so just protect against memory violations
    if ((_readIndex + 4) <= _bufferLen)
    {
        // read out and shift the required 4 bytes in msb order
        value = _buffer[_readIndex++] << 24;
        value |= _buffer[_readIndex++] << 16;
        value |= _buffer[_readIndex++] << 8;
        value |= _buffer[_readIndex++];
    }

    // return the result
    return value;
}

// returns the next 4 byte float value read from the buffer
float RxContext::ReadFloat32()
{
    float value = 0;

    // we don't deal with underruns here as the messages from the hub are really very much set in stone, so just protect against memory violations
    if ((_readIndex + 4) <= _bufferLen)
    {
        volatile uint32 iValue;
        volatile float *fValuePtr;

        // a way to pack/upack the bytes of the float value via the uint reader for the raw bits
        iValue = ReadUInt32();
        fValuePtr = (volatile float *) &iValue;
        value = *fValuePtr;
    }

    // return the result
    return value;
}

// returns a string read from the buffer as an fname instance
FName RxContext::ReadString()
{
    // strings are prefixed by their length, use the sister method to read it
    int32 len = ReadUInt32();

    // if we have a valid length then proceed, protecting against memory overruns
    if ((len > 0) && ((_readIndex + len) <= _bufferLen))
    {
        // prepare the required pointer from the buffer and current index before adjusting it to consume the text
        ANSICHAR* ptr = (ANSICHAR*) (_buffer + _readIndex);
        _readIndex += len;

        // return an fname created from this data, if the string has been used before the fname class will efficiently reuse it
        return FName((int32) len, ptr);
    }

    // return an empty fname instance as we have nothing to read
    return FName();
}

// can be used to skip the set number of bytes in messages where the data is not needed locally but we want to continue reading from the buffer
void RxContext::Skip(uint32 bytesToSkip)
{
    // skip the number of bytes, if this takes us over our buffer length then clip to that to maintain some sort of data consistency
    _readIndex += bytesToSkip;
    if (_readIndex > _bufferLen)
    {
        // this way the index maximum is always a sensible value when at the end of the buffer data
        _readIndex = _bufferLen;
    }
}
