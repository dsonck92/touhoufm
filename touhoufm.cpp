#include "report.h"
#include "touhoufm.h"

#include <QMessageBox>

#include <QMenu>
#include <QWebSocket>

#include <QNetworkReply>
#include <QSettings>
#include <QTimerEvent>

#include <QPainter>
#include <QApplication>
#include <QClipboard>
#include <QInputDialog>

#include <QSvgRenderer>
#include <QDesktopServices>
#include <QDir>

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
    // Create an settings object to store persistent settings
    settings = new QSettings(this);

    // Initialize the rating to "unrated"
    m_iRating = -1;

    // Enable mouse tracking for hover effects
    setMouseTracking(true);

    // Create a socket to talk to TouHouFM
    m_sockInfo = new TouhouFMSocket(settings->value("authtoken").toString(),this);

    // Connect the necessary signals to handle TouHouFM info
    connect(m_sockInfo,SIGNAL(newApprovedAuthToken(QString)),SLOT(storeAuthToken(QString)));
    connect(m_sockInfo,SIGNAL(grantingRequired(QUrl)),SLOT(showUrl(QUrl)));
    connect(m_sockInfo,SIGNAL(newNotification(QString,QString)),SLOT(showNotification(QString,QString)));
    connect(m_sockInfo,SIGNAL(metaDataChanged(QString,QVariant)),SLOT(metaDataChanged(QString,QVariant)));
    connect(m_sockInfo,SIGNAL(newProgress(qreal)),SLOT(newProgress(qreal)));
    connect(m_sockInfo,SIGNAL(newTime(QString)),SLOT(newTime(QString)));
    connect(m_sockInfo,SIGNAL(newUserRating(int)),SLOT(newRating(int)));
    connect(m_sockInfo,SIGNAL(newGlobalRating(qreal)),SLOT(newGlobalRating(qreal)));

    // Initialize the ratings options TODO: Should be handled through TouHouFM to sync between client and site
    m_slRate << tr("Awful") << tr("Terrible") << tr("Bad") << tr("Neutral") << tr("Good") << tr("Nice") << tr("Awesome");

    // Last status message time set to now
    m_last = QTime::currentTime();

    // Load the current skin
    loadSkin(settings->value("skin",QString(":/skins/reimu.svg")).toString());
    //    ui->setupUi(this);

    retryId = -1;

    // Mouse target variable
    m_nTarget = 0;

    // Initialize mediaplayer
    play = new QMediaPlayer(this);

    // Connect the status signal to notify about media states and errors
    connect(play,SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));

    // Create context menu
    m_menu = new QMenu;

    m_menu->addAction(tr("Play"),play,SLOT(play()));
    m_menu->addAction(tr("Stop"),play,SLOT(stop()));
    m_menu->addSeparator();
    m_menu->addAction(tr("Report"),this,SLOT(report()));
    m_menu->addAction(tr("Skip"),m_sockInfo,SLOT(skipSong()));
    m_menu->addSeparator();
    m_menu->addAction(tr("Login"),this,SLOT(login()));
    m_menu->addSeparator();
    QMenu * smenu = m_menu->addMenu(tr("Skin"));
    m_menu->addSeparator();
    m_menu->addAction(tr("Show"),this,SLOT(show()));
    m_menu->addAction(tr("Minimize"),this,SLOT(showMinimized()));
    m_menu->addAction(tr("Hide"),this,SLOT(hide()));
    m_menu->addSeparator();
    m_menu->addAction(tr("Quit"),this,SLOT(close()));

    // List all available skins
    QDir d(":/skins/");

    QStringList skins = d.entryList(QStringList() << "*.svg");

    foreach(QString skin, skins)
    {
        m_skinAssoc[smenu->addAction(skin)] = ":/skins/"+skin;
    }

    smenu->addSeparator();

    d.setPath("skins/");

    skins = d.entryList(QStringList() << "*.svg");

    foreach(QString skin, skins)
    {
        m_skinAssoc[smenu->addAction(skin)] = "skins/"+skin;
    }

    // Set the preferred window type, frameless
    setWindowFlags(Qt::Window | Qt::FramelessWindowHint);

    // Do a default 1fps refresh
    startTimer(1000);

    // Load the default url TODO: handle this by querying TouHouFM about actual stream urls
    play->setMedia(QMediaContent(QUrl("http://en.touhou.fm:8010/touhou.mp3")));

    // If the user kept the state in playing, resume playback on creation
    if(settings->value("state","stopped") == "playing")
    {
        play->play();
    }
    // If the user hid the window before closing, return hidden
    if(settings->value("shown",true).toBool())
    {
        show();
    }
    else
    {
        hide();
    }

    // Go into the systemtray
    m_systray = new QSystemTrayIcon(this);

    m_systray->setContextMenu(m_menu);
    m_systray->setIcon(QIcon(":/images/icon.png"));
    setWindowIcon(QIcon(":/images/icon.png"));
    m_systray->show();

    // Handle various interactions with the systemtray
    connect(m_systray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),SLOT(systrayActivated(QSystemTrayIcon::ActivationReason)));
    connect(smenu,SIGNAL(triggered(QAction*)),SLOT(handleAction(QAction*)));

    // Create a semi transparent window
    setWindowOpacity(1.0);
    QPalette pal = palette();
    pal.setColor(backgroundRole(),QColor(0,0,0,0));

    setPalette(pal);
    setAttribute(Qt::WA_TranslucentBackground,true);
    setAttribute(Qt::WA_PaintOnScreen,false);

    // Restore the volume of last session
    play->setVolume(settings->value("volume",play->volume()).toInt());

    // Create our animation timer
    animationId = startTimer(20);
    m_fAverageRating = 0;
}

TouHouFM::~TouHouFM()
{
    settings->setValue("volume",play->volume());

    //    delete ui;
    m_sockInfo->deleteLater();
}

void TouHouFM::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    // Update the status message
    switch(status)
    {
    case QMediaPlayer::UnknownMediaStatus:
        m_sStatus = tr("Unknown");
        //        ui->labelInfo->setText("Unknown");
        break;
    case QMediaPlayer::NoMedia:
        m_sStatus = tr("No Media!");
        break;
    case QMediaPlayer::LoadingMedia:
        m_sStatus = tr("Loading ...");
        break;
    case QMediaPlayer::LoadedMedia:
        m_sStatus = tr("Loaded!");
        break;
    case QMediaPlayer::StalledMedia:
        m_sStatus = tr("Stalled!");
        break;
    case QMediaPlayer::BufferingMedia:
        m_sStatus = tr("Buffering...");
        break;
    case QMediaPlayer::BufferedMedia:
        m_sStatus = tr("Buffered");
        break;
    case QMediaPlayer::EndOfMedia:
        m_sStatus = tr("Done!");
        retryId = startTimer(3000);
        break;
    case QMediaPlayer::InvalidMedia:
        m_sStatus = tr("Invalid Media!");
        retryId = startTimer(3000);
        break;

    }
    m_last = QTime::currentTime();
    update();
}

void TouHouFM::mousePressEvent(QMouseEvent *event) {
    m_pMouseClick = event->pos();

    if(m_areas["playpause"].contains(event->pos()))
    {
        if(play->state() == QMediaPlayer::PlayingState)
        {
            play->stop();
            killTimer(retryId);
        }
        else
        {
            play->play();
        }
    }
    else if(m_areas["volume"].contains(event->pos()))
    {
        m_nTarget = 1;
    }
    else if(m_areas["info"].contains(event->pos()))
    {
        QApplication::clipboard()->setText(m_sInfo);

        m_sStatus = "Copied!";
        m_last = QTime::currentTime();
    }
    else if(m_areas["rating"].contains(event->pos()))
    {
        QRectF pos = QRectF(m_areas["rating"].topLeft(),m_sRateStar);
        for(int i=0;i < 7;i++)
        {
            if(pos.contains(event->pos()))
            {
                sendRating(i);
                m_sStatus = tr("Rating: %1").arg(m_slRate[i]);
                m_last = QTime::currentTime();
            }
            else
            {
            }
            pos.translate(m_sRateStar.width(),0);
        }
    }
    else if(m_areas["report"].contains(event->pos()))
    {
        report();
    }
    else if(event->button() == Qt::LeftButton)
    {
        m_nTarget = 2;
    }
}

void TouHouFM::mouseMoveEvent(QMouseEvent *event) {
    if(m_nTarget == 2)
        move(event->globalPos() - m_pMouseClick);
    if(m_nTarget == 1)
    {
        QPointF rel = event->pos() - m_areas["volume"].center();

        qreal ang = atan2(rel.y(),rel.x())/M_PI/2.0+.5;

        play->setVolume(ang*100);

        update();
    }

    m_pMouse = event->pos();
    update();
}

void TouHouFM::mouseReleaseEvent(QMouseEvent *event)
{
    if(m_nTarget == 1)
    {
        if(event->pos() == m_pMouseClick)
        {
            play->setMuted(play->isMuted());
            //aout->setMute(!aout->isMute());
        }
    }
    m_nTarget = 0;
}

void TouHouFM::metaDataChanged(QString field, QVariant value)
{
    meta[field] = value;

    QString status = tr("%1 by %2 from %3 << %4 >>").arg(meta.value("Title",QString("~")).toString())
            .arg(meta.value("Artist",QString("~")).toString())
            .arg(meta.value("Album",QString("~")).toString())
            .arg(meta.value("Circle",QString("~")).toString());

    if(status!=m_sInfo)
    {
        m_sInfo = status;
        update();
        //        ui->labelInfo->setText(status);
        m_systray->showMessage(tr("Now Playing:"),tr("Title: %1\nArtist: %2\nAlbum: %3\nCircle: %4").arg(meta.value("Title",QString("~")).toString())
                               .arg(meta.value("Artist",QString("~")).toString())
                               .arg(meta.value("Album",QString("~")).toString())
                               .arg(meta.value("Circle",QString("~")).toString()));
    }

    if(field == "progress")
    {
        m_rProgress = value.toFloat();
        //        ui->progress->setValue(value.toFloat());
    }
    else if(field == "user-rating")
    {
        m_iRating = value.toInt() +3;
    }
    else if(field == "rating")
    {
        m_fGlobalRating = value.toFloat() + 3;
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
    handleAction(m_menu->exec(event->globalPos()));
}

void TouHouFM::timerEvent(QTimerEvent *event)
{

    if(event->timerId() == downloadId)
    {

    }
    else if(event->timerId() == retryId)
    {
        play->play();
        killTimer(retryId);
        //        network->get(QNetworkRequest(infoUrl));
    }
    else if(event->timerId() == animationId)
    {
        qreal newrate = .3*m_fGlobalRating + .7*m_fAverageRating;
        if(qAbs(m_fAverageRating - newrate) > 0.001)
        {
            m_fAverageRating = newrate;
            update();
        }
        else
        {
            m_fAverageRating = m_fGlobalRating;
        }
    }
}

void TouHouFM::resizeEvent(QResizeEvent *event)
{
    Q_UNUSED(event)
    setMask(m_bMask);

}

void TouHouFM::paintEvent(QPaintEvent *)
{
    QString text;
    if(m_last.secsTo(QTime::currentTime()) < 3)
    {
        text = m_sStatus;
    }
    else
    {
        text = m_sInfo;
    }

    QPainter p(this);

    p.setRenderHint(QPainter::Antialiasing,true);

    p.drawPixmap(0,0,m_pixmaps["background"]);

    p.setOpacity(.75+.25*(m_areas["playpause"].contains(m_pMouse)));
    p.drawPixmap(m_areas["playpause"],play->state() == QMediaPlayer::PlayingState?m_pixmaps["stop"]:m_pixmaps["play"],QRectF(QPointF(0,0),m_pixmaps["stop"].size()));

    p.setOpacity(.75+.25*(m_areas["report"].contains(m_pMouse)));
    p.drawPixmap(m_areas["report"],m_pixmaps["report"],QRectF(QPointF(0,0),m_pixmaps["report"].size()));

    p.setOpacity(.5);
    p.setBrush(play->isMuted()?Qt::gray : Qt::white);
    p.setPen(Qt::NoPen);
    p.drawPie(m_areas["volume-inner"],16.0*180.0-16.0*180.0*play->volume()/50,-(16.0*360.0-16.0*180.0*play->volume()/50));

    p.setOpacity(.75+.25*(m_areas["volume"].contains(m_pMouse)));
    if(!play->isMuted())
    {
        p.drawPixmap(m_areas["volume"],m_pixmaps["volume"],QRectF(QPointF(0,0),m_pixmaps["volume"].size()));
    }
    else
    {
        p.drawPixmap(m_areas["volume"],m_pixmaps["volume-mute"],QRectF(QPointF(0,0),m_pixmaps["volume-mute"].size()));
    }

    p.setOpacity(1.0);
    p.setPen(Qt::black);
    p.setFont(m_f);

    QRectF pos = QRectF(m_areas["rating"].topLeft(),m_sRateStar);
    p.drawPixmap(m_areas["rating"].topLeft() + QPoint((.5+m_fAverageRating)*m_sRateStar.width()-m_pixmaps["arrow"].width()/2,(m_areas["rating"].height()-m_pixmaps["arrow"].height())/2.0),m_pixmaps["arrow"]);
    for(int i=0;i < 7;i++)
    {
        if(pos.contains(m_pMouse) || i == m_iRating)
        {
            p.drawPixmap(pos,m_pixmaps["rating"],QRect(QPoint(m_sRateStar.width()*i,m_sRateStar.height()),m_sRateStar));
        }
        else
        {
            p.drawPixmap(pos,m_pixmaps["rating"],QRect(QPoint(m_sRateStar.width()*i,0),m_sRateStar));
        }
        pos.translate(m_sRateStar.width(),0);
    }

    p.setOpacity(.25);
    p.drawPixmap(m_areas["rating"].topLeft() + QPoint((.5+m_fAverageRating)*m_sRateStar.width()-m_pixmaps["arrow"].width()/2,(m_areas["rating"].height()-m_pixmaps["arrow"].height())/2.0),m_pixmaps["arrow"]);
    p.setOpacity(1);

    QFontMetrics m(m_f);
    QRect m_statusTextSize;
    m_statusTextSize.setSize(m.boundingRect(text+" | ").size());

    QRectF renderRect = m_areas["info"];

    m_statusTextSize.moveTop(renderRect.y()+(renderRect.height()-m_statusTextSize.height())/2);
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
        p.drawText(m_statusTextSize,text+" | ");
        m_statusTextSize.moveLeft(-fmod(m_rProgress,10.0)/10.0*m_statusTextSize.width()+renderRect.x()+m_statusTextSize.width());
        p.drawText(m_statusTextSize,text+" | ");
    }
    else
    {
        p.drawText(m_statusTextSize,text);
    }
    p.setPen(QPen(QColor::fromRgb(0,64,128),2));
    p.drawLine(renderRect.bottomLeft()+QPointF(1,-3),renderRect.bottomLeft()+QPointF(1+renderRect.width()*m_rProgress/100.0,-3));

    p.setPen(Qt::black);
    p.setFont(m_f2);
    p.setClipRect(m_areas["time"]);
    p.drawText(m_areas["time"],m_sTime);
}

void TouHouFM::sendRequest()
{
}

void TouHouFM::sendRating(int rating)
{
    m_sockInfo->rateSong(rating);
}

void TouHouFM::volumeChanged(qreal vol)
{
    m_sStatus = tr("Volume: %1%").arg(vol*100.0,0,'f',1);
    m_last = QTime::currentTime();
}

void TouHouFM::wheelEvent(QWheelEvent *event)
{
    if(QRectF(312,272,64,64).contains(event->pos()))
    {
        play->setVolume(play->volume() + event->angleDelta().y()/360.0);
    }
}

void TouHouFM::report()
{
    Report box("TouHou.FM",tr("Reporting %1 by %2 from %3 << %4 >>").arg(meta.value("Title",QString("~")).toString())
               .arg(meta.value("Artist",QString("~")).toString())
               .arg(meta.value("Album",QString("~")).toString())
               .arg(meta.value("Circle",QString("~")).toString()),m_sockInfo->getWhats());

    if(box.exec() == QDialog::Accepted)
    {
        qDebug() << "Sending report";

        m_sockInfo->sendReport(box.getWhat(),box.getDetail());

    }
}

void TouHouFM::sendSkip()
{
    qDebug() << "Sending skip";
    m_sockInfo->skipSong();
}

void TouHouFM::login()
{
    m_sockInfo->login();

}

void TouHouFM::storeAuthToken(QString token)
{
    settings->setValue("authtoken",token);
}

void TouHouFM::showUrl(QUrl url)
{
    QDesktopServices::openUrl(url);

    if(QMessageBox::question(this,tr("Touhou.FM Radio"),tr("Please authorize and click Ok when ready."),QMessageBox::Ok,QMessageBox::NoButton) == QMessageBox::Ok)
    {
        m_sockInfo->login();
    }

}

void TouHouFM::showNotification(QString type, QString text)
{
    QMessageBox::information(this,tr("TouHou.FM Radio"),type + ": " + text);
}

void TouHouFM::loadSkin(QString path)
{
    QSvgRenderer r(path,this);

    {
        QStringList items;

        items << "time" << "info" << "rating" << "playpause" << "volume" << "volume-inner" << "report";

        foreach(QString item, items)
        {
            m_areas[item] = r.boundsOnElement("rect-"+item);
        }
    }

    {
        QStringList items;
        items << "background" << "play" << "stop" << "volume" << "volume-mute" << "rating" << "arrow" << "report";

        foreach(QString item, items)
        {
            QRectF rc = r.boundsOnElement(item);
            m_pixmaps[item] = QPixmap(rc.size().toSize());
            m_pixmaps[item].fill(QColor(0,0,0,0));

            QPainter p2(&m_pixmaps[item]);
            r.render(&p2,item);
        }
    }

    m_sRateStar = m_pixmaps["rating"].size();
    m_sRateStar.rwidth()  /= 7;
    m_sRateStar.rheight() /= 2;

    m_bMask = m_pixmaps["background"].mask();

    setMinimumSize(m_pixmaps["background"].size());
    setMaximumSize(m_pixmaps["background"].size());

    m_f = m_f2 = font();
    m_f.setPixelSize(m_areas["info"].height()*.8);
    m_f2.setPixelSize(m_areas["time"].height()*.8);


}

void TouHouFM::handleAction(QAction *action)
{
    if(m_skinAssoc.contains(action))
    {
        loadSkin(m_skinAssoc[action]);
        settings->setValue("skin",m_skinAssoc[action]);
    }
}
