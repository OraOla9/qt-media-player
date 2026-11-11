#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QVideoWidget>
#include <QSlider>
#include <QPushButton>
#include <QLabel>
#include <QComboBox>
#include <QListWidget>
#include <QFileDialog>
#include <QTime>
#include <QKeyEvent>
#include <QEvent>
#include <QTimer>
#include <QApplication>
#include <QSettings>
#include <QCloseEvent>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openFolder();
    void openFile();
    void play();
    void pause();
    void stop();
    void next();
    void previous();
    void setVolume(int volume);
    void setPosition(int position);
    void durationChanged(qint64 duration);
    void positionChanged(qint64 position);
    void updateDurationInfo(qint64 currentInfo);
    void setPlaybackMode(int mode);
    void setPlaybackRate(qreal rate);
    void addToPlaylist(const QStringList &fileNames);
    void onPlaylistItemDoubleClicked(QListWidgetItem *item);
    void handleMediaStatusChanged(QMediaPlayer::MediaStatus status);
    void toggleVideoFullscreen();
    void clearPlaylist();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;

private:
    Ui::MainWindow *ui;
    QMediaPlayer *player;
    QMediaPlaylist *playlist;
    QVideoWidget *videoWidget;
    QListWidget *playlistWidget;
    QAction *openFileAct;
    QAction *openFolderAct;
    QPushButton *addBtn;
    QPushButton *delBtn;
    QPushButton *clearBtn;
    QPushButton *sortBtn;
    QPushButton *playButton;
    QPushButton *stopButton;
    QPushButton *nextButton;
    QPushButton *previousButton;
    QPushButton *fullscreenButton;
    QSlider *positionSlider;
    QSlider *volumeSlider;
    QLabel *durationLabel;
    QComboBox *playbackModeCombo;
    QComboBox *playbackRateCombo;
    bool isVideo;
    QStringList playlistFilePaths;
    void setupUI();
    void setupConnections();
    void updatePlaybackMode();
    QString formatTime(qint64 ms);
};
#endif // MAINWINDOW_H
