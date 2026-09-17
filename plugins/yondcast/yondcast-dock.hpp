#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QLineEdit;
class QNetworkAccessManager;
class QPushButton;
class QStackedWidget;
class QWebSocket;

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
	void connectGuestRoom();
	void addGuestSource();
	void toggleIsoRecording();
	void showProducePage();
	void showEngagePage();
	void showEarnPage();

private:
	void updateQuickActions();
	void updateSceneControls();
	void setPage(int index);
	QString guestInviteUrl() const;
	QString guestBridgeUrl() const;
	QString selectedGuestId() const;
	QString selectedGuestName() const;
	void sendSignaling(const QByteArray &json);
	void handleSignalingMessage(const QString &message);
	void updateGuestCombo(const QString &id, const QString &name, bool connected);
	void startIsoRecording();
	void stopIsoRecording();

	QLabel *engineStatusLabel = nullptr;
	QLabel *streamingStatusLabel = nullptr;
	QLabel *recordingStatusLabel = nullptr;
	QLabel *profileLabel = nullptr;
	QLabel *collectionLabel = nullptr;
	QLabel *sceneLabel = nullptr;
	QLabel *studioModeLabel = nullptr;
	QLabel *guestStatusLabel = nullptr;
	QLabel *isoStatusLabel = nullptr;

	QComboBox *programSceneCombo = nullptr;
	QComboBox *previewSceneCombo = nullptr;
	QComboBox *guestParticipantCombo = nullptr;
	QLineEdit *guestBaseUrlEdit = nullptr;
	QLineEdit *guestSessionEdit = nullptr;
	QLineEdit *guestNameEdit = nullptr;
	QPushButton *transitionButton = nullptr;
	QPushButton *guestOpenButton = nullptr;
	QPushButton *guestCopyButton = nullptr;
	QPushButton *guestConnectButton = nullptr;
	QPushButton *guestAddSourceButton = nullptr;
	QPushButton *isoRecordButton = nullptr;
	QPushButton *streamButton = nullptr;
	QPushButton *recordButton = nullptr;
	QPushButton *studioModeButton = nullptr;
	QPushButton *produceButton = nullptr;
	QPushButton *engageButton = nullptr;
	QPushButton *earnButton = nullptr;
	QPushButton *refreshButton = nullptr;

	QStackedWidget *pages = nullptr;
	QWebSocket *signalingSocket = nullptr;
	QNetworkAccessManager *network = nullptr;
	QString signalingParticipantId;
	QString isoRecordingId;
	QString isoRecordingToken;
	qint64 isoRecordingStartedAt = 0;
	bool isoRecordingActive = false;
};
