#ifndef QVIDEOOUTPUT_H
#define QVIDEOOUTPUT_H

#include <QObject>
#include <QImage>
//#include <QVideoFrame>

class AVStream;
class AVFormatContext;
class AVCodec;
class AVCodec;
class AVOutputFormat;
class SwsContext;
class AVFrame;

class QVideoOutput : public QObject
{
public:
   QVideoOutput(QObject * parent=0);
   virtual ~QVideoOutput();
   bool recording() const { return opened; }
   bool openMediaFile(int width,
                      int height,
                      int frameRate,
                      const QString & filename);
   bool closeMediaFile();
   void setResolution(int width, int height);
public slots:
   bool newFrame(const QImage & image);
//   bool newFrame(const QVideoFrame& f);
protected:
   AVStream * addStream(AVFormatContext *inFormatContext,
                        AVCodec **codec) const;
   bool openVideo(AVCodec *codec, AVStream *st);
   bool writeVideoFrame(int width, int height, int format,
                        const uint8_t* bits, int bytesPerLine,
                        AVFormatContext *inFormatContext,
                        AVStream *st);
   void closeVideo(AVStream *st);

   SwsContext      * swsContext;
   AVFormatContext * formatContext;
   AVOutputFormat  * outputFormat;
   AVStream        * videoStream;
   AVCodec         * videoCodec;
   AVFrame         * frame;
   int swsFlags;
   int streamFrameRate;
   int width;
   int height;
   int frameCount;
   bool opened;
};

#endif // QVIDEOOUTPUT_H
