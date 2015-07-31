#ifndef TOUHOUFM_H
#define TOUHOUFM_H

#include <QFrame>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QContextMenuEvent>
#include <QMenu>
#include <QNetworkAccessManager>
#include <QDomDocument>
#include <QSystemTrayIcon>
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

    // Timer id's
    int downloadId;
    int retryId;
    int animationId;

    // QtAV objects
    QMediaPlayer *play;

    // Settings object to store permanent options
    QSettings *settings;

    // Menu objects
    QMenu *m_menu,*m_radios;

    // Systemtray icon
    QSystemTrayIcon *m_systray;

    // Progress state
    qreal progress;

    // Last check time
    QTime m_last;

    // Skin pixmaps
    QMap<QString,QRectF> m_areas;
    QMap<QString,QPixmap> m_pixmaps;

    // Rate bar
    QRect m_rRatebar;
    QSize m_sRateStar;

    // window shape
    QBitmap m_bMask;

    // Information channel
    QWebSocket *m_wsInfo;

    // Status texts
    QString m_sInfo;
    QString m_sStatus;
    QString m_sTime;
    QVariantMap meta;


    // Auth token
    QString m_sAuth;

    // Font
    QFont m_f,m_f2;

    // progress state
    qreal m_rProgress;

    // Radios
    QMap<QAction*, RadioInfo> radio_data;

    // Rating options
    QStringList m_slRate;

    // Report "what is wrong" options
    QStringList m_slWhats;

    // Rating
    int m_iRating;
    qreal m_fAverageRating,m_fGlobalRating;

public:
    explicit TouHouFM(QWidget *parent = 0);
    ~TouHouFM();

private slots:
    // Handles changes in mediastatus
    void mediaStatusChanged(QMediaPlayer::MediaStatus status);
    // Handles updates in meta data
    void metaDataChanged(QString field,QVariant value);
    // handle systemtray interaction
    void systrayActivated(QSystemTrayIcon::ActivationReason reason);
    // Show possible radio stations menu
    void showRadios();
    // send request new songinfo
    void sendRequest();
    // submit user rating
    void sendRating(int rating);
    // handle websocket info message
    void handleMessage(QString info);
    // update volume display
    void volumeChanged(qreal vol);
    // show report dialog and submit
    void report();
    // attempt to login
    void login();

//    void calculateFFT(QByteArray buff);

private:
    // custom mouse handlers
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
    // custom context menu handler
    void contextMenuEvent(QContextMenuEvent * event);
    // custom timer events
    void timerEvent(QTimerEvent *event);
    // custom resize event
    void resizeEvent(QResizeEvent *event);
    // custom drawing
    void paintEvent(QPaintEvent *);
    // last mouseclick pos
    QPoint m_pMouseClick, m_pMouse;
    // mouse press target
    int m_nTarget;

};

#endif // TOUHOUFM_H
