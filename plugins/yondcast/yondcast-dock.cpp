#include "yondcast-dock.hpp"

#include <obs-frontend-api.h>
#include <obs.h>
#include <util/bmem.h>

#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QStackedWidget>
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

QLabel *makeMetaLabel(const QString &title, QWidget *parent)
{
	auto *label = new QLabel(parent);
	label->setTextFormat(Qt::RichText);
	label->setText(QStringLiteral("<span style='color:#9aa0aa'>%1</span><br><b>—</b>").arg(title));
	label->setWordWrap(true);
	return label;
}

QPushButton *makeFeatureButton(const QString &title, const QString &subtitle, QWidget *parent)
{
	auto *button = new QPushButton(parent);
	button->setText(QStringLiteral("%1\n%2").arg(title, subtitle));
	button->setMinimumHeight(54);
	button->setEnabled(false);
	return button;
}

QWidget *makeSectionPage(const QString &heading, const QString &description,
			 const QList<QPair<QString, QString>> &features, QWidget *parent)
{
	auto *page = new QWidget(parent);
	auto *layout = new QVBoxLayout(page);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->setSpacing(8);

	auto *title = new QLabel(QStringLiteral("<b style='font-size:15px'>%1</b>").arg(heading), page);
	auto *subtitle = new QLabel(description, page);
	subtitle->setWordWrap(true);
	subtitle->setStyleSheet(QStringLiteral("color:#9aa0aa;"));

	layout->addWidget(title);
	layout->addWidget(subtitle);

	auto *grid = new QGridLayout();
	grid->setHorizontalSpacing(8);
	grid->setVerticalSpacing(8);
	for (int i = 0; i < features.size(); ++i) {
		auto *button = makeFeatureButton(features[i].first, features[i].second, page);
		grid->addWidget(button, i / 2, i % 2);
	}
	layout->addLayout(grid);
	layout->addStretch(1);
	return page;
}

QString currentSceneName()
{
	obs_source_t *scene = obs_frontend_get_current_scene();
	if (!scene)
		return QStringLiteral("Sem cena");

	const char *name = obs_source_get_name(scene);
	const QString result = name ? QString::fromUtf8(name) : QStringLiteral("Sem cena");
	obs_source_release(scene);
	return result;
}

QString currentProfileName()
{
	char *profile = obs_frontend_get_current_profile();
	if (!profile)
		return QStringLiteral("—");
	const QString result = QString::fromUtf8(profile);
	bfree(profile);
	return result;
}

QString currentCollectionName()
{
	char *collection = obs_frontend_get_current_scene_collection();
	if (!collection)
		return QStringLiteral("—");
	const QString result = QString::fromUtf8(collection);
	bfree(collection);
	return result;
}
} // namespace

YondCastDock::YondCastDock(QWidget *parent) : QWidget(parent)
{
	setObjectName(QStringLiteral("YondCastDock"));
	setMinimumWidth(320);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(10);

	auto *titleRow = new QHBoxLayout();
	auto *titleBlock = new QVBoxLayout();
	auto *title = new QLabel(QStringLiteral("<b style='font-size:20px'>Yond Cast</b>"), this);
	auto *subtitle = new QLabel(QStringLiteral("Produção ao vivo com o motor nativo do OBS Studio."), this);
	subtitle->setWordWrap(true);
	subtitle->setStyleSheet(QStringLiteral("color:#9aa0aa;"));
	titleBlock->addWidget(title);
	titleBlock->addWidget(subtitle);
	titleRow->addLayout(titleBlock, 1);

	auto *liveBadge = new QLabel(QStringLiteral("NATIVE OBS"), this);
	liveBadge->setAlignment(Qt::AlignCenter);
	liveBadge->setStyleSheet(QStringLiteral(
		"QLabel{background:#29233f;color:#bfa8ff;border:1px solid #544681;border-radius:8px;padding:5px 8px;font-weight:700;}"));
	titleRow->addWidget(liveBadge, 0, Qt::AlignTop);
	layout->addLayout(titleRow);

	auto *separator = new QFrame(this);
	separator->setFrameShape(QFrame::HLine);
	separator->setFrameShadow(QFrame::Sunken);
	layout->addWidget(separator);

	engineStatusLabel = makeStatusRow(QStringLiteral("Motor OBS"), this);
	streamingStatusLabel = makeStatusRow(QStringLiteral("Transmissão"), this);
	recordingStatusLabel = makeStatusRow(QStringLiteral("Gravação"), this);

	auto *statusGrid = new QGridLayout();
	statusGrid->setHorizontalSpacing(12);
	statusGrid->setVerticalSpacing(8);
	statusGrid->addWidget(engineStatusLabel, 0, 0);
	statusGrid->addWidget(streamingStatusLabel, 0, 1);
	statusGrid->addWidget(recordingStatusLabel, 1, 0);
	studioModeLabel = makeStatusRow(QStringLiteral("Modo estúdio"), this);
	statusGrid->addWidget(studioModeLabel, 1, 1);
	layout->addLayout(statusGrid);

	auto *contextFrame = new QFrame(this);
	contextFrame->setFrameShape(QFrame::StyledPanel);
	auto *contextLayout = new QGridLayout(contextFrame);
	contextLayout->setContentsMargins(10, 10, 10, 10);
	contextLayout->setSpacing(8);
	profileLabel = makeMetaLabel(QStringLiteral("Perfil"), contextFrame);
	collectionLabel = makeMetaLabel(QStringLiteral("Coleção de cenas"), contextFrame);
	sceneLabel = makeMetaLabel(QStringLiteral("Cena atual"), contextFrame);
	contextLayout->addWidget(profileLabel, 0, 0);
	contextLayout->addWidget(collectionLabel, 0, 1);
	contextLayout->addWidget(sceneLabel, 1, 0, 1, 2);
	layout->addWidget(contextFrame);

	auto *quickTitle = new QLabel(QStringLiteral("<b>Ações rápidas</b>"), this);
	layout->addWidget(quickTitle);

	auto *quickGrid = new QGridLayout();
	quickGrid->setHorizontalSpacing(8);
	quickGrid->setVerticalSpacing(8);
	streamButton = new QPushButton(this);
	recordButton = new QPushButton(this);
	studioModeButton = new QPushButton(this);
	refreshButton = new QPushButton(QStringLiteral("Atualizar"), this);
	quickGrid->addWidget(streamButton, 0, 0);
	quickGrid->addWidget(recordButton, 0, 1);
	quickGrid->addWidget(studioModeButton, 1, 0);
	quickGrid->addWidget(refreshButton, 1, 1);
	layout->addLayout(quickGrid);

	connect(streamButton, &QPushButton::clicked, this, &YondCastDock::toggleStreaming);
	connect(recordButton, &QPushButton::clicked, this, &YondCastDock::toggleRecording);
	connect(studioModeButton, &QPushButton::clicked, this, &YondCastDock::toggleStudioMode);
	connect(refreshButton, &QPushButton::clicked, this, &YondCastDock::refreshStatus);

	auto *nav = new QHBoxLayout();
	produceButton = new QPushButton(QStringLiteral("Produzir"), this);
	engageButton = new QPushButton(QStringLiteral("Engajar"), this);
	earnButton = new QPushButton(QStringLiteral("Ganhar"), this);
	produceButton->setCheckable(true);
	engageButton->setCheckable(true);
	earnButton->setCheckable(true);
	nav->addWidget(produceButton);
	nav->addWidget(engageButton);
	nav->addWidget(earnButton);
	layout->addLayout(nav);

	connect(produceButton, &QPushButton::clicked, this, &YondCastDock::showProducePage);
	connect(engageButton, &QPushButton::clicked, this, &YondCastDock::showEngagePage);
	connect(earnButton, &QPushButton::clicked, this, &YondCastDock::showEarnPage);

	pages = new QStackedWidget(this);
	pages->addWidget(makeSectionPage(
		QStringLiteral("Produzir"), QStringLiteral("Controles de produção que serão ligados diretamente ao libobs."),
		{{QStringLiteral("Convidados"), QStringLiteral("participantes remotos")},
		 {QStringLiteral("Layouts"), QStringLiteral("cenas e composição")},
		 {QStringLiteral("Mídia"), QStringLiteral("vídeos, imagens e áudio")},
		 {QStringLiteral("Identidade"), QStringLiteral("logos e lower thirds")}},
		pages));
	pages->addWidget(makeSectionPage(
		QStringLiteral("Engajar"), QStringLiteral("Recursos de interação do Yond Cast durante a transmissão."),
		{{QStringLiteral("Chat"), QStringLiteral("YouTube e destinos")},
		 {QStringLiteral("Enquetes"), QStringLiteral("interação em tempo real")},
		 {QStringLiteral("Quiz"), QStringLiteral("perguntas e respostas")},
		 {QStringLiteral("Nuvem"), QStringLiteral("palavras do público")}},
		pages));
	pages->addWidget(makeSectionPage(
		QStringLiteral("Ganhar"), QStringLiteral("Monetização, comunidade e automações do ecossistema Yond Cast."),
		{{QStringLiteral("Apoios"), QStringLiteral("PIX e contribuições")},
		 {QStringLiteral("Meta"), QStringLiteral("objetivos ao vivo")},
		 {QStringLiteral("Ranking"), QStringLiteral("comunidade")},
		 {QStringLiteral("TTS"), QStringLiteral("mensagens por voz")}},
		pages));
	layout->addWidget(pages, 1);

	setPage(0);
	refreshStatus();
}

void YondCastDock::setStreamingActive(bool active)
{
	streamingStatusLabel->setText(
		active ? QStringLiteral("<b>Transmissão</b><br><span style='color:#54d48a'>AO VIVO</span>")
		       : QStringLiteral("<b>Transmissão</b><br><span style='color:#9aa0aa'>Parada</span>"));
	updateQuickActions();
}

void YondCastDock::setRecordingActive(bool active)
{
	recordingStatusLabel->setText(
		active ? QStringLiteral("<b>Gravação</b><br><span style='color:#54d48a'>Gravando</span>")
		       : QStringLiteral("<b>Gravação</b><br><span style='color:#9aa0aa'>Parada</span>"));
	updateQuickActions();
}

void YondCastDock::refreshStatus()
{
	engineStatusLabel->setText(QStringLiteral("<b>Motor OBS</b><br><span style='color:#54d48a'>Pronto</span>"));
	setStreamingActive(obs_frontend_streaming_active());
	setRecordingActive(obs_frontend_recording_active());

	const bool studioMode = obs_frontend_preview_program_mode_active();
	studioModeLabel->setText(
		studioMode ? QStringLiteral("<b>Modo estúdio</b><br><span style='color:#54d48a'>Ativo</span>")
		           : QStringLiteral("<b>Modo estúdio</b><br><span style='color:#9aa0aa'>Desativado</span>"));

	profileLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Perfil</span><br><b>%1</b>").arg(currentProfileName().toHtmlEscaped()));
	collectionLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Coleção de cenas</span><br><b>%1</b>")
				 .arg(currentCollectionName().toHtmlEscaped()));
	sceneLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Cena atual</span><br><b>%1</b>").arg(currentSceneName().toHtmlEscaped()));
	updateQuickActions();
}

void YondCastDock::toggleStreaming()
{
	if (obs_frontend_streaming_active())
		obs_frontend_streaming_stop();
	else
		obs_frontend_streaming_start();
	updateQuickActions();
}

void YondCastDock::toggleRecording()
{
	if (obs_frontend_recording_active())
		obs_frontend_recording_stop();
	else
		obs_frontend_recording_start();
	updateQuickActions();
}

void YondCastDock::toggleStudioMode()
{
	obs_frontend_set_preview_program_mode(!obs_frontend_preview_program_mode_active());
	refreshStatus();
}

void YondCastDock::updateQuickActions()
{
	if (!streamButton || !recordButton || !studioModeButton)
		return;

	const bool streaming = obs_frontend_streaming_active();
	const bool recording = obs_frontend_recording_active();
	const bool studioMode = obs_frontend_preview_program_mode_active();

	streamButton->setText(streaming ? QStringLiteral("Encerrar transmissão") : QStringLiteral("Iniciar transmissão"));
	recordButton->setText(recording ? QStringLiteral("Parar gravação") : QStringLiteral("Iniciar gravação"));
	studioModeButton->setText(studioMode ? QStringLiteral("Desativar estúdio") : QStringLiteral("Ativar modo estúdio"));
}

void YondCastDock::setPage(int index)
{
	if (!pages)
		return;
	pages->setCurrentIndex(index);
	produceButton->setChecked(index == 0);
	engageButton->setChecked(index == 1);
	earnButton->setChecked(index == 2);
}

void YondCastDock::showProducePage()
{
	setPage(0);
}

void YondCastDock::showEngagePage()
{
	setPage(1);
}

void YondCastDock::showEarnPage()
{
	setPage(2);
}
