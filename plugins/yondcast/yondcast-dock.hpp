#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
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
	void selectProgramScene(int index);
	void selectPreviewScene(int index);
	void triggerTransition();
	void openGuestInvite();
	void copyGuestInvite();
	void addGuestSource();
	void showProducePage();
	void showEngagePage();
	void showEarnPage();

private:
	void updateQuickActions();
	void updateSceneControls();
	void setPage(int index);
	QString guestInviteUrl() const;
	QString guestBridgeUrl() const;

	QLabel *engineStatusLabel = nullptr;
	QLabel *streamingStatusLabel = nullptr;
	QLabel *recordingStatusLabel = nullptr;
	QLabel *profileLabel = nullptr;
	QLabel *collectionLabel = nullptr;
	QLabel *sceneLabel = nullptr;
	QLabel *studioModeLabel = nullptr;
	QLabel *guestStatusLabel = nullptr;

	QComboBox *programSceneCombo = nullptr;
	QComboBox *previewSceneCombo = nullptr;
	QLineEdit *guestBaseUrlEdit = nullptr;
	QLineEdit *guestSessionEdit = nullptr;
	QLineEdit *guestNameEdit = nullptr;
	QPushButton *transitionButton = nullptr;
	QPushButton *guestOpenButton = nullptr;
	QPushButton *guestCopyButton = nullptr;
	QPushButton *guestAddSourceButton = nullptr;
	QPushButton *streamButton = nullptr;
	QPushButton *recordButton = nullptr;
	QPushButton *studioModeButton = nullptr;
	QPushButton *produceButton = nullptr;
	QPushButton *engageButton = nullptr;
	QPushButton *earnButton = nullptr;
	QPushButton *refreshButton = nullptr;

	QStackedWidget *pages = nullptr;
};
