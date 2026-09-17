#include "yondcast-dock.hpp"

#include <obs-frontend-api.h>
#include <obs.h>
#include <util/bmem.h>

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDesktopServices>
#include <QFrame>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QUuid>
#include <QUrl>
#include <QUrlQuery>
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
	for (int i = 0; i < features.size(); ++i)
		grid->addWidget(makeFeatureButton(features[i].first, features[i].second, page), i / 2, i % 2);
	layout->addLayout(grid);
	layout->addStretch(1);
	return page;
}

QString sourceName(obs_source_t *source)
{
	if (!source)
		return QStringLiteral("Sem cena");
	const char *name = obs_source_get_name(source);
	return name ? QString::fromUtf8(name) : QStringLiteral("Sem cena");
}

QString currentSceneName()
{
	obs_source_t *scene = obs_frontend_get_current_scene();
	const QString result = sourceName(scene);
	if (scene)
		obs_source_release(scene);
	return result;
}

QString currentPreviewSceneName()
{
	obs_source_t *scene = obs_frontend_get_current_preview_scene();
	const QString result = sourceName(scene);
	if (scene)
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

obs_source_t *sceneByName(const QString &name)
{
	struct obs_frontend_source_list scenes = {};
	obs_frontend_get_scenes(&scenes);
	obs_source_t *result = nullptr;
	for (size_t i = 0; i < scenes.sources.num; ++i) {
		obs_source_t *source = scenes.sources.array[i];
		const char *sourceName = obs_source_get_name(source);
		if (sourceName && QString::fromUtf8(sourceName) == name) {
			result = obs_source_get_ref(source);
			break;
		}
	}
	obs_frontend_source_list_free(&scenes);
	return result;
}

QString normalizedBaseUrl(QString value)
{
	value = value.trimmed();
	while (value.endsWith('/'))
		value.chop(1);
	return value;
}
} // namespace

YondCastDock::YondCastDock(QWidget *parent) : QWidget(parent)
{
	setObjectName(QStringLiteral("YondCastDock"));
	setMinimumWidth(380);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(12, 12, 12, 12);
	layout->setSpacing(10);

	auto *titleRow = new QHBoxLayout();
	auto *titleBlock = new QVBoxLayout();
	auto *title = new QLabel(QStringLiteral("<b style='font-size:20px'>Yond Cast</b>"), this);
	auto *subtitle = new QLabel(QStringLiteral("Produção Yond Cast com o OBS como motor nativo."), this);
	subtitle->setWordWrap(true);
	subtitle->setStyleSheet(QStringLiteral("color:#9aa0aa;"));
	titleBlock->addWidget(title);
	titleBlock->addWidget(subtitle);
	titleRow->addLayout(titleBlock, 1);

	auto *liveBadge = new QLabel(QStringLiteral("OBS ENGINE"), this);
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
	studioModeLabel = makeStatusRow(QStringLiteral("Modo estúdio"), this);

	auto *statusGrid = new QGridLayout();
	statusGrid->setHorizontalSpacing(12);
	statusGrid->setVerticalSpacing(8);
	statusGrid->addWidget(engineStatusLabel, 0, 0);
	statusGrid->addWidget(streamingStatusLabel, 0, 1);
	statusGrid->addWidget(recordingStatusLabel, 1, 0);
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

	auto *directorFrame = new QFrame(this);
	directorFrame->setFrameShape(QFrame::StyledPanel);
	auto *directorLayout = new QGridLayout(directorFrame);
	directorLayout->setContentsMargins(10, 10, 10, 10);
	directorLayout->setSpacing(8);
	auto *directorTitle = new QLabel(QStringLiteral("<b>Direção de cenas</b>"), directorFrame);
	programSceneCombo = new QComboBox(directorFrame);
	previewSceneCombo = new QComboBox(directorFrame);
	transitionButton = new QPushButton(QStringLiteral("Levar preview ao ar"), directorFrame);
	directorLayout->addWidget(directorTitle, 0, 0, 1, 2);
	directorLayout->addWidget(new QLabel(QStringLiteral("Programa"), directorFrame), 1, 0);
	directorLayout->addWidget(programSceneCombo, 1, 1);
	directorLayout->addWidget(new QLabel(QStringLiteral("Preview"), directorFrame), 2, 0);
	directorLayout->addWidget(previewSceneCombo, 2, 1);
	directorLayout->addWidget(transitionButton, 3, 0, 1, 2);
	layout->addWidget(directorFrame);

	connect(programSceneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&YondCastDock::selectProgramScene);
	connect(previewSceneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this,
		&YondCastDock::selectPreviewScene);
	connect(transitionButton, &QPushButton::clicked, this, &YondCastDock::triggerTransition);

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

	// PRODUZIR: Guest Engine V1 is functional. Other production modules follow on this same native layer.
	auto *producePage = new QWidget(pages);
	auto *produceLayout = new QVBoxLayout(producePage);
	produceLayout->setContentsMargins(0, 0, 0, 0);
	produceLayout->setSpacing(8);
	auto *produceTitle = new QLabel(QStringLiteral("<b style='font-size:15px'>Produzir</b>"), producePage);
	auto *produceDescription = new QLabel(
		QStringLiteral("Convidados remotos entram por WebRTC e viram fontes independentes do OBS."), producePage);
	produceDescription->setWordWrap(true);
	produceDescription->setStyleSheet(QStringLiteral("color:#9aa0aa;"));
	produceLayout->addWidget(produceTitle);
	produceLayout->addWidget(produceDescription);

	auto *guestFrame = new QFrame(producePage);
	guestFrame->setFrameShape(QFrame::StyledPanel);
	auto *guestLayout = new QGridLayout(guestFrame);
	guestLayout->setContentsMargins(10, 10, 10, 10);
	guestLayout->setSpacing(8);
	auto *guestTitle = new QLabel(QStringLiteral("<b>Convidados · Guest Engine V1</b>"), guestFrame);
	auto *guestHelp = new QLabel(
		QStringLiteral("Crie a sala, envie o link e adicione cada convidado como uma fonte WebRTC limpa no Program."), guestFrame);
	guestHelp->setWordWrap(true);
	guestHelp->setStyleSheet(QStringLiteral("color:#9aa0aa;"));
	guestBaseUrlEdit = new QLineEdit(QStringLiteral("https://yondcast.com"), guestFrame);
	guestSessionEdit = new QLineEdit(QUuid::createUuid().toString(QUuid::WithoutBraces), guestFrame);
	guestNameEdit = new QLineEdit(guestFrame);
	guestNameEdit->setPlaceholderText(QStringLiteral("Nome exato usado pelo convidado"));
	guestOpenButton = new QPushButton(QStringLiteral("Abrir link do convidado"), guestFrame);
	guestCopyButton = new QPushButton(QStringLiteral("Copiar convite"), guestFrame);
	guestAddSourceButton = new QPushButton(QStringLiteral("Adicionar convidado ao OBS"), guestFrame);
	guestStatusLabel = new QLabel(QStringLiteral("Pronto para criar uma fonte de convidado."), guestFrame);
	guestStatusLabel->setWordWrap(true);
	guestStatusLabel->setStyleSheet(QStringLiteral("color:#9aa0aa;"));

	guestLayout->addWidget(guestTitle, 0, 0, 1, 2);
	guestLayout->addWidget(guestHelp, 1, 0, 1, 2);
	guestLayout->addWidget(new QLabel(QStringLiteral("Servidor Yond Cast"), guestFrame), 2, 0);
	guestLayout->addWidget(guestBaseUrlEdit, 2, 1);
	guestLayout->addWidget(new QLabel(QStringLiteral("Sessão"), guestFrame), 3, 0);
	guestLayout->addWidget(guestSessionEdit, 3, 1);
	guestLayout->addWidget(new QLabel(QStringLiteral("Convidado"), guestFrame), 4, 0);
	guestLayout->addWidget(guestNameEdit, 4, 1);
	guestLayout->addWidget(guestOpenButton, 5, 0);
	guestLayout->addWidget(guestCopyButton, 5, 1);
	guestLayout->addWidget(guestAddSourceButton, 6, 0, 1, 2);
	guestLayout->addWidget(guestStatusLabel, 7, 0, 1, 2);
	produceLayout->addWidget(guestFrame);

	connect(guestOpenButton, &QPushButton::clicked, this, &YondCastDock::openGuestInvite);
	connect(guestCopyButton, &QPushButton::clicked, this, &YondCastDock::copyGuestInvite);
	connect(guestAddSourceButton, &QPushButton::clicked, this, &YondCastDock::addGuestSource);

	auto *futureGrid = new QGridLayout();
	futureGrid->setHorizontalSpacing(8);
	futureGrid->setVerticalSpacing(8);
	futureGrid->addWidget(makeFeatureButton(QStringLiteral("Layouts"), QStringLiteral("composição automática"), producePage), 0, 0);
	futureGrid->addWidget(makeFeatureButton(QStringLiteral("Legendas"), QStringLiteral("nomes e lower thirds"), producePage), 0, 1);
	futureGrid->addWidget(makeFeatureButton(QStringLiteral("Comentários"), QStringLiteral("mensagens no Program"), producePage), 1, 0);
	futureGrid->addWidget(makeFeatureButton(QStringLiteral("ISO"), QStringLiteral("gravação por participante"), producePage), 1, 1);
	produceLayout->addLayout(futureGrid);
	produceLayout->addStretch(1);
	pages->addWidget(producePage);

	pages->addWidget(makeSectionPage(
		QStringLiteral("Engajar"), QStringLiteral("Recursos de interação do Yond Cast durante a transmissão."),
		{{QStringLiteral("Chat"), QStringLiteral("YouTube e destinos")},
		 {QStringLiteral("Enquetes"), QStringLiteral("interação em tempo real")},
		 {QStringLiteral("Quiz"), QStringLiteral("perguntas e respostas")},
		 {QStringLiteral("Nuvem"), QStringLiteral("palavras do público")}}, pages));
	pages->addWidget(makeSectionPage(
		QStringLiteral("Ganhar"), QStringLiteral("Monetização, comunidade e automações do ecossistema Yond Cast."),
		{{QStringLiteral("Apoios"), QStringLiteral("PIX e contribuições")},
		 {QStringLiteral("Meta"), QStringLiteral("objetivos ao vivo")},
		 {QStringLiteral("Ranking"), QStringLiteral("comunidade")},
		 {QStringLiteral("TTS"), QStringLiteral("mensagens por voz")}}, pages));
	layout->addWidget(pages, 1);

	setPage(0);
	refreshStatus();
}

void YondCastDock::setStreamingActive(bool active)
{
	streamingStatusLabel->setText(active
		? QStringLiteral("<b>Transmissão</b><br><span style='color:#54d48a'>AO VIVO</span>")
		: QStringLiteral("<b>Transmissão</b><br><span style='color:#9aa0aa'>Parada</span>"));
	updateQuickActions();
}

void YondCastDock::setRecordingActive(bool active)
{
	recordingStatusLabel->setText(active
		? QStringLiteral("<b>Gravação</b><br><span style='color:#54d48a'>Gravando</span>")
		: QStringLiteral("<b>Gravação</b><br><span style='color:#9aa0aa'>Parada</span>"));
	updateQuickActions();
}

void YondCastDock::refreshStatus()
{
	engineStatusLabel->setText(QStringLiteral("<b>Motor OBS</b><br><span style='color:#54d48a'>Pronto</span>"));
	setStreamingActive(obs_frontend_streaming_active());
	setRecordingActive(obs_frontend_recording_active());

	const bool studioMode = obs_frontend_preview_program_mode_active();
	studioModeLabel->setText(studioMode
		? QStringLiteral("<b>Modo estúdio</b><br><span style='color:#54d48a'>Ativo</span>")
		: QStringLiteral("<b>Modo estúdio</b><br><span style='color:#9aa0aa'>Desativado</span>"));
	profileLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Perfil</span><br><b>%1</b>")
		.arg(currentProfileName().toHtmlEscaped()));
	collectionLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Coleção de cenas</span><br><b>%1</b>")
		.arg(currentCollectionName().toHtmlEscaped()));
	sceneLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Cena atual</span><br><b>%1</b>")
		.arg(currentSceneName().toHtmlEscaped()));
	updateQuickActions();
	updateSceneControls();
}

void YondCastDock::updateSceneControls()
{
	if (!programSceneCombo || !previewSceneCombo || !transitionButton)
		return;

	QSignalBlocker blockProgram(programSceneCombo);
	QSignalBlocker blockPreview(previewSceneCombo);
	programSceneCombo->clear();
	previewSceneCombo->clear();

	char **names = obs_frontend_get_scene_names();
	if (names) {
		for (size_t i = 0; names[i]; ++i) {
			const QString name = QString::fromUtf8(names[i]);
			programSceneCombo->addItem(name);
			previewSceneCombo->addItem(name);
		}
		bfree(names);
	}

	const QString program = currentSceneName();
	const QString preview = currentPreviewSceneName();
	programSceneCombo->setCurrentText(program);
	previewSceneCombo->setCurrentText(preview);

	const bool studioMode = obs_frontend_preview_program_mode_active();
	previewSceneCombo->setEnabled(studioMode);
	transitionButton->setEnabled(studioMode && previewSceneCombo->count() > 0);
	transitionButton->setText(studioMode ? QStringLiteral("Levar preview ao ar")
					 : QStringLiteral("Ative o modo estúdio para usar preview"));
}

void YondCastDock::selectProgramScene(int index)
{
	if (index < 0 || !programSceneCombo)
		return;
	obs_source_t *scene = sceneByName(programSceneCombo->itemText(index));
	if (!scene)
		return;
	obs_frontend_set_current_scene(scene);
	obs_source_release(scene);
}

void YondCastDock::selectPreviewScene(int index)
{
	if (index < 0 || !previewSceneCombo || !obs_frontend_preview_program_mode_active())
		return;
	obs_source_t *scene = sceneByName(previewSceneCombo->itemText(index));
	if (!scene)
		return;
	obs_frontend_set_current_preview_scene(scene);
	obs_source_release(scene);
}

void YondCastDock::triggerTransition()
{
	if (obs_frontend_preview_program_mode_active())
		obs_frontend_preview_program_trigger_transition();
}

QString YondCastDock::guestInviteUrl() const
{
	const QString base = normalizedBaseUrl(guestBaseUrlEdit ? guestBaseUrlEdit->text() : QString());
	if (base.isEmpty() || !guestSessionEdit)
		return {};
	QUrl url(base + QStringLiteral("/"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("session"), guestSessionEdit->text().trimmed());
	query.addQueryItem(QStringLiteral("role"), QStringLiteral("guest"));
	url.setQuery(query);
	return url.toString(QUrl::FullyEncoded);
}

QString YondCastDock::guestBridgeUrl() const
{
	const QString base = normalizedBaseUrl(guestBaseUrlEdit ? guestBaseUrlEdit->text() : QString());
	if (base.isEmpty() || !guestSessionEdit || !guestNameEdit)
		return {};
	QUrl url(base + QStringLiteral("/obs-guest.html"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("session"), guestSessionEdit->text().trimmed());
	query.addQueryItem(QStringLiteral("guest"), guestNameEdit->text().trimmed());
	query.addQueryItem(QStringLiteral("fit"), QStringLiteral("cover"));
	url.setQuery(query);
	return url.toString(QUrl::FullyEncoded);
}

void YondCastDock::openGuestInvite()
{
	const QString url = guestInviteUrl();
	if (url.isEmpty())
		return;
	QDesktopServices::openUrl(QUrl(url));
}

void YondCastDock::copyGuestInvite()
{
	const QString url = guestInviteUrl();
	if (url.isEmpty())
		return;
	QApplication::clipboard()->setText(url);
	if (guestStatusLabel)
		guestStatusLabel->setText(QStringLiteral("Convite copiado. O convidado deve usar o mesmo nome informado abaixo."));
}

void YondCastDock::addGuestSource()
{
	if (!guestNameEdit || !guestSessionEdit || !guestBaseUrlEdit)
		return;
	const QString guestName = guestNameEdit->text().trimmed();
	const QString sessionId = guestSessionEdit->text().trimmed();
	const QString bridgeUrl = guestBridgeUrl();
	if (guestName.isEmpty() || sessionId.isEmpty() || bridgeUrl.isEmpty()) {
		QMessageBox::warning(this, QStringLiteral("Yond Cast"),
			QStringLiteral("Informe servidor, sessão e o nome exato do convidado."));
		return;
	}

	obs_video_info ovi = {};
	obs_get_video_info(&ovi);
	const uint32_t width = ovi.base_width ? ovi.base_width : 1920;
	const uint32_t height = ovi.base_height ? ovi.base_height : 1080;
	const QString sourceDisplayName = QStringLiteral("Yond Cast · %1").arg(guestName);

	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "url", bridgeUrl.toUtf8().constData());
	obs_data_set_int(settings, "width", width);
	obs_data_set_int(settings, "height", height);
	obs_data_set_int(settings, "fps", 30);
	obs_data_set_bool(settings, "reroute_audio", true);
	obs_data_set_bool(settings, "shutdown", false);
	obs_data_set_bool(settings, "restart_when_active", true);

	obs_source_t *source = obs_get_source_by_name(sourceDisplayName.toUtf8().constData());
	if (source) {
		obs_source_update(source, settings);
	} else {
		source = obs_source_create("browser_source", sourceDisplayName.toUtf8().constData(), settings, nullptr);
	}
	obs_data_release(settings);

	if (!source) {
		QMessageBox::critical(this, QStringLiteral("Yond Cast"),
			QStringLiteral("Não foi possível criar a fonte Browser. Confirme que o plugin obs-browser está habilitado."));
		return;
	}

	obs_source_t *sceneSource = obs_frontend_get_current_scene();
	obs_scene_t *scene = sceneSource ? obs_scene_from_source(sceneSource) : nullptr;
	if (!scene) {
		if (sceneSource)
			obs_source_release(sceneSource);
		obs_source_release(source);
		QMessageBox::warning(this, QStringLiteral("Yond Cast"), QStringLiteral("Nenhuma cena ativa para receber o convidado."));
		return;
	}

	const QByteArray sourceNameUtf8 = sourceDisplayName.toUtf8();
	obs_sceneitem_t *existingItem = obs_scene_find_source(scene, sourceNameUtf8.constData());
	if (!existingItem)
		obs_scene_add(scene, source);

	if (sceneSource)
		obs_source_release(sceneSource);
	obs_source_release(source);

	if (guestStatusLabel) {
		guestStatusLabel->setText(QStringLiteral(
			"Fonte criada: <b>%1</b><br>Ela ficará aguardando o convidado <b>%2</b> entrar na sessão.")
			.arg(sourceDisplayName.toHtmlEscaped(), guestName.toHtmlEscaped()));
		guestStatusLabel->setTextFormat(Qt::RichText);
	}
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

void YondCastDock::showProducePage() { setPage(0); }
void YondCastDock::showEngagePage() { setPage(1); }
void YondCastDock::showEarnPage() { setPage(2); }
