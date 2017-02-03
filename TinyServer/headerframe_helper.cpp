#include "headerframe_helper.h"

namespace TcpserverCore
{
    qint32 TcpHeaderFrameHelper::sizeofHeaderFrame()
    {
        return sizeof(TcpHeaderFrame);
    }
    
    QByteArray TcpHeaderFrameHelper::bindHeaderAndDatagram(const TcpHeaderFrame &header,const QByteArray &realDataBytes)
    {
        QByteArray byteArray, temp;
        temp.resize(4);
        
        unsignedToQByteArray(static_cast<quint32>(header.messageType), temp);
        byteArray += temp;
        
        unsignedToQByteArray(static_cast<quint32>(header.featureCode), temp);
        byteArray += temp;
        
        unsignedToQByteArray(static_cast<quint32>(header.sourceFeatureCode), temp);
        byteArray += temp;
        
        unsignedToQByteArray(static_cast<quint32>(header.messageLength), temp);
        byteArray += temp;
        
        byteArray += realDataBytes;
        return byteArray;
    }
    
    void TcpHeaderFrameHelper::praseHeaderAndDatagram(const QByteArray &dataBytes,TcpHeaderFrame &headerFrame,QByteArray &realDataBytes)
    {
        int headerSize = sizeofHeaderFrame();
        realDataBytes.resize(dataBytes.size() - headerSize);
        praseHeader(dataBytes,headerFrame);
        
        realDataBytes = dataBytes.mid(headerSize, dataBytes.size());
    }
    
    void TcpHeaderFrameHelper::praseHeader(const QByteArray &datagram, TcpHeaderFrame &headerFrame)
    {
        int nextStop = 0, previousStop = 0;
        
        nextStop += sizeof(headerFrame.messageType);
        headerFrame.messageType       = static_cast<qint32>(qByteArrayToUnsignedInt(datagram.left(nextStop)));
        
        previousStop = nextStop;
        nextStop += sizeof(headerFrame.featureCode);
        headerFrame.featureCode       = static_cast<qint32>(qByteArrayToUnsignedInt(datagram.mid(previousStop,nextStop)));
        
        previousStop = nextStop;
        nextStop += sizeof(headerFrame.sourceFeatureCode);
        headerFrame.sourceFeatureCode = static_cast<qint32>(qByteArrayToUnsignedInt(datagram.mid(previousStop,nextStop)));
        
        previousStop = nextStop;
        nextStop += sizeof(headerFrame.messageLength);
        headerFrame.messageLength     = static_cast<qint32>(qByteArrayToUnsignedInt(datagram.mid(previousStop,nextStop)));
    }
    
    quint32 TcpHeaderFrameHelper::qByteArrayToUnsignedInt(QByteArray bytes)
    {
        quint32 result = 0;
        result |= ((static_cast<quint32>(bytes[0]) << 0)  & 0x000000ff); 
        result |= ((static_cast<quint32>(bytes[1]) << 8)  & 0x0000ff00); 
        result |= ((static_cast<quint32>(bytes[2]) << 16) & 0x00ff0000); 
        result |= ((static_cast<quint32>(bytes[3]) << 24) & 0xff000000); 
        return result;
    }
    
    void TcpHeaderFrameHelper::unsignedToQByteArray(quint32 num, QByteArray &bytes)
    {
        bytes.resize(4);
        bytes[0] = static_cast<char>(0x000000ff & num);
        bytes[1] = static_cast<char>((0x0000ff00 & (num)) >> 8);
        bytes[2] = static_cast<char>((0x00ff0000 & (num)) >> 16);
        bytes[3] = static_cast<char>((0xff000000 & (num)) >> 24);
    }
}
