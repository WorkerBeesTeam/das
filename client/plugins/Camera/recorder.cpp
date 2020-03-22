
#include "encoder.h"
#include "recorder.h"

bool Recorder::recording() const
{
    return rec && rec->recording();
}

void Recorder::proccessFrame(QImage *frame)
{
    m_inWork = true;

    rec->newFrame( *frame );
    delete frame;

    m_inWork = false;
}

bool Recorder::open(QSize resolution, int frameRate, const QString &filename)
{
    if (rec)
        close();
    rec = new QVideoOutput;
    return rec->openMediaFile(resolution.width(), resolution.height(), frameRate, filename);
}

void Recorder::close()
{
    if (rec)
    {
        rec->closeMediaFile();
        delete rec; rec = nullptr;
    }
}

bool Recorder::inWork() const
{
    return m_inWork;
}

RecorderThread::RecorderThread() : m_recorder(nullptr) {
    qRegisterMetaType<QImage*>("QImage*");
}

void RecorderThread::addFrame(const QImage& frame)
{
    if (m_recorder && m_recorder->recording())
    {
        if (!m_recorder->inWork())
            QMetaObject::invokeMethod(m_recorder, "proccessFrame", Qt::QueuedConnection,
                                  Q_ARG(QImage*, new QImage( frame )));
    }
}

void RecorderThread::run()
{
    m_recorder = new Recorder;

    exec();

    delete m_recorder;
    m_recorder = nullptr;
}
