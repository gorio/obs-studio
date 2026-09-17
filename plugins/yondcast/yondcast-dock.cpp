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

QPushButton *makeFeatureButton(const QString &title, const QString &subtitle, QWidget *parent, bool enabled = false)
{
	auto *button = new QPushButton(parent);
	button->setText(QStringLiteral("%1\n%2").arg(title, subtitle));
	button->setMinimumHeight(58);
	button->setEnabled(enabled);
	return button;
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
	setMinimumWidth(390);

	auto *layout = new QVBoxLayout(this);
	layout->setContentsMargins(14, 14, 14, 14);
	layout->setSpacing(12);

	// Header: Yond Cast is the product. OBS stays in the background as the engine.
	auto *titleRow = new QHBoxLayout();
	auto *titleBlock = new QVBoxLayout();
	auto *title = new QLabel(QStringLiteral("<b style='font-size:22px'>Yond Cast</b>"), this);
	auto *subtitle = new QLabel(QStringLiteral("Convidados, layouts, legendas, comentários e gravação individual."), this);
	subtitle->setWordWrap(true);
	subtitle->setStyleSheet(QStringLiteral("color:#9aa0aa;"));
	titleBlock->addWidget(title);
	titleBlock->addWidget(subtitle);
	titleRow->addLayout(titleBlock, 1);
	auto *readyBadge = new QLabel(QStringLiteral("PRONTO"), this);
	readyBadge->setAlignment(Qt::AlignCenter);
	readyBadge->setStyleSheet(QStringLiteral("QLabel{background:#173b2b;color:#65df99;border:1px solid #2f7650;border-radius:8px;padding:5px 9px;font-weight:700;}"));
	titleRow->addWidget(readyBadge, 0, Qt::AlignTop);
	layout->addLayout(titleRow);

	// Compact output status. We intentionally hide OBS implementation details from the main workflow.
	auto *outputFrame = new QFrame(this);
	outputFrame->setFrameShape(QFrame::StyledPanel);
	auto *outputLayout = new QHBoxLayout(outputFrame);
	outputLayout->setContentsMargins(10, 8, 10, 8);
	streamingStatusLabel = makeStatusRow(QStringLiteral("Ao vivo"), outputFrame);
	recordingStatusLabel = makeStatusRow(QStringLiteral("Master"), outputFrame);
	outputLayout->addWidget(streamingStatusLabel, 1);
	outputLayout->addWidget(recordingStatusLabel, 1);
	layout->addWidget(outputFrame);

	// Core production modules. Guest Engine is active; next modules are intentionally visible as the roadmap.
	auto *modulesTitle = new QLabel(QStringLiteral("<b style='font-size:15px'>Produção</b>"), this);
	layout->addWidget(modulesTitle);
	auto *moduleGrid = new QGridLayout();
	moduleGrid->setHorizontalSpacing(8);
	moduleGrid->setVerticalSpacing(8);
	auto *guestsModule = makeFeatureButton(QStringLiteral("Convidados"), QStringLiteral("WebRTC remoto"), this, true);
	auto *layoutsModule = makeFeatureButton(QStringLiteral("Layouts"), QStringLiteral("composição automática"), this);
	auto *captionsModule = makeFeatureButton(QStringLiteral("Legendas"), QStringLiteral("nome e lower third"), this);
	auto *commentsModule = makeFeatureButton(QStringLiteral("Comentários"), QStringLiteral("mensagens no ar"), this);
	auto *isoModule = makeFeatureButton(QStringLiteral("ISO"), QStringLiteral("gravação individual"), this);
	moduleGrid->addWidget(guestsModule, 0, 0);
	moduleGrid->addWidget(layoutsModule, 0, 1);
	moduleGrid->addWidget(captionsModule, 1, 0);
	moduleGrid->addWidget(commentsModule, 1, 1);
	moduleGrid->addWidget(isoModule, 2, 0, 1, 2);
	layout->addLayout(moduleGrid);

	// Guest Engine V1: this is now the primary dock workflow.
	auto *guestFrame = new QFrame(this);
	guestFrame->setFrameShape(QFrame::StyledPanel);
	auto *guestLayout = new QGridLayout(guestFrame);
	guestLayout->setContentsMargins(12, 12, 12, 12);
	guestLayout->setHorizontalSpacing(8);
	guestLayout->setVerticalSpacing(8);
	auto *guestTitle = new QLabel(QStringLiteral("<b style='font-size:15px'>Convidados</b>"), guestFrame);
	auto *guestHelp = new QLabel(QStringLiteral("Compartilhe o convite. Depois adicione o participante ao Program como uma fonte independente."), guestFrame);
	guestHelp->setWordWrap(true);
	guestHelp->setStyleSheet(QStringLiteral("color:#9aa0aa;"));
	guestBaseUrlEdit = new QLineEdit(QStringLiteral("https://yondcast.com"), guestFrame);
	guestBaseUrlEdit->setVisible(false);
	guestSessionEdit = new QLineEdit(QUuid::createUuid().toString(QUuid::WithoutBraces), guestFrame);
	guestSessionEdit->setReadOnly(true);
	guestNameEdit = new QLineEdit(guestFrame);
	guestNameEdit->setPlaceholderText(QStringLiteral("Nome usado pelo convidado"));
	guestOpenButton = new QPushButton(QStringLiteral("Abrir convite"), guestFrame);
	guestCopyButton = new QPushButton(QStringLiteral("Copiar link"), guestFrame);
	guestAddSourceButton = new QPushButton(QStringLiteral("Adicionar ao Program"), guestFrame);
	guestAddSourceButton->setMinimumHeight(38);
	guestStatusLabel = new QLabel(QStringLiteral("Aguardando convidado."), guestFrame);
	guestStatusLabel->setWordWrap(true);
	guestStatusLabel->setTextFormat(Qt::RichText);
	guestStatusLabel->setStyleSheet(QStringLiteral("color:#9aa0aa;"));

	guestLayout->addWidget(guestTitle, 0, 0, 1, 2);
	guestLayout->addWidget(guestHelp, 1, 0, 1, 2);
	guestLayout->addWidget(new QLabel(QStringLiteral("Sessão"), guestFrame), 2, 0);
	guestLayout->addWidget(guestSessionEdit, 2, 1);
	guestLayout->addWidget(new QLabel(QStringLiteral("Convidado"), guestFrame), 3, 0);
	guestLayout->addWidget(guestNameEdit, 3, 1);
	guestLayout->addWidget(guestOpenButton, 4, 0);
	guestLayout->addWidget(guestCopyButton, 4, 1);
	guestLayout->addWidget(guestAddSourceButton, 5, 0, 1, 2);
	guestLayout->addWidget(guestStatusLabel, 6, 0, 1, 2);
	layout->addWidget(guestFrame);

	connect(guestOpenButton, &QPushButton::clicked, this, &YondCastDock::openGuestInvite);
	connect(guestCopyButton, &QPushButton::clicked, this, &YondCastDock::copyGuestInvite);
	connect(guestAddSourceButton, &QPushButton::clicked, this, &YondCastDock::addGuestSource);

	// Minimal transport controls at the bottom. OBS-specific scene/profile controls are deliberately hidden.
	auto *actions = new QHBoxLayout();
	streamButton = new QPushButton(this);
	recordButton = new QPushButton(this);
	streamButton->setMinimumHeight(38);
	recordButton->setMinimumHeight(38);
	actions->addWidget(streamButton, 1);
	actions->addWidget(recordButton, 1);
	layout->addLayout(actions);
	connect(streamButton, &QPushButton::clicked, this, &YondCastDock::toggleStreaming);
	connect(recordButton, &QPushButton::clicked, this, &YondCastDock::toggleRecording);
	layout->addStretch(1);

	// Compatibility objects kept off-screen while older event wiring still calls refreshStatus/updateSceneControls.
	engineStatusLabel = makeStatusRow(QStringLiteral("Motor OBS"), this);
	studioModeLabel = makeStatusRow(QStringLiteral("Modo estúdio"), this);
	profileLabel = makeMetaLabel(QStringLiteral("Perfil"), this);
	collectionLabel = makeMetaLabel(QStringLiteral("Coleção"), this);
	sceneLabel = makeMetaLabel(QStringLiteral("Cena"), this);
	programSceneCombo = new QComboBox(this);
	previewSceneCombo = new QComboBox(this);
	transitionButton = new QPushButton(this);
	studioModeButton = new QPushButton(this);
	refreshButton = new QPushButton(this);
	produceButton = new QPushButton(this);
	engageButton = new QPushButton(this);
	earnButton = new QPushButton(this);
	pages = new QStackedWidget(this);
	for (auto *w : {static_cast<QWidget *>(engineStatusLabel), static_cast<QWidget *>(studioModeLabel), static_cast<QWidget *>(profileLabel),
			 static_cast<QWidget *>(collectionLabel), static_cast<QWidget *>(sceneLabel), static_cast<QWidget *>(programSceneCombo),
			 static_cast<QWidget *>(previewSceneCombo), static_cast<QWidget *>(transitionButton), static_cast<QWidget *>(studioModeButton),
			 static_cast<QWidget *>(refreshButton), static_cast<QWidget *>(produceButton), static_cast<QWidget *>(engageButton),
			 static_cast<QWidget *>(earnButton), static_cast<QWidget *>(pages)})
		w->hide();

	connect(programSceneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &YondCastDock::selectProgramScene);
	connect(previewSceneCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &YondCastDock::selectPreviewScene);
	connect(transitionButton, &QPushButton::clicked, this, &YondCastDock::triggerTransition);
	connect(studioModeButton, &QPushButton::clicked, this, &YondCastDock::toggleStudioMode);
	connect(refreshButton, &QPushButton::clicked, this, &YondCastDock::refreshStatus);
	connect(produceButton, &QPushButton::clicked, this, &YondCastDock::showProducePage);
	connect(engageButton, &QPushButton::clicked, this, &YondCastDock::showEngagePage);
	connect(earnButton, &QPushButton::clicked, this, &YondCastDock::showEarnPage);

	refreshStatus();
}

QString YondCastDock::guestInviteUrl() const
{
	QUrl url(normalizedBaseUrl(guestBaseUrlEdit ? guestBaseUrlEdit->text() : QStringLiteral("https://yondcast.com")) + QStringLiteral("/"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("session"), guestSessionEdit ? guestSessionEdit->text().trimmed() : QString());
	query.addQueryItem(QStringLiteral("role"), QStringLiteral("guest"));
	url.setQuery(query);
	return url.toString(QUrl::FullyEncoded);
}

QString YondCastDock::guestBridgeUrl() const
{
	QUrl url(normalizedBaseUrl(guestBaseUrlEdit ? guestBaseUrlEdit->text() : QStringLiteral("https://yondcast.com")) + QStringLiteral("/obs-guest.html"));
	QUrlQuery query;
	query.addQueryItem(QStringLiteral("session"), guestSessionEdit ? guestSessionEdit->text().trimmed() : QString());
	query.addQueryItem(QStringLiteral("guest"), guestNameEdit ? guestNameEdit->text().trimmed() : QString());
	query.addQueryItem(QStringLiteral("fit"), QStringLiteral("cover"));
	url.setQuery(query);
	return url.toString(QUrl::FullyEncoded);
}

void YondCastDock::openGuestInvite() { QDesktopServices::openUrl(QUrl(guestInviteUrl())); }

void YondCastDock::copyGuestInvite()
{
	QApplication::clipboard()->setText(guestInviteUrl());
	if (guestStatusLabel)
		guestStatusLabel->setText(QStringLiteral("<span style='color:#54d48a'>Link copiado.</span> Envie para o convidado."));
}

void YondCastDock::addGuestSource()
{
	const QString guest = guestNameEdit ? guestNameEdit->text().trimmed() : QString();
	if (guest.isEmpty()) {
		if (guestStatusLabel)
			guestStatusLabel->setText(QStringLiteral("<span style='color:#e56b6b'>Informe o mesmo nome usado pelo convidado.</span>"));
		return;
	}
	obs_source_t *current = obs_frontend_get_current_scene();
	if (!current)
		return;
	obs_scene_t *scene = obs_scene_from_source(current);
	if (!scene) {
		obs_source_release(current);
		return;
	}

	const QString sourceLabel = QStringLiteral("Yond Cast · %1").arg(guest);
	const QByteArray sourceLabelUtf8 = sourceLabel.toUtf8();
	const QByteArray bridgeUtf8 = guestBridgeUrl().toUtf8();
	obs_video_info ovi = {};
	obs_get_video_info(&ovi);
	obs_data_t *settings = obs_data_create();
	obs_data_set_string(settings, "url", bridgeUtf8.constData());
	obs_data_set_int(settings, "width", ovi.base_width > 0 ? ovi.base_width : 1920);
	obs_data_set_int(settings, "height", ovi.base_height > 0 ? ovi.base_height : 1080);
	obs_data_set_bool(settings, "reroute_audio", true);
	obs_data_set_bool(settings, "restart_when_active", false);

	obs_source_t *source = obs_get_source_by_name(sourceLabelUtf8.constData());
	if (source) {
		obs_source_update(source, settings);
		obs_sceneitem_t *item = obs_scene_find_source(scene, sourceLabelUtf8.constData());
		if (item)
			obs_sceneitem_set_visible(item, true);
	} else {
		source = obs_source_create("browser_source", sourceLabelUtf8.constData(), settings, nullptr);
		if (source)
			obs_scene_add(scene, source);
	}
	obs_data_release(settings);
	if (source)
		obs_source_release(source);
	obs_source_release(current);
	if (guestStatusLabel)
		guestStatusLabel->setText(QStringLiteral("<span style='color:#54d48a'>No Program:</span> %1").arg(sourceLabel.toHtmlEscaped()));
}

void YondCastDock::setStreamingActive(bool active)
{
	if (streamingStatusLabel)
		streamingStatusLabel->setText(active ? QStringLiteral("<b>Ao vivo</b><br><span style='color:#54d48a'>Transmitindo</span>")
							 : QStringLiteral("<b>Ao vivo</b><br><span style='color:#9aa0aa'>Parado</span>"));
	updateQuickActions();
}

void YondCastDock::setRecordingActive(bool active)
{
	if (recordingStatusLabel)
		recordingStatusLabel->setText(active ? QStringLiteral("<b>Master</b><br><span style='color:#54d48a'>Gravando</span>")
							 : QStringLiteral("<b>Master</b><br><span style='color:#9aa0aa'>Parado</span>"));
	updateQuickActions();
}

void YondCastDock::refreshStatus()
{
	if (engineStatusLabel)
		engineStatusLabel->setText(QStringLiteral("<b>Motor OBS</b><br><span style='color:#54d48a'>Pronto</span>"));
	setStreamingActive(obs_frontend_streaming_active());
	setRecordingActive(obs_frontend_recording_active());
	const bool studioMode = obs_frontend_preview_program_mode_active();
	if (studioModeLabel)
		studioModeLabel->setText(studioMode ? QStringLiteral("<b>Modo estúdio</b><br><span style='color:#54d48a'>Ativo</span>")
							 : QStringLiteral("<b>Modo estúdio</b><br><span style='color:#9aa0aa'>Desativado</span>"));
	if (profileLabel)
		profileLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Perfil</span><br><b>%1</b>").arg(currentProfileName().toHtmlEscaped()));
	if (collectionLabel)
		collectionLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Coleção</span><br><b>%1</b>").arg(currentCollectionName().toHtmlEscaped()));
	if (sceneLabel)
		sceneLabel->setText(QStringLiteral("<span style='color:#9aa0aa'>Cena</span><br><b>%1</b>").arg(currentSceneName().toHtmlEscaped()));
	updateQuickActions();
	updateSceneControls();
}

void YondCastDock::updateSceneControls()
{
	if (!programSceneCombo || !previewSceneCombo || !transitionButton)
		return;
	QSignalBlocker a(programSceneCombo), b(previewSceneCombo);
	programSceneCombo->clear();
	previewSceneCombo->clear();
	char **names = obs_frontend_get_scene_names();
	if (names) {
		for (size_t i = 0; names[i]; ++i) {
			const QString n = QString::fromUtf8(names[i]);
			programSceneCombo->addItem(n);
			previewSceneCombo->addItem(n);
		}
		bfree(names);
	}
	programSceneCombo->setCurrentText(currentSceneName());
	previewSceneCombo->setCurrentText(currentPreviewSceneName());
	const bool studioMode = obs_frontend_preview_program_mode_active();
	previewSceneCombo->setEnabled(studioMode);
	transitionButton->setEnabled(studioMode && previewSceneCombo->count() > 0);
}

void YondCastDock::selectProgramScene(int index)
{
	if (index < 0 || !programSceneCombo)
		return;
	obs_source_t *scene = sceneByName(programSceneCombo->itemText(index));
	if (scene) {
		obs_frontend_set_current_scene(scene);
		obs_source_release(scene);
	}
}

void YondCastDock::selectPreviewScene(int index)
{
	if (index < 0 || !previewSceneCombo || !obs_frontend_preview_program_mode_active())
		return;
	obs_source_t *scene = sceneByName(previewSceneCombo->itemText(index));
	if (scene) {
		obs_frontend_set_current_preview_scene(scene);
		obs_source_release(scene);
	}
}

void YondCastDock::triggerTransition()
{
	if (obs_frontend_preview_program_mode_active())
		obs_frontend_preview_program_trigger_transition();
}

void YondCastDock::toggleStreaming()
{
	obs_frontend_streaming_active() ? obs_frontend_streaming_stop() : obs_frontend_streaming_start();
	updateQuickActions();
}

void YondCastDock::toggleRecording()
{
	obs_frontend_recording_active() ? obs_frontend_recording_stop() : obs_frontend_recording_start();
	updateQuickActions();
}

void YondCastDock::toggleStudioMode()
{
	obs_frontend_set_preview_program_mode(!obs_frontend_preview_program_mode_active());
	refreshStatus();
}

void YondCastDock::updateQuickActions()
{
	if (streamButton)
		streamButton->setText(obs_frontend_streaming_active() ? QStringLiteral("Encerrar transmissão") : QStringLiteral("Transmitir"));
	if (recordButton)
		recordButton->setText(obs_frontend_recording_active() ? QStringLiteral("Parar master") : QStringLiteral("Gravar master"));
	if (studioModeButton)
		studioModeButton->setText(obs_frontend_preview_program_mode_active() ? QStringLiteral("Desativar estúdio") : QStringLiteral("Ativar modo estúdio"));
}

void YondCastDock::setPage(int index)
{
	if (pages && pages->count() > index)
		pages->setCurrentIndex(index);
	if (produceButton)
		produceButton->setChecked(index == 0);
	if (engageButton)
		engageButton->setChecked(index == 1);
	if (earnButton)
		earnButton->setChecked(index == 2);
}

void YondCastDock::showProducePage() { setPage(0); }
void YondCastDock::showEngagePage() { setPage(1); }
void YondCastDock::showEarnPage() { setPage(2); }
