#include <QMainWindow>
#include <QSettings>
#include <QFile>

class QStatusBar;
class QLabel;
class QProgressBar;
class QWebSocket;

enum state {
    IDLE,
    START,
    STREAM,
    FLUSH
};

class MyMainWindow: public QMainWindow
{
public:
    MyMainWindow();
    void setup();
public slots:
    void upload();
    void selectFile();

    void onTextFrameReceived(const QString &frame, bool isLastFrame);
    void onBinaryFrameReceived(const QByteArray &frame, bool isLastFrame);
    void onConnected();
    void onDisconnected();
    void onBytesWritten(quint64);
private:

    void updatePercent(int p);
    void updateAction(const QString &level, const QString&message);
    void updatePhase(const QString &phase);

    QLineEdit *m_url;
    QLineEdit *m_filename;
    QLabel *m_phase;
    QLabel *m_action;
    QProgressBar *m_progress;
    QStatusBar *m_status;
    QSettings m_settings;
    QWebSocket *m_socket;
    QFile *m_file;
    enum state m_state;
};
