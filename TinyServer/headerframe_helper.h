#ifndef HEADERFRAME_HELPER_H
#define HEADERFRAME_HELPER_H

#include <QByteArray>

namespace TcpserverCore
{
    class TcpHeaderFrameHelper
    {
    public:
        enum class MessageType
        {
            //心跳包检测设施
            PulseFacility = 0,
            //普通报文（非HTML）
            PlainMessage = 2,
            //HTML
            HtmlMessage = 4,
            //声音信息
            VoiceMessage = 8,
            //某设备下线的信息
            DeviceLogOut = 16,
            //某设备上线的信息
            DeviceLogIn = 32,
            //来自服务器的测试
            ServerTest = 64,
            //ACK应答
            ACK = 128,
            //坐标位置
            Coordinate = 256,
            //其他我还没想好的信息
            Other = 512
        };
        
        struct TcpHeaderFrame
        {
            //用于标识报文的类型
            int messageType;
            //用于标识发送者的特征码，用于校验
            qint32 featureCode;
            //信息来源对应特征码
			int sourceFeatureCode;
            //用于标识报文的长度，用于校验
            int messageLength;
        };
        
        static QByteArray bindHeaderAndDatagram(const TcpHeaderFrame &header,const QByteArray &realDataBytes);
        static void praseHeaderAndDatagram(const QByteArray &dataBytes,TcpHeaderFrame &headerFrame,QByteArray &realDataBytes);
        
        static unsigned qByteArrayToInt(QByteArray bytes);
        static void unsignedToQByteArray(unsigned num, QByteArray &bytes);
    };
    
}


#endif // HEADERFRAME_HELPER_H
