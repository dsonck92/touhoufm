#ifndef TOUHOUFMSOCKET_H
#define TOUHOUFMSOCKET_H

#include <QObject>
#include <QWebSocket>

class TouhouFMSocket : public QObject
{
    Q_OBJECT

    // Information channel
    QWebSocket *m_wsInfo;

    // Auth token
    QString m_sAuth;

    QStringList m_perms;

public:
    explicit TouhouFMSocket(QString authToken = QString(), QObject *parent = 0);

    bool open();

    void setAuthToken(QString authToken);

    void login();

    QStringList permissions();

signals:
    void newApprovedAuthToken(QString token);
    void grantingRequired(QUrl grantUrl);

    void newUserRating(int rating);
    void newGlobalRating(qreal rating);

public slots:
    void skipSong();
    void rateSong(int rating);
    void
};

#endif // TOUHOUFMSOCKET_H
