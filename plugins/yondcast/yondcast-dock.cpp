#include "yondcast-dock.hpp"

#include <obs-frontend-api.h>

#include <QFrame>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

namespace {
QLabel *makeStatusRow(const QString &title, QWidget *parent)
{
	auto *label = new QLabel(parent);
	label->setTextFormat(Qt::RichText);
	label->setText(QStringLiteral("<b>%1</b><br><span style='color:#9aa0aa'>Aguardando...</span>").arg(title));
	label->setWordWrap(true);
	return label;
}
}

YondCastDock::YondCastDock(QWidget *parent) : QWidget(parent)
{
	setObjectName(QStringLiteral("YondCastDock"));

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(10);

	auto *title = new QLabel(QStringLiteral("<b style='font-size:18px'>Yond Cast</b>"), this);
	auto *subtitle = new QLabel(
		QStringLiteral("Camada Yond Cast sobre o motor nativo do OBS Studio."), this);
	subtitle->setWordWrap(true);

	auto *separator = new QFrame(this);
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);

	engineStatusLabel = makeStatusRow(QStringLiteral("Motor OBS"), this);
	streamingStatusLabel = makeStatusRow(QStringLiteral("Transmissão"), this);
	recordingStatusLabel = makeStatusRow(QStringLiteral("Gravação"), this);

	refreshButton = new QPushButton(QStringLiteral("Atualizar status"), this);
	connect(refreshButton, &QPushButton::clicked, this, &YondCastDock::refreshStatus);

	layout->addWidget(title);
	layout->addWidget(subtitle);
	layout->addWidget(separator);
	layout->addWidget(engineStatusLabel);
	layout->addWidget(streamingStatusLabel);
	layout->addWidget(recordingStatusLabel);
	layout->addStretch(1);
	layout->addWidget(refreshButton);

	refreshStatus();
}

void YondCastDock::setStreamingActive(bool active)
{
	streamingStatusLabel->setText(
		active ? QStringLiteral("<b>Transmissão</b><br><span style='color:#54d48a'>AO VIVO pelo OBS</span>")
		       : QStringLiteral("<b>Transmissão</b><br><span style='color:#9aa0aa'>Parada</span>"));
}

void YondCastDock::setRecordingActive(bool active)
{
	recordingStatusLabel->setText(
		active ? QStringLiteral("<b>Gravação</b><br><span style='color:#54d48a'>Gravando</span>")
		       : QStringLiteral("<b>Gravação</b><br><span style='color:#9aa0aa'>Parada</span>"));
}

void YondCastDock::refreshStatus()
{
	engineStatusLabel->setText(
		QStringLiteral("<b>Motor OBS</b><br><span style='color:#54d48a'>Pronto</span>"));
	setStreamingActive(obs_frontend_streaming_active());
	setRecordingActive(obs_frontend_recording_active());
}
