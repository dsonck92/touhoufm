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

class QSettings;

namespace Ui {
class TouHouFM;
}

struct RadioInfo
{
    QString name;
    QUrl    stream, info;
};

class TouHouFM : public QFrame
{
    Q_OBJECT

    QMediaPlayer *player;

    QUrl infoUrl;

    QNetworkAccessManager *network;

    QDomDocument meta_info;

    QMap<QString, QVariant> meta;
    QMap<QAction*, RadioInfo> radio_data;

    QSettings *settings;

    QMenu *m_menu,*m_radios;

    QSystemTrayIcon *m_systray;

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

private:
    void mousePressEvent(QMouseEvent *event);
    void mouseMoveEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent * event);
    void timerEvent(QTimerEvent *event);
    int m_nMouseClick_X_Coordinate;
    int m_nMouseClick_Y_Coordinate;
    bool m_nMove;

    Ui::TouHouFM *ui;
};

#endif // TOUHOUFM_H
