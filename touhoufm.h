#ifndef TOUHOUFM_H
#define TOUHOUFM_H

#include <QFrame>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QDomDocument>
#include <QSystemTrayIcon>
#include <QAudioProbe>
#include <QBitmap>

class QSettings;
class QWebSocket;

struct RadioInfo
{
    QString name;
    QUrl    stream, info;
};

class TouHouFM : public QFrame
{
    Q_OBJECT

    int downloadId, retryId;

    QMediaPlayer *player;

    QUrl infoUrl;

    QNetworkAccessManager *network;

    QDomDocument meta_info;

    QMap<QString, QVariant> meta;
    QMap<QAction*, RadioInfo> radio_data;

    QSettings *settings;

    QMenu *m_menu,*m_radios;

    QSystemTrayIcon *m_systray;

    qreal progress, progress_auto, speed;
    QTime m_last;

    QPixmap m_pBackground;
    QPixmap m_pStop,m_pPlay;
    QBitmap m_bMask;

    QWebSocket *m_wsInfo;

    QString m_sInfo;

    QFont m_f;

    qreal m_rProgress;

    QMediaPlayer::State m_status;

    QRectF m_statusTextSize;

public:
    explicit TouHouFM(QWidget *parent = 0);
    ~TouHouFM();

private slots:
    void stateChanged(QMediaPlayer::State state);
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    void metaDataChanged(QString field,QVariant value);
    void replyFinished(QNetworkReply* reply);
    void systrayActivated(QSystemTrayIcon::ActivationReason reason);
    void showRadios();
    void sendRequest();
    void handleMessage(QString info);

private:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent * event);
    void timerEvent(QTimerEvent *event);
    void resizeEvent(QResizeEvent *event);
    void paintEvent(QPaintEvent *);
    int m_nMouseClick_X_Coordinate;
    int m_nMouseClick_Y_Coordinate;
    bool m_nMove;

};

#endif // TOUHOUFM_H
