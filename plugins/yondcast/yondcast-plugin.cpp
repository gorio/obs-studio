#include "yondcast-dock.hpp"

#include <obs-frontend-api.h>
#include <obs-module.h>

#include <QMetaObject>

OBS_DECLARE_MODULE()
OBS_MODULE_AUTHOR("Yond Cast")

static YondCastDock *g_dock = nullptr;

static void frontend_event(enum obs_frontend_event event, void *)
{
	if (!g_dock)
		return;

	switch (event) {
	case OBS_FRONTEND_EVENT_STREAMING_STARTED:
		QMetaObject::invokeMethod(g_dock, [=]() { g_dock->setStreamingActive(true); }, Qt::QueuedConnection);
		break;
	case OBS_FRONTEND_EVENT_STREAMING_STOPPED:
		QMetaObject::invokeMethod(g_dock, [=]() { g_dock->setStreamingActive(false); }, Qt::QueuedConnection);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STARTED:
		QMetaObject::invokeMethod(g_dock, [=]() { g_dock->setRecordingActive(true); }, Qt::QueuedConnection);
		break;
	case OBS_FRONTEND_EVENT_RECORDING_STOPPED:
		QMetaObject::invokeMethod(g_dock, [=]() { g_dock->setRecordingActive(false); }, Qt::QueuedConnection);
		break;
	case OBS_FRONTEND_EVENT_FINISHED_LOADING:
	case OBS_FRONTEND_EVENT_PROFILE_CHANGED:
	case OBS_FRONTEND_EVENT_SCENE_COLLECTION_CHANGED:
		QMetaObject::invokeMethod(g_dock, &YondCastDock::refreshStatus, Qt::QueuedConnection);
		break;
	default:
		break;
	}
}

bool obs_module_load(void)
{
	g_dock = new YondCastDock();
	if (!obs_frontend_add_dock_by_id("yondcast-control", "Yond Cast", g_dock)) {
		delete g_dock;
		g_dock = nullptr;
		blog(LOG_ERROR, "[Yond Cast] Failed to register dock");
		return false;
	}

	obs_frontend_add_event_callback(frontend_event, nullptr);
	blog(LOG_INFO, "[Yond Cast] Native dock loaded");
	return true;
}

void obs_module_unload(void)
{
	obs_frontend_remove_event_callback(frontend_event, nullptr);
	obs_frontend_remove_dock("yondcast-control");
	g_dock = nullptr;
	blog(LOG_INFO, "[Yond Cast] Native dock unloaded");
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return "Yond Cast product layer for OBS Studio";
}
