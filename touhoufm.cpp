#include "touhoufm.h"

#include <QAudioDeviceInfo>
#include <QMessageBox>
#include <QJsonDocument>

#include <QMenu>
#include <QWebSocket>

#include <QNetworkReply>
#include <QSettings>
#include <QTimerEvent>

#include <QPainter>

#include <cmath>

enum ClientMessages {
    NopMessage         = 0,
    RequestInfoMessage = 1,
    InfoMessage           ,
    ProgressMessage
};


TouHouFM::TouHouFM(QWidget *parent) :
    QFrame(parent)
{
    m_last = QTime::currentTime();

    m_pBackground = QPixmap(":/images/background.png");
    m_pStop = QPixmap(":/images/stop.png");
    m_pPlay = QPixmap(":/images/play.png");

    m_bMask = m_pBackground.mask();

    setMinimumSize(m_pBackground.size());
    setMaximumSize(m_pBackground.size());
    //    ui->setupUi(this);

    retryId = -1;

    m_menu = new QMenu;

    m_nMove = false;

    player = new QMediaPlayer(this);


    //    connect(player,SIGNAL(volumeChanged(int)),ui->sliderVolume,SLOT(setValue(int)));
    //    connect(player,SIGNAL(mutedChanged(bool)),ui->pushMute,SLOT(setChecked(bool)));

    connect(player,SIGNAL(stateChanged(QMediaPlayer::State)),SLOT(stateChanged(QMediaPlayer::State)));
    connect(player,SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));

    //    connect(player,SIGNAL(bufferStatusChanged(int)),ui->progress,SLOT(setValue(int)));
    connect(player,SIGNAL(metaDataChanged(QString,QVariant)),SLOT(metaDataChanged(QString,QVariant)));

    m_menu->addAction("Play",player,SLOT(play()));
    m_menu->addAction("Stop",player,SLOT(stop()));
    m_menu->addSeparator();
    m_radios = m_menu->addMenu("Radios");
    m_menu->addSeparator();
    m_menu->addAction("Show",this,SLOT(show()));
    m_menu->addAction("Minimize",this,SLOT(showMinimized()));
    m_menu->addAction("Hide",this,SLOT(hide()));
    m_menu->addSeparator();
    m_menu->addAction("Quit",this,SLOT(close()));

    setWindowFlags(Qt::FramelessWindowHint|Qt::WindowTitleHint);

    network = new QNetworkAccessManager(this);

    connect(network, SIGNAL(finished(QNetworkReply*)),
            this, SLOT(replyFinished(QNetworkReply*)));

    downloadId = startTimer(5000);
    startTimer(1000);

    network->get(QNetworkRequest(QUrl("http://en.touhou.fm/radios.xml")));

    settings = new QSettings(this);

    player->setMedia(QMediaContent(settings->value("url",QUrl("http://en.touhou.fm:8010/touhou.mp3")).toUrl()));
    infoUrl = settings->value("info",QUrl("http://en.touhou.fm/wp-content/plugins/touhou.fm-plugin/xml.php")).toUrl();

    if(settings->value("state","stopped") == "playing")
    {
        player->play();
    }
    if(settings->value("shown",true).toBool())
    {
        show();
    }
    else
    {
        hide();
    }

    m_systray = new QSystemTrayIcon(this);

    m_systray->setContextMenu(m_menu);
    m_systray->setIcon(QIcon(":/images/icon.png"));
    m_systray->show();

    connect(m_systray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),SLOT(systrayActivated(QSystemTrayIcon::ActivationReason)));

    m_wsInfo = new QWebSocket();

    connect(m_wsInfo,SIGNAL(connected()),SLOT(sendRequest()));
    connect(m_wsInfo,SIGNAL(textMessageReceived(QString)),SLOT(handleMessage(QString)));

    m_wsInfo->open(QUrl("ws://en.touhou.fm/wsapp/"));

    m_f = font();
    m_f.setPixelSize(24);
}

TouHouFM::~TouHouFM()
{
    //   settings->setValue("volume",ui->sliderVolume->value());
    //   settings->setValue("shown",isVisible());

    //    delete ui;
    m_wsInfo->deleteLater();
}

void TouHouFM::stateChanged(QMediaPlayer::State state)
{
    switch(state)
    {
    case QMediaPlayer::StoppedState:
        settings->setValue("state","stopped");
        break;
    case QMediaPlayer::PlayingState:
        settings->setValue("state","playing");
        break;
    default:
        break;
    }
    m_status = state;
    update();
}

void TouHouFM::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch(status)
    {
    case QMediaPlayer::UnknownMediaStatus:
        m_sInfo = "Unknown";
        //        ui->labelInfo->setText("Unknown");
        break;
    case QMediaPlayer::NoMedia:
        m_sInfo = "No Media!";
        break;
    case QMediaPlayer::LoadingMedia:
        m_sInfo = "Loading ...";
        break;
    case QMediaPlayer::LoadedMedia:
        m_sInfo = "Loaded!";
        break;
    case QMediaPlayer::StalledMedia:
        m_sInfo = "Stalled!";
        break;
    case QMediaPlayer::BufferingMedia:
        m_sInfo = "Buffering...";
        break;
    case QMediaPlayer::BufferedMedia:
        m_sInfo = "Buffered";
        break;
    case QMediaPlayer::EndOfMedia:
        m_sInfo = "Done!";
        retryId = startTimer(3000);
        break;
    case QMediaPlayer::InvalidMedia:
        m_sInfo = "Invalid Media!";
        retryId = startTimer(3000);
        break;

    }
    update();
}

void TouHouFM::showRadios()
{
    QAction *act = m_radios->exec(mapToGlobal(QPoint(0, 0)));

    qDebug() << "Selected: " << act;

    if(act && radio_data.contains(act))
    {
        RadioInfo info = radio_data.value(act);


        qDebug() << "Playing: " << info.stream.toString();

        player->setMedia(info.stream);
        infoUrl = info.info;

        //        ui->title->setText(QString("TouHou.FM Player - %1").arg(info.name));

        settings->setValue("url",info.stream);
        settings->setValue("info",info.info);

        player->play();

    }
}

void TouHouFM::replyFinished(QNetworkReply *reply)
{
    meta_info.setContent(reply);

    QDomElement root = meta_info.documentElement();

    if(root.tagName() == "data")
    {
        QDomElement child = root.firstChildElement();
        while (!child.isNull()) {
            if(child.tagName() == "progress")
            {
                speed = (child.text().toFloat() - progress)/qreal(m_last.secsTo(QTime::currentTime()));
                m_last = QTime::currentTime();
                progress = child.text().toFloat();
                progress_auto = child.text().toFloat();
            }
            else
            {
                metaDataChanged(child.tagName(),child.text());
            }

            child = child.nextSiblingElement();
        }
        network->get(QNetworkRequest(infoUrl));
    }
    if(root.tagName() == "radios")
    {
        QDomElement radio = root.firstChildElement();

        RadioInfo ri;

        while (!radio.isNull()) {
            QDomElement info = radio.firstChildElement();
            while (!info.isNull()) {
                if(info.tagName() == "name")
                {
                    ri.name = info.text();
                }
                if(info.tagName() == "url")
                {
                    ri.stream = info.text();
                }
                if(info.tagName() == "info")
                {
                    ri.info = info.text();
                }
                info = info.nextSiblingElement();
            }

            radio_data[m_radios->addAction(ri.name)] = ri;
            radio = radio.nextSiblingElement();
        }
    }

}

void TouHouFM::mousePressEvent(QMouseEvent *event) {
    m_nMouseClick_X_Coordinate = event->x();
    m_nMouseClick_Y_Coordinate = event->y();

    if(QRectF(QPointF(32,156),QSize(64,64)).contains(event->pos()))
    {
        if(m_status == QMediaPlayer::PlayingState)
        {
            player->stop();
            killTimer(retryId);
        }
        else
        {
            player->play();
        }
    }

    m_nMove = true;
}

void TouHouFM::mouseMoveEvent(QMouseEvent *event) {
    if(m_nMove)
        move(event->globalX()-m_nMouseClick_X_Coordinate,event->globalY()-m_nMouseClick_Y_Coordinate);
}

void TouHouFM::mouseReleaseEvent(QMouseEvent *event)
{
    m_nMove = false;
}

void TouHouFM::metaDataChanged(QString field, QVariant value)
{
    meta[field] = value;

    QString status = QString("%1 by %2 from %3 << %4 >>").arg(meta.value("Title",QString("~")).toString())
            .arg(meta.value("Artist",QString("~")).toString())
            .arg(meta.value("Album",QString("~")).toString())
            .arg(meta.value("AlbumArtist",QString("~")).toString());

    if(status!=m_sInfo)
    {
        m_sInfo = status;
        update();
        //        ui->labelInfo->setText(status);
        m_systray->showMessage("Now Playing:",QString("Title: %1\nArtist: %2\nAlbum: %3\nCircle: %4").arg(meta.value("Title",QString("~")).toString())
                               .arg(meta.value("Artist",QString("~")).toString())
                               .arg(meta.value("Album",QString("~")).toString())
                               .arg(meta.value("AlbumArtist",QString("~")).toString()));
    }

    if(field == "progress")
    {

        //        ui->progress->setValue(value.toFloat());
    }
}

void TouHouFM::systrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        if(isHidden())
        {
            show();
        }
        else
        {
            hide();
        }
    }
}

void TouHouFM::contextMenuEvent(QContextMenuEvent *event)
{
    QAction *act = m_menu->exec(event->globalPos());

    if(act && radio_data.contains(act))
    {
        RadioInfo info = radio_data.value(act);

        player->setMedia(info.stream);
        infoUrl = info.info;

        //        ui->title->setText(QString("TouHou.FM Player - %1").arg(info.name));

        settings->setValue("url",info.stream);
        settings->setValue("info",info.info);

        player->play();

    }
}

void TouHouFM::timerEvent(QTimerEvent *event)
{

    if(event->timerId() == downloadId)
    {

    }
    else if(event->timerId() == retryId)
    {
        player->play();
        killTimer(retryId);
        //        network->get(QNetworkRequest(infoUrl));
    }
    else
    {
        progress_auto += speed;
        metaDataChanged("progress",progress_auto);

    }
}

void TouHouFM::resizeEvent(QResizeEvent *event)
{
    setMask(m_bMask);
}

void TouHouFM::paintEvent(QPaintEvent *)
{
    QPainter p(this);

    p.drawPixmap(0,0,m_pBackground);

    p.drawPixmap(32,156,m_status == QMediaPlayer::PlayingState?m_pStop:m_pPlay);

    p.setPen(Qt::black);
    p.setFont(m_f);

    QFontMetrics m(m_f);
    m_statusTextSize.setSize(m.boundingRect(m_sInfo+" | ").size());

    QRectF renderRect(QPointF(28+1,380+1),QPointF(380-1,412-1));

    m_statusTextSize.setY(renderRect.y());
    if(m_statusTextSize.width() > renderRect.width())
    {
        m_statusTextSize.moveLeft(-fmod(m_rProgress,10.0)/10.0*m_statusTextSize.width()+renderRect.x());
    }
    else
    {
        m_statusTextSize.moveLeft(renderRect.x());
    }

    p.setClipRect(renderRect);

    if(m_statusTextSize.width() > renderRect.width())
    {
        p.drawText(m_statusTextSize,m_sInfo+" | ");
        m_statusTextSize.moveLeft(-fmod(m_rProgress,10.0)/10.0*m_statusTextSize.width()+renderRect.x()+m_statusTextSize.width());
        p.drawText(m_statusTextSize,m_sInfo+" | ");
    }
    else
    {
        p.drawText(m_statusTextSize,m_sInfo);
    }
    p.setPen(QPen(Qt::red,2));
    p.drawLine(QPointF(28+1,412-3),QPointF(28+1+(380-1-1-28)*m_rProgress/100.0,412-3));
}

void TouHouFM::sendRequest()
{
    qDebug() << "Sending request message";
    QVariantMap msg;

    msg.insert("type",RequestInfoMessage);

    m_wsInfo->sendTextMessage(QJsonDocument::fromVariant(msg).toJson());
}

void TouHouFM::handleMessage(QString info)
{
    QVariantMap msg = QJsonDocument::fromJson(info.toUtf8()).toVariant().toMap();

    switch(msg.value("type").toInt())
    {
    case InfoMessage:
    {
        QVariantMap tags = msg.value("tags").toMap();
        foreach(QString key, tags.keys())
        {
            metaDataChanged(key,tags.value(key));
        }
    }
        break;
    case ProgressMessage:
    {
        m_rProgress = msg.value("progress").toFloat();
        update();
    }
    }
}
