#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QStyle>
#include <QToolBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include <QDockWidget>
#include <QDebug>
#include <QMediaService>
#include <QMediaMetaData>
#include <QRandomGenerator>
#include <random>
#include <QDirIterator>
#include <QToolButton>
#include <QTimer>
#include <QSettings>
#include <QCloseEvent>
#include <QCoreApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , isVideo(false)
{
    ui->setupUi(this);
    try {
        // 创建视频显示区域
        videoWidget = new QVideoWidget(this);
        videoWidget->setAspectRatioMode(Qt::KeepAspectRatio);
        setCentralWidget(videoWidget);

        // 创建播放器和播放列表
        player = new QMediaPlayer(this);
        playlist = new QMediaPlaylist(this);
        // 设置视频输出
        player->setVideoOutput(videoWidget);
        player->setPlaylist(playlist);

        // 设置其他UI组件
        setupUI();
        setupConnections();

        // 设置初始音量
        player->setVolume(50);

        // 连接错误信号
        connect(player, QOverload<QMediaPlayer::Error>::of(&QMediaPlayer::error),
                this, [this](QMediaPlayer::Error error) {
            QString errorMessage = QString("Error %1: %2").arg(error).arg(player->errorString());
            qDebug() << errorMessage;
            QMessageBox::warning(this, "Media Player Error", errorMessage);
        });

        // 连接媒体状态变化信号
        connect(player, &QMediaPlayer::mediaStatusChanged, [this](QMediaPlayer::MediaStatus status) {
            qDebug() << "Media status changed:" << status;
            switch (status) {
                case QMediaPlayer::UnknownMediaStatus:
                    qDebug() << "Unknown media status";
                    break;
                case QMediaPlayer::NoMedia:
                    qDebug() << "No media";
                    break;
                case QMediaPlayer::LoadingMedia:
                    qDebug() << "Loading media";
                    break;
                case QMediaPlayer::LoadedMedia:
                    qDebug() << "Media loaded";
                    break;
                case QMediaPlayer::StalledMedia:
                    qDebug() << "Media stalled";
                    break;
                case QMediaPlayer::BufferingMedia:
                    qDebug() << "Buffering media";
                    break;
                case QMediaPlayer::BufferedMedia:
                    qDebug() << "Media buffered";
                    break;
                case QMediaPlayer::EndOfMedia:
                    qDebug() << "End of media";
                    break;
                case QMediaPlayer::InvalidMedia:
                    qDebug() << "Invalid media";
                    break;
            }
        });

        setWindowTitle("OraOla Media Player");
        
        // 在构造函数中安装应用程序级别的事件过滤器
        qApp->installEventFilter(this);

        // 加载保存的设置
        QSettings settings("./settings.ini", QSettings::IniFormat);
        player->setVolume(settings.value("volume", 50).toInt());
        volumeSlider->setValue(player->volume());
        playbackModeCombo->setCurrentIndex(settings.value("playbackMode", 0).toInt());
        playlist->setPlaybackMode(static_cast<QMediaPlaylist::PlaybackMode>(settings.value("playbackMode", 0).toInt()));
        player->setPlaybackRate(settings.value("playbackRate", 1.0).toDouble());
        // 根据playbackRateCombo的itemData来设置playbackRateCombo的当前index
        qreal savedRate = settings.value("playbackRate", 1.0).toDouble();
        for (int i = 0; i < playbackRateCombo->count(); ++i) {
            if (qFuzzyCompare(playbackRateCombo->itemData(i).toDouble(), savedRate)) {
                playbackRateCombo->setCurrentIndex(i);
                break;
            }
        }

        QStringList savedPlaylist = settings.value("playlist", QStringList()).toStringList();
        if (!savedPlaylist.isEmpty()) {
            addToPlaylist(savedPlaylist);
            QString lastPlayedFile = settings.value("lastPlayedFile").toString();
            qint64 lastPlayedPosition = settings.value("lastPlayedPosition", 0).toLongLong();

            if (!lastPlayedFile.isEmpty()) {
                int index = -1;
                for (int i = 0; i < playlist->mediaCount(); ++i) {
                    if (playlist->media(i).canonicalUrl().toLocalFile() == lastPlayedFile) {
                        index = i;
                        break;
                    }
                }
                if (index != -1) {
                    playlist->setCurrentIndex(index);
                    player->play();
                    player->setPosition(lastPlayedPosition);
                }
            }

        }

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Initialization error: %1").arg(e.what()));
    }
}

MainWindow::~MainWindow()
{
    if (player) {
        player->stop();
    }
    delete ui;
}

void MainWindow::setupUI()
{
    try {
        //主窗口
        setStyleSheet("background-color: #f7f7f7; color: #222;");
        QWidget *centralWidget = new QWidget(this);
        //垂直布局管理
        QVBoxLayout *mainVLayout = new QVBoxLayout(centralWidget);
        mainVLayout->setContentsMargins(0, 0, 0, 0);    //子部件之间紧挨
        mainVLayout->setSpacing(0);


        // 顶部信息栏 + 菜单
        QWidget *topBar = new QWidget(this);
        topBar->setStyleSheet("background: #f0f0f0; border-bottom: 1px solid #e0e0e0;"); //下边线
        QHBoxLayout *topBarLayout = new QHBoxLayout(topBar);
        topBarLayout->setContentsMargins(12, 6, 12, 6);
        // OraOlaMediaPlay菜单按钮
        QToolButton *oraMenuBtn = new QToolButton(this);
        oraMenuBtn->setText("OraOlaMediaPlay");
        oraMenuBtn->setPopupMode(QToolButton::InstantPopup);
        QMenu *oraMenu = new QMenu(this);
        openFileAct = oraMenu->addAction("打开文件");
        openFolderAct = oraMenu->addAction("打开文件夹");
        oraMenuBtn->setMenu(oraMenu);
        topBarLayout->addWidget(oraMenuBtn);
        // 当前播放信息
        QLabel *nowPlayingLabel = new QLabel("当前播放: ", this);
        nowPlayingLabel->setObjectName("nowPlayingLabel");
        nowPlayingLabel->setStyleSheet("font-weight: bold; font-size: 15px; color: #333;");
        topBarLayout->addWidget(nowPlayingLabel);
        //调整（空白）
        topBarLayout->addStretch();
        // 视频全屏按钮
        fullscreenButton = new QPushButton(this);
        fullscreenButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));//获取图标
        fullscreenButton->setFixedSize(28, 28);
        fullscreenButton->setStyleSheet("QPushButton { background: none; border: none; } QPushButton:hover { background: #e0e0e0; border-radius: 3px; }");
        topBarLayout->addWidget(fullscreenButton);

        mainVLayout->addWidget(topBar);


        // 中间区域
        QHBoxLayout *centerLayout = new QHBoxLayout();
        centerLayout->setContentsMargins(0, 0, 0, 0);
        centerLayout->setSpacing(0);

        // 视频区
        QWidget *videoArea = new QWidget(this);
        videoArea->setObjectName("videoArea");
        QVBoxLayout *videoLayout = new QVBoxLayout(videoArea);
        videoLayout->setContentsMargins(24, 16, 8, 0);
        videoLayout->setSpacing(8);
        // 直接添加视频控件
        videoLayout->addWidget(videoWidget, 1);
        centerLayout->addWidget(videoArea, 7);
        
        // 右侧播放列表区
        QWidget *rightPanel = new QWidget(this);
        rightPanel->setStyleSheet("background: #fafafa; border-left: 1px solid #e0e0e0;");
        QVBoxLayout *rightLayout = new QVBoxLayout(rightPanel);
        rightLayout->setContentsMargins(8, 16, 8, 8);
        //标签
        QLabel *playlistLabel = new QLabel("播放列表", this);
        playlistLabel->setStyleSheet("font-weight: bold; font-size: 14px; color: #444;");
        rightLayout->addWidget(playlistLabel);
        //初始化播放列表
        playlistWidget = new QListWidget(this);
        playlistWidget->setMinimumWidth(240);
        playlistWidget->setStyleSheet("QListWidget { background: #fff; border: 1px solid #e0e0e0; border-radius: 4px; } QListWidget::item:selected { background: #e6f0fa; color: #222; }");
        rightLayout->addWidget(playlistWidget, 1);
        //列表调整按钮
        QHBoxLayout *plistBtnLayout = new QHBoxLayout();
        addBtn = new QPushButton("添加", this);
        delBtn = new QPushButton("删除", this);
        clearBtn = new QPushButton("清空", this);
        sortBtn = new QPushButton("排序", this);
        addBtn->setStyleSheet("QPushButton { background: #f0f0f0; border: 1px solid #e0e0e0; border-radius: 3px; padding: 2px 10px; } QPushButton:hover { background: #e6f0fa; }");
        delBtn->setStyleSheet(addBtn->styleSheet());
        sortBtn->setStyleSheet(addBtn->styleSheet());
        clearBtn->setStyleSheet(addBtn->styleSheet());
        plistBtnLayout->addWidget(addBtn);
        plistBtnLayout->addWidget(delBtn);
        plistBtnLayout->addWidget(sortBtn);
        plistBtnLayout->addWidget(clearBtn);
        plistBtnLayout->addStretch();
        rightLayout->addLayout(plistBtnLayout);
        centerLayout->addWidget(rightPanel, 3);
        mainVLayout->addLayout(centerLayout, 1);



        // 底部控制栏
        QWidget *bottomBar = new QWidget(this);
        bottomBar->setStyleSheet("background: #f0f0f0; border-top: 1px solid #e0e0e0;");
        QHBoxLayout *bottomLayout = new QHBoxLayout(bottomBar);
        bottomLayout->setContentsMargins(16, 6, 16, 6);

        //调接按钮
        previousButton = new QPushButton(this);
        playButton = new QPushButton(this);
        stopButton = new QPushButton(this);
        nextButton = new QPushButton(this);
        previousButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipBackward));
        playButton->setIcon(style()->standardIcon(QStyle::SP_MediaPlay));
        stopButton->setIcon(style()->standardIcon(QStyle::SP_MediaStop));
        nextButton->setIcon(style()->standardIcon(QStyle::SP_MediaSkipForward));
        previousButton->setFixedSize(28, 28);
        playButton->setFixedSize(28, 28);
        stopButton->setFixedSize(28, 28);
        nextButton->setFixedSize(28, 28);
        bottomLayout->addWidget(previousButton);
        bottomLayout->addWidget(playButton);
        bottomLayout->addWidget(stopButton);
        bottomLayout->addWidget(nextButton);
        bottomLayout->addSpacing(8);
        //播放条
        positionSlider = new QSlider(Qt::Horizontal, this);
        positionSlider->setRange(0, 0);
        positionSlider->setMinimumWidth(120);
        positionSlider->setMaximumWidth(300);
        bottomLayout->addWidget(positionSlider, 1);
        //播放时间
        durationLabel = new QLabel("00:00:00 / 00:00:00", this);
        durationLabel->setStyleSheet("color: #666; font-size: 12px; min-width: 150px;");
        durationLabel->setAlignment(Qt::AlignCenter);
        bottomLayout->addWidget(durationLabel);

        // 音量
        QLabel *volumeIconLabel = new QLabel(this);
        volumeIconLabel->setPixmap(style()->standardIcon(QStyle::SP_MediaVolume).pixmap(QSize(16, 16))); 
        volumeIconLabel->setFixedSize(16, 16); 
        bottomLayout->addWidget(volumeIconLabel);
        volumeSlider = new QSlider(Qt::Horizontal, this);
        volumeSlider->setRange(0, 100);
        volumeSlider->setValue(50);
        volumeSlider->setMaximumWidth(80);
        bottomLayout->addWidget(volumeSlider);
        //播放模式
        playbackModeCombo = new QComboBox(this);
        playbackModeCombo->addItem("顺序播放", 0);
        playbackModeCombo->addItem("单曲循环", 1);
        playbackModeCombo->addItem("随机播放", 2);
        playbackModeCombo->addItem("列表循环", 3);
        playbackModeCombo->setCurrentIndex(0);
        bottomLayout->addWidget(playbackModeCombo);
        //倍数
        playbackRateCombo = new QComboBox(this);
        playbackRateCombo->addItem("0.5x", 0.5);
        playbackRateCombo->addItem("1.0x", 1.0);
        playbackRateCombo->addItem("1.5x", 1.5);
        playbackRateCombo->addItem("2.0x", 2.0);
        playbackRateCombo->setCurrentIndex(1);
        bottomLayout->addWidget(playbackRateCombo);
        mainVLayout->addWidget(bottomBar);
        setCentralWidget(centralWidget);
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("UI setup error: %1").arg(e.what()));
    }
}

void MainWindow::setupConnections()
{
    try {
        connect(fullscreenButton, &QPushButton::clicked, this, &MainWindow::toggleVideoFullscreen);
        connect(openFileAct, &QAction::triggered, this, &MainWindow::openFile);
        connect(openFolderAct, &QAction::triggered, this, &MainWindow::openFolder);
        connect(addBtn, &QPushButton::clicked, this, &MainWindow::openFile);
        connect(delBtn, &QPushButton::clicked, [this]() {
            int row = playlistWidget->currentRow();
            if (row >= 0) {
                playlist->removeMedia(row);
                delete playlistWidget->takeItem(row);
            }
        });
        connect(sortBtn, &QPushButton::clicked, [this]() {
            playlistWidget->sortItems();
        });
        connect(clearBtn, &QPushButton::clicked, this, &MainWindow::clearPlaylist);
        connect(previousButton, &QPushButton::clicked, this, &MainWindow::previous);
        connect(playButton, &QPushButton::clicked, this, &MainWindow::play);
        connect(stopButton, &QPushButton::clicked, this, &MainWindow::stop);
        connect(nextButton, &QPushButton::clicked, this, &MainWindow::next);
        connect(positionSlider, &QSlider::sliderMoved, this, &MainWindow::setPosition);
        connect(volumeSlider, &QSlider::valueChanged, this, &MainWindow::setVolume);
        connect(playbackModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::setPlaybackMode);
        connect(playbackRateCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this](int index) {
            setPlaybackRate(playbackRateCombo->itemData(index).toDouble());
        });
        connect(playlistWidget, &QListWidget::itemDoubleClicked, this, &MainWindow::onPlaylistItemDoubleClicked);
        connect(player, &QMediaPlayer::durationChanged, this, &MainWindow::durationChanged);
        connect(player, &QMediaPlayer::positionChanged, this, &MainWindow::positionChanged);
        connect(player, &QMediaPlayer::stateChanged, [this](QMediaPlayer::State state) {
            playButton->setIcon(style()->standardIcon(state == QMediaPlayer::PlayingState ? QStyle::SP_MediaPause : QStyle::SP_MediaPlay));
        });
        // 添加媒体状态变化连接
        connect(player, &QMediaPlayer::mediaStatusChanged, this, &MainWindow::handleMediaStatusChanged);
        // 为 positionSlider 安装事件过滤器，以支持点击跳转
        positionSlider->installEventFilter(this);

    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Connection setup error: %1").arg(e.what()));
    }
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    //qDebug() << "Event filter active. Event type: " << event->type();
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
        qDebug() << "Key press event in filter: Key = " << keyEvent->key() << ", Text = '" << keyEvent->text() << "'"; // 调试：捕获到的按键事件
        qDebug() << "Object receiving event: " << obj->objectName(); // 调试：事件接收对象

        //速率
        qreal currentRate = player->playbackRate();
        //音量
        int currentVolume = player->volume();

        switch (keyEvent->key()) {
            case Qt::Key_Space:
                qDebug() << "Spacebar pressed. Calling play().";
                play(); // 播放/暂停
                return true; // 事件已处理
            case Qt::Key_Right:
                qDebug() << "Right arrow pressed. Seeking forward.";
                player->setPosition(player->position() + 5000); // 快进5秒
                return true; // 事件已处理
            case Qt::Key_Left:
                qDebug() << "Left arrow pressed. Seeking backward.";
                player->setPosition(player->position() - 5000); // 倒退5秒
                return true; // 事件已处理
            case Qt::Key_C:
            {
                qDebug() << "C pressed. Increasing playback rate.";
                int currentIndex = playbackRateCombo->currentIndex();
                if (currentIndex < playbackRateCombo->count() - 1) {
                    // 直接传递新的目标播放速度给 setPlaybackRate
                    setPlaybackRate(playbackRateCombo->itemData(currentIndex + 1).toDouble());
                }
                return true; // 事件已处理
            }
            case Qt::Key_X:
            {
                qDebug() << "X pressed. Decreasing playback rate.";
                int currentIndex = playbackRateCombo->currentIndex();
                if (currentIndex > 0) {
                    // 直接传递新的目标播放速度给 setPlaybackRate
                    setPlaybackRate(playbackRateCombo->itemData(currentIndex - 1).toDouble());
                }
                return true; // 事件已处理
            }
            case Qt::Key_Z:
            {
                qDebug() << "Z pressed. Resetting playback rate.";
                // 直接传递 1.0x 播放速度给 setPlaybackRate
                setPlaybackRate(1.0); // 恢复正常播放速度
                return true; // 事件已处理
            }
            case Qt::Key_Up:
                qDebug() << "Up arrow pressed. Increasing volume.";
                setVolume(qMin(100, currentVolume + 5)); // 增加音量
                return true; // 事件已处理
            case Qt::Key_Down:
                qDebug() << "Down arrow pressed. Decreasing volume.";
                setVolume(qMax(0, currentVolume - 5)); // 减少音量
                return true; // 事件已处理
            case Qt::Key_F:
                qDebug() << "F pressed. Toggling fullscreen.";
                toggleVideoFullscreen(); // 视频全屏
                return true; // 事件已处理
            case Qt::Key_Escape: // 处理Esc键退出全屏
                if (videoWidget->isFullScreen()) {
                    qDebug() << "Esc key pressed in fullscreen mode. Calling toggleVideoFullscreen().";
                    toggleVideoFullscreen();
                    return true; // 事件已处理，停止传播
                } else {
                    qDebug() << "Esc key pressed, but videoWidget is not in fullscreen.";
                }
                break; // 如果不是全屏下的Esc键，继续默认处理
            default:
                break;
        }
    }

    // 处理 positionSlider 的鼠标点击事件以实现点击跳转
    if (obj == positionSlider && event->type() == QEvent::MouseButtonPress) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
        if (mouseEvent->button() == Qt::LeftButton) {
            int value = QStyle::sliderValueFromPosition(
                positionSlider->minimum(),
                positionSlider->maximum(),
                mouseEvent->pos().x(),
                positionSlider->width());
            positionSlider->setValue(value);
            player->setPosition(value); // 直接设置播放位置
            return true; // 事件已处理
        }
    }

    return QObject::eventFilter(obj, event); // 继续处理其他事件
}

void MainWindow::openFolder()
{
    try {
        QString folderPath = QFileDialog::getExistingDirectory(this, "选择文件夹", "",
            QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

        if (!folderPath.isEmpty()) {
            // 清空当前播放列表
            clearPlaylist();

            //找到的媒体文件路径
            QStringList mediaFiles;
            //遍历所选文件夹及其子文件夹
            QDirIterator it(folderPath,
                          QStringList() << "*.mp3" << "*.wav" << "*.mp4" << "*.avi" << "*.mkv",
                          QDir::Files,
                          QDirIterator::Subdirectories);

            while (it.hasNext()) {
                mediaFiles << it.next();
            }

            if (!mediaFiles.isEmpty()) {
                addToPlaylist(mediaFiles);
            } else {
                QMessageBox::information(this, "提示", "所选文件夹中没有找到支持的媒体文件");
            }
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Folder open error: %1").arg(e.what()));
    }
}

void MainWindow::openFile()
{
    try {
        QStringList fileNames = QFileDialog::getOpenFileNames(this, "选择文件", QString(),
            "媒体文件 (*.mp3 *.wav *.mp4 *.avi *.mkv);;所有文件 (*.*)");
        if (!fileNames.isEmpty()) {
            // 检查重复文件
            QStringList newFiles;
            for (const QString &fileName : fileNames) {
                if (playlistFilePaths.contains(fileName)) {
                    QMessageBox::warning(this, "警告", "文件已存在: " + QFileInfo(fileName).fileName());
                } else {
                    newFiles.append(fileName);
                }
            }
            if (!newFiles.isEmpty()) {
                addToPlaylist(newFiles);
            }
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("File open error: %1").arg(e.what()));
    }
}

void MainWindow::addToPlaylist(const QStringList &fileNames)
{
    try {
        for (const QString &fileName : fileNames) {
            QUrl url = QUrl::fromLocalFile(fileName);
            if (url.isValid()) {
                playlist->addMedia(url);
                QFileInfo fileInfo(fileName);
                playlistWidget->addItem(fileInfo.fileName());
                playlistFilePaths.append(fileName); // 将完整路径添加到列表中
                qDebug() << "Added to playlist:" << fileInfo.fileName();
            } else {
                QMessageBox::warning(this, "Error", "Invalid file: " + fileName);
            }
        }

        // 设置默认播放模式为顺序播放
        playlist->setPlaybackMode(QMediaPlaylist::Sequential);
        playbackModeCombo->setCurrentIndex(0);

        // 如果播放列表不为空，自动开始播放第一个文件
        if (playlist->mediaCount() > 0) {
            if (player->state() != QMediaPlayer::PlayingState && player->state() != QMediaPlayer::PausedState) {
                playlist->setCurrentIndex(0);
                
                // 更新当前播放标签
                QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
                if (nowPlayingLabel) {
                    QListWidgetItem *item = playlistWidget->item(0);
                    if (item) {
                        nowPlayingLabel->setText("当前播放: " + item->text());
                        qDebug() << "Initial playback:" << item->text();
                    }
                }
                player->play();
            }
        }
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Playlist error: %1").arg(e.what()));
    }
}

void MainWindow::play()
{
    if (player->state() == QMediaPlayer::PlayingState) {
        player->pause();
    } else {
        player->play();
    }
}

void MainWindow::pause()
{
    player->pause();
}

void MainWindow::stop()
{
    player->stop();
}

void MainWindow::next()
{
    // 如果是随机播放模式，使用随机偏移量
    if (playlist->playbackMode() == QMediaPlaylist::Random) {
        int currentIndex = playlist->currentIndex();
        int count = playlist->mediaCount();
        
        if (count > 1) {
            // 生成1到count-1之间的随机数
            int randomOffset = QRandomGenerator::global()->bounded(1, count);
            // 计算新的索引位置
            int nextIndex = (currentIndex + randomOffset) % count;
            
            // 设置下一首
            playlist->setCurrentIndex(nextIndex);
            qDebug() << "随机播放下一首: 当前索引" << currentIndex << "随机偏移" << randomOffset << "新索引" << nextIndex;
            
            // 更新当前播放标签
            QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
            if (nowPlayingLabel) {
                QListWidgetItem *item = playlistWidget->item(nextIndex);
                if (item) {
                    nowPlayingLabel->setText("当前播放: " + item->text());
                    qDebug() << "Updated now playing label to:" << item->text();
                }
            }
            
            player->play();
        } else {
            // 只有一首歌，继续播放
            playlist->setCurrentIndex(0);
            player->play();
        }
    } else {
        // 非随机模式使用Qt的内置功能
        playlist->next();
        
        // 更新当前播放标签
        QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
        if (nowPlayingLabel) {
            int currentIndex = playlist->currentIndex();
            if (currentIndex >= 0) {
                QListWidgetItem *item = playlistWidget->item(currentIndex);
                if (item) {
                    nowPlayingLabel->setText("当前播放: " + item->text());
                    qDebug() << "Updated now playing label to:" << item->text();
                }
            }
        }
    }
}

void MainWindow::previous()
{
    // 如果是随机播放模式，使用随机偏移量
    if (playlist->playbackMode() == QMediaPlaylist::Random) {
        int currentIndex = playlist->currentIndex();
        int count = playlist->mediaCount();
        
        if (count > 1) {
            // 生成1到count-1之间的随机数
            int randomOffset = QRandomGenerator::global()->bounded(1, count);
            // 计算新的索引位置
            int prevIndex = (currentIndex - randomOffset + count) % count;
            // 设置上一首
            playlist->setCurrentIndex(prevIndex);
            qDebug() << "随机播放上一首: 当前索引" << currentIndex << "随机偏移" << randomOffset << "新索引" << prevIndex;
            
            // 更新当前播放标签
            QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
            if (nowPlayingLabel) {
                QListWidgetItem *item = playlistWidget->item(prevIndex);
                if (item) {
                    nowPlayingLabel->setText("当前播放: " + item->text());
                    qDebug() << "Updated now playing label to:" << item->text();
                }
            }
            
            player->play();
        } else {
            // 只有一首歌，继续播放
            playlist->setCurrentIndex(0);
            player->play();
        }
    } else {
        // 非随机模式使用Qt的内置功能
        playlist->previous();
        
        // 更新当前播放标签
        QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
        if (nowPlayingLabel) {
            int currentIndex = playlist->currentIndex();
            if (currentIndex >= 0) {
                QListWidgetItem *item = playlistWidget->item(currentIndex);
                if (item) {
                    nowPlayingLabel->setText("当前播放: " + item->text());
                    qDebug() << "Updated now playing label to:" << item->text();
                }
            }
        }
    }
}

void MainWindow::setVolume(int volume)
{
    player->setVolume(volume);
    volumeSlider->setValue(volume); // 同步音量滑块显示
}

void MainWindow::setPosition(int position)
{
    player->setPosition(position);
}

void MainWindow::durationChanged(qint64 duration)
{
    positionSlider->setRange(0, duration);
    updateDurationInfo(duration);
}

void MainWindow::positionChanged(qint64 position)
{
    positionSlider->setValue(position);
    updateDurationInfo(position);
}

void MainWindow::updateDurationInfo(qint64 currentInfo)
{
    QString currentTime = formatTime(currentInfo);
    QString totalTime = formatTime(player->duration());
    QString tStr = currentTime + " / " + totalTime;
    durationLabel->setText(tStr);
    qDebug() << "Time display:" << tStr; // 添加调试输出
}
QString MainWindow::formatTime(qint64 ms)
{
    int totalSeconds = ms / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;
    
    return QString("%1:%2:%3")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void MainWindow::setPlaybackMode(int mode)
{
    try {
        // 直接使用QMediaPlaylist的枚举值
        QMediaPlaylist::PlaybackMode playbackMode;
        switch(mode) {
            case 0: playbackMode = QMediaPlaylist::Sequential; break;
            case 1: playbackMode = QMediaPlaylist::CurrentItemInLoop; break;
            case 2: playbackMode = QMediaPlaylist::Random; break;
            case 3: playbackMode = QMediaPlaylist::Loop; break;
            default: playbackMode = QMediaPlaylist::Sequential; break;
        }
        
        qDebug() << "Setting playback mode to:" << mode << "Current playlist count:" << playlist->mediaCount();
        
        // 设置播放模式
        playlist->setPlaybackMode(playbackMode);
        
        // 验证播放模式是否设置成功
        QMediaPlaylist::PlaybackMode currentMode = playlist->playbackMode();
        qDebug() << "Actual playback mode after setting:" << currentMode;

        // 添加更详细的调试信息
        QString modeName;
        switch(currentMode) {
            case QMediaPlaylist::Sequential: modeName = "Sequential"; break;
            case QMediaPlaylist::CurrentItemInLoop: modeName = "CurrentItemInLoop"; break;
            case QMediaPlaylist::Random: modeName = "Random"; break;
            case QMediaPlaylist::Loop: modeName = "Loop"; break;
            default: modeName = "Unknown"; break;
        }
        qDebug() << "Playback mode changed to:" << modeName << "(" << currentMode << ")";
    } catch (const std::exception& e) {
        QMessageBox::critical(this, "Error", QString("Failed to set playback mode: %1").arg(e.what()));
    }
}

void MainWindow::handleMediaStatusChanged(QMediaPlayer::MediaStatus status)
{
    if (status == QMediaPlayer::EndOfMedia) {
        qDebug() << "Media ended, current playback mode:" << playlist->playbackMode();
        // 对于随机播放模式，使用随机偏移量
        if (playlist->playbackMode() == QMediaPlaylist::Random) {
            int currentIndex = playlist->currentIndex();
            int count = playlist->mediaCount();
            qDebug() << "Random mode: current index:" << currentIndex << "total count:" << count;
            
            if (count > 1) {
                // 生成1到count-1之间的随机数
                int randomOffset = QRandomGenerator::global()->bounded(1, count);
                // 计算新的索引位置
                int nextIndex = (currentIndex + randomOffset) % count;
                
                // 设置下一首
                playlist->setCurrentIndex(nextIndex);
                qDebug() << "Setting next random track: current index" << currentIndex << "random offset" << randomOffset << "new index" << nextIndex;
                
                // 更新当前播放标签
                QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
                if (nowPlayingLabel) {
                    QListWidgetItem *item = playlistWidget->item(nextIndex);
                    if (item) {
                        nowPlayingLabel->setText("当前播放: " + item->text());
                        qDebug() << "Updated now playing label to:" << item->text();
                    }
                }
                
                player->play();
            } else {
                qDebug() << "Not enough items for random selection";
                // 只有一首歌，继续播放
                playlist->setCurrentIndex(0);
                player->play();
            }
        }
    } else if (status == QMediaPlayer::LoadedMedia) {
        QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
        if (nowPlayingLabel) {
            int currentIndex = playlist->currentIndex();
            if (currentIndex >= 0) {
                QListWidgetItem *item = playlistWidget->item(currentIndex);
                if (item) {
                    nowPlayingLabel->setText("当前播放: " + item->text());
                    qDebug() << "Now playing:" << item->text() << "at index:" << currentIndex;
                }
            }
        }
    }
}

void MainWindow::setPlaybackRate(qreal rate)
{
    qDebug() << "setPlaybackRate called with rate: " << rate; // 调试：收到的rate值
    // 先暂停播放
    player->pause();

    // 设置播放速率
    player->setPlaybackRate(rate);

    // 恢复播放
    player->play();

    // 同步更新播放速度下拉框
    bool foundExactMatch = false;
    for (int i = 0; i < playbackRateCombo->count(); ++i) {
        qreal itemRate = playbackRateCombo->itemData(i).toDouble();
        qDebug() << "  Checking item: " << i << ", rate: " << itemRate; // 调试：遍历项
        if (qFuzzyCompare(itemRate, rate)) {
            playbackRateCombo->setCurrentIndex(i);
            qDebug() << "  Found exact match at index: " << i; // 调试：找到精确匹配
            foundExactMatch = true;
            break;
        }
    }

    if (!foundExactMatch) {
        qDebug() << "No exact match found for playback rate: " << rate << ", trying to find nearest or setting to 1.0x.";
        // 查找最接近的项（简单实现，可以根据需要优化）
        // 记录最小差值
        qreal minDiff = std::numeric_limits<qreal>::max();
        int nearestIndex = 1; // 默认设置为1.0x的索引
        for (int i = 0; i < playbackRateCombo->count(); ++i) {
            qreal itemRate = playbackRateCombo->itemData(i).toDouble();
            qreal diff = qAbs(itemRate - rate);
            qDebug() << "    Nearest search: item " << i << ", rate: " << itemRate << ", diff: " << diff; // 调试：查找最近项过程
            if (diff < minDiff) {
                minDiff = diff;
                nearestIndex = i;
            }
        }
        playbackRateCombo->setCurrentIndex(nearestIndex);
        qDebug() << "  Set to nearest index: " << nearestIndex << " (rate: " << playbackRateCombo->currentData().toDouble() << ")"; // 调试：设置最近项
    }
    qDebug() << "Final playbackRateCombo index: " << playbackRateCombo->currentIndex(); // 调试：最终索引
}

void MainWindow::onPlaylistItemDoubleClicked(QListWidgetItem *item)
{
    int index = playlistWidget->row(item);
    playlist->setCurrentIndex(index);
    player->play();
    // 更新当前播放信息
    QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
    if (nowPlayingLabel) {
        nowPlayingLabel->setText("当前播放: " + item->text());
    }
}

//全屏
void MainWindow::toggleVideoFullscreen()
{
    if (videoWidget->isFullScreen()) {
        videoWidget->setFullScreen(false);
        fullscreenButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarMaxButton));
    } else {
        videoWidget->setFullScreen(true);
        fullscreenButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarNormalButton));
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    QSettings settings("./settings.ini", QSettings::IniFormat);
    settings.setValue("volume", player->volume());
    settings.setValue("playbackMode", playbackModeCombo->currentIndex());
    settings.setValue("playbackRate", player->playbackRate());

    // 保存播放列表
    settings.setValue("playlist", playlistFilePaths);

    // 无论是否正在播放，只要播放列表不为空就保存当前索引
    if (playlist->mediaCount() > 0) {
        int currentPlayIndex = playlist->currentIndex();
        settings.setValue("lastPlayedIndex", currentPlayIndex);
        qDebug() << "Saving lastPlayedIndex: " << currentPlayIndex;
    } else {
        settings.remove("lastPlayedIndex"); // 如果播放列表为空，清除索引
        qDebug() << "Playlist is empty, removing lastPlayedIndex.";
    }

    // 保存当前播放的文件和位置（只要有当前播放媒体且播放器支持定位）
    if (playlist->currentIndex() != -1 && player->isSeekable()) {
        settings.setValue("lastPlayedFile", player->currentMedia().canonicalUrl().toLocalFile());
        settings.setValue("lastPlayedPosition", player->position());
        qDebug() << "Saving lastPlayedPosition: " << player->position();
    } else {
        // 如果没有正在播放的媒体或者播放器不支持定位，清除上次播放信息
        settings.remove("lastPlayedFile");
        settings.remove("lastPlayedPosition");
        qDebug() << "Clearing lastPlayedFile and lastPlayedPosition (no current media or not seekable).";
    }

    settings.sync(); // 强制将更改写入文件
    event->accept();
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // 所有快捷键处理已转移到 eventFilter，这里只调用基类实现
    QMainWindow::keyPressEvent(event);
}

void MainWindow::clearPlaylist()
{
    player->stop();
    playlist->clear();
    playlistWidget->clear();
    playlistFilePaths.clear(); 

    // 重置当前播放标签
    QLabel *nowPlayingLabel = findChild<QLabel*>("nowPlayingLabel");
    if (nowPlayingLabel) {
        nowPlayingLabel->setText("当前播放: ");
    }
    qDebug() << "Playlist cleared.";
}

