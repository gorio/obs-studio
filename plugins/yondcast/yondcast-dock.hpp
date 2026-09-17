#pragma once

#include <QWidget>

class QLabel;
class QPushButton;
class QStackedWidget;

class YondCastDock final : public QWidget {
	Q_OBJECT

public:
	explicit YondCastDock(QWidget *parent = nullptr);

public slots:
	void setStreamingActive(bool active);
	void setRecordingActive(bool active);
	void refreshStatus();

private slots:
	void toggleStreaming();
	void toggleRecording();
	void toggleStudioMode();
	void showProducePage();
	void showEngagePage();
	void showEarnPage();

private:
	void updateQuickActions();
	void setPage(int index);

	QLabel *engineStatusLabel = nullptr;
	QLabel *streamingStatusLabel = nullptr;
	QLabel *recordingStatusLabel = nullptr;
	QLabel *profileLabel = nullptr;
	QLabel *collectionLabel = nullptr;
	QLabel *sceneLabel = nullptr;
	QLabel *studioModeLabel = nullptr;

	QPushButton *streamButton = nullptr;
	QPushButton *recordButton = nullptr;
	QPushButton *studioModeButton = nullptr;
	QPushButton *produceButton = nullptr;
	QPushButton *engageButton = nullptr;
	QPushButton *earnButton = nullptr;
	QPushButton *refreshButton = nullptr;

	QStackedWidget *pages = nullptr;
};
