#include "headerframe_helper.h"

namespace TcpserverCore
{
    QByteArray TcpHeaderFrameHelper::bindHeaderAndDatagram(const TcpHeaderFrame &header,const QByteArray &realDataBytes)
    {
        QByteArray byteArray, temp;
        temp.resize(4);
        
        unsignedToQByteArray((unsigned)header.messageType, temp);
        byteArray += temp;
        
        unsignedToQByteArray((unsigned)header.featureCode, temp);
        byteArray += temp;
        
        unsignedToQByteArray((unsigned)header.sourceFeatureCode, temp);
        byteArray += temp;
        
        unsignedToQByteArray((unsigned)header.messageLength, temp);
        byteArray += temp;
        
        byteArray +=realDataBytes;
        return byteArray;
    }
    
    void TcpHeaderFrameHelper::praseHeaderAndDatagram(const QByteArray &dataBytes,TcpHeaderFrame &headerFrame,QByteArray &realDataBytes)
    {
        int size = sizeof(headerFrame);
        int nextStop = 0, previousStop = 0;
        
        realDataBytes.resize(dataBytes.size() - size);
        
        nextStop += sizeof(headerFrame.messageType);
        headerFrame.messageType = qByteArrayToInt(dataBytes.left(nextStop));
        
        previousStop = nextStop;
        nextStop += sizeof(headerFrame.featureCode);
        headerFrame.featureCode = qByteArrayToInt(dataBytes.mid(previousStop,nextStop));
        
        previousStop = nextStop;
        nextStop += sizeof(headerFrame.sourceFeatureCode);
        headerFrame.sourceFeatureCode = qByteArrayToInt(dataBytes.mid(previousStop,nextStop));
        
        previousStop = nextStop;
        nextStop += sizeof(headerFrame.messageLength);
        headerFrame.messageLength = qByteArrayToInt(dataBytes.mid(previousStop,nextStop));
        
        realDataBytes = dataBytes.mid(size, dataBytes.size());
    }
    
    unsigned TcpHeaderFrameHelper::qByteArrayToInt(QByteArray bytes)
    {
        int result = 0;
        result |= ((bytes[0]) & 0x000000ff); 
        result |= ((bytes[1] << 8) & 0x0000ff00); 
        result |= ((bytes[2] << 16) & 0x00ff0000); 
        result |= ((bytes[3] << 24) & 0xff000000); 
        
        return result;
    }
    
    void TcpHeaderFrameHelper::unsignedToQByteArray(unsigned num, QByteArray &bytes)
    {
        bytes.resize(4);
        bytes[0] = (char)( 0x000000ff & num);
        bytes[1] = (char)((0x0000ff00 & (num)) >> 8);
        bytes[2] = (char)((0x00ff0000 & (num)) >> 16);
        bytes[3] = (char)((0xff000000 & (num)) >> 24);
    }
}
