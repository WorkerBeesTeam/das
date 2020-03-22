#ifndef RECORDER_H
#define RECORDER_H

#include <QThread>
#include <QImage>

class QVideoOutput;
class Recorder : public QObject
{
    Q_OBJECT
public:
    Recorder() : rec(nullptr), m_inWork(false) {}
    bool inWork() const;

public slots:
    bool recording() const;
    void proccessFrame(QImage* frame);
    bool open(QSize resolution,
              int frameRate,
              const QString & filename);
    void close();
private:
    QVideoOutput* rec;
    bool m_inWork;
};

class RecorderThread : public QThread
{
    Q_OBJECT
public:
    RecorderThread();
    Recorder* recorder() { return m_recorder; }
signals:
    void frame(QImage*);
public slots:
    void addFrame(const QImage &frame);
private:
    void run();
    Recorder* m_recorder;
};

#endif // RECORDER_H
