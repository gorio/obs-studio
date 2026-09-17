#pragma once

#include <QWidget>

class QLabel;
class QPushButton;

class YondCastDock final : public QWidget {
	Q_OBJECT

public:
	explicit YondCastDock(QWidget *parent = nullptr);

public slots:
	void setStreamingActive(bool active);
	void setRecordingActive(bool active);
	void refreshStatus();

private:
	QLabel *engineStatusLabel = nullptr;
	QLabel *streamingStatusLabel = nullptr;
	QLabel *recordingStatusLabel = nullptr;
	QPushButton *refreshButton = nullptr;
};
