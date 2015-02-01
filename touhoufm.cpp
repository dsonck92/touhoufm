#include "touhoufm.h"
#include "ui_touhoufm.h"

#include <QAudioDeviceInfo>
#include <QMessageBox>

#include <QMenu>

#include <QNetworkReply>
#include <QSettings>
#include <QTimerEvent>

TouHouFM::TouHouFM(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::TouHouFM)
{
    ui->setupUi(this);

    retryId = -1;

    m_menu = new QMenu;

    m_nMove = false;

    player = new QMediaPlayer(this);


    connect(ui->pushPlay,SIGNAL(pressed()),player,SLOT(play()));
    connect(ui->pushStop,SIGNAL(pressed()),player,SLOT(stop()));
    connect(player,SIGNAL(volumeChanged(int)),ui->sliderVolume,SLOT(setValue(int)));
    connect(ui->sliderVolume,SIGNAL(valueChanged(int)),player,SLOT(setVolume(int)));
    connect(ui->pushMute,SIGNAL(toggled(bool)),player,SLOT(setMuted(bool)));
    connect(player,SIGNAL(mutedChanged(bool)),ui->pushMute,SLOT(setChecked(bool)));

    connect(player,SIGNAL(stateChanged(QMediaPlayer::State)),SLOT(stateChanged(QMediaPlayer::State)));
    connect(player,SIGNAL(mediaStatusChanged(QMediaPlayer::MediaStatus)),SLOT(mediaStatusChanged(QMediaPlayer::MediaStatus)));

    connect(player,SIGNAL(bufferStatusChanged(int)),ui->progress,SLOT(setValue(int)));
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

    network->get(QNetworkRequest(QUrl("http://touhou.fm/radios.xml")));

    settings = new QSettings(this);

    player->setMedia(QMediaContent(settings->value("url",QUrl("http://www.touhou.fm/m/touhou")).toUrl()));
    infoUrl = settings->value("info",QUrl("http://touhou.fm/wp-content/plugins/touhou.fm-plugin/xml.php")).toUrl();

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

    ui->sliderVolume->setValue(settings->value("volume",80).toInt());

    m_systray = new QSystemTrayIcon(this);

    m_systray->setContextMenu(m_menu);
    m_systray->setIcon(QIcon(":/images/icon.png"));
    m_systray->show();

    connect(m_systray,SIGNAL(activated(QSystemTrayIcon::ActivationReason)),SLOT(systrayActivated(QSystemTrayIcon::ActivationReason)));
}

TouHouFM::~TouHouFM()
{
    settings->setValue("volume",ui->sliderVolume->value());
    settings->setValue("shown",isVisible());

    delete ui;
}

void TouHouFM::stateChanged(QMediaPlayer::State state)
{
    switch(state)
    {
    case QMediaPlayer::StoppedState:
        ui->pushStop->setChecked(true);
        settings->setValue("state","stopped");
        break;
    case QMediaPlayer::PlayingState:
        ui->pushPlay->setChecked(true);
        settings->setValue("state","playing");
        break;
    default:
        ui->pushStop->setChecked(false);
        ui->pushPlay->setChecked(false);
    }
}

void TouHouFM::mediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    switch(status)
    {
    case QMediaPlayer::UnknownMediaStatus:
        ui->labelInfo->setText("Unknown");
        break;
    case QMediaPlayer::NoMedia:
        ui->labelInfo->setText("No Media!");
        break;
    case QMediaPlayer::LoadingMedia:
        ui->labelInfo->setText("Loading ...");
        break;
    case QMediaPlayer::LoadedMedia:
        ui->labelInfo->setText("Loaded!");
        break;
    case QMediaPlayer::StalledMedia:
        ui->labelInfo->setText("Stalled!");
        break;
    case QMediaPlayer::BufferingMedia:
        ui->labelInfo->setText("Buffering...");
        break;
    case QMediaPlayer::BufferedMedia:
        ui->labelInfo->setText("Buffered");
        break;
    case QMediaPlayer::EndOfMedia:
        ui->labelInfo->setText("Done!");
        retryId = startTimer(3000);
        break;
    case QMediaPlayer::InvalidMedia:
        ui->labelInfo->setText("Invalid Media!");
        retryId = startTimer(3000);
        break;

    }
}

void TouHouFM::showRadios()
{
    QAction *act = m_radios->exec(ui->pushRadio->mapToGlobal(QPoint(0, 0)));

    if(act && radio_data.contains(act))
    {
        RadioInfo info = radio_data.value(act);

        player->setMedia(info.stream);
        infoUrl = info.info;

        ui->title->setText(QString("TouHou.FM Player - %1").arg(info.name));

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
                speed = (child.text().toFloat() - progress);
                progress = child.text().toFloat();
                progress_auto = child.text().toFloat();
            }
            else
            {
                metaDataChanged(child.tagName(),child.text());
            }

            child = child.nextSiblingElement();
        }
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

    QString status = QString("<b>%1</b> by <b>%2</b> from <b>%3</b> &lt;&lt; <b>%4</b> &gt;&gt;").arg(meta.value("Title",QString("~")).toString())
            .arg(meta.value("Artist",QString("~")).toString())
            .arg(meta.value("Album",QString("~")).toString())
            .arg(meta.value("AlbumArtist",QString("~")).toString());

    if(status!=ui->labelInfo->text())
    {
        ui->labelInfo->setText(status);
        m_systray->showMessage("Now Playing:",QString("Title: %1\nArtist: %2\nAlbum: %3\nCircle: %4").arg(meta.value("Title",QString("~")).toString())
                               .arg(meta.value("Artist",QString("~")).toString())
                               .arg(meta.value("Album",QString("~")).toString())
                               .arg(meta.value("AlbumArtist",QString("~")).toString()));
    }

    if(field == "progress")
    {

        ui->progress->setValue(value.toFloat());
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

        ui->title->setText(QString("TouHou.FM Player - %1").arg(info.name));

        settings->setValue("url",info.stream);
        settings->setValue("info",info.info);

        player->play();

    }
}

void TouHouFM::timerEvent(QTimerEvent *event)
{

    if(event->timerId() == downloadId)
    {
        network->get(QNetworkRequest(infoUrl));

    }
    else if(event->timerId() == retryId)
    {
        player->play();
        killTimer(retryId);
    }
    else
    {
        progress_auto += speed/5.0;
        metaDataChanged("progress",progress_auto);

    }
}
