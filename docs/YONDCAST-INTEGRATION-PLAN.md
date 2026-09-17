# Yond Cast on OBS Studio — Integration Plan

## Goal

Use OBS Studio as the native broadcast engine and bring the Yond Cast / Tools 4 Live product concept into the OBS fork without replacing OBS' proven capture, compositing, audio, encoding and output pipeline.

## Source projects

- OBS base: `obsproject/obs-studio` -> fork `gorio/obs-studio`
- Product/feature source: `gorio/tools4live` branch `feat/core-saas-phase1`

At the time this plan was created, `gorio/obs-studio:master` is aligned with `obsproject/obs-studio:master` at commit `caaa0223401f2195128f9998265a0f66d84c9a02`.

## Architectural rule

OBS remains responsible for:

- capture devices and media sources;
- GPU composition and scene rendering;
- audio mixer and monitoring;
- recording;
- encoders;
- RTMP/RTMPS/SRT outputs;
- service/output health and dropped-frame telemetry;
- scene/source persistence.

Yond Cast adds product workflows and creator features around OBS instead of reimplementing the media engine in browser JavaScript.

## Product areas to migrate

The Tools 4 Live catalog is organized around three creator objectives: Produzir, Engajar and Ganhar. The OBS fork should retain that product model while mapping each feature to native OBS concepts.

### 1. Produzir / Studio

Native OBS mapping:

- Sessions / Projects -> scene collections + Yond Cast metadata
- Host / Guests -> Yond Cast participant subsystem + OBS sources
- Camera / microphone -> native OBS sources
- Screen share -> native display/window capture
- Scenes -> native OBS scenes
- Layouts / automatic layouts -> scene-item transforms managed by Yond Cast dock/controller
- Program Output -> native OBS preview/program output
- Media library -> Yond Cast media dock backed by OBS media/image/browser sources
- Logos / brand / lower thirds -> native image/browser/text sources and scene-item templates
- Chat, Poll, Quiz, Word Cloud, Weather, Goal, Ranking, Paid Message -> Yond Cast browser/native overlay sources
- Program recording -> native OBS recording
- Per-participant recording -> isolated-source recording module/plugin phase
- Captions -> caption/transcription service feeding OBS text/browser overlay and export

### 2. Ao vivo / Destinations

Native OBS mapping:

- YouTube, Facebook, Twitch, custom RTMP/RTMPS -> native OBS services/outputs
- SRT -> native OBS output where available
- Multistream -> multiple-output module / plugin architecture
- Assisted resolutions, FPS, encoder presets -> OBS video/output settings surfaced through simplified Yond Cast UI
- Health -> OBS native output stats, dropped frames, congestion and encoder stats

### 3. Interatividade

Keep the existing product concepts and APIs from Tools 4 Live, but render them in OBS through docks and overlay sources:

- Poll
- Quiz
- Word cloud
- Countdown
- QR code
- Public participation page
- Moderation
- Real-time results

### 4. Comunidade

This remains an application/backend feature, exposed inside OBS as docks/panels:

- unique people
- messages
- participant profiles
- history
- ranking
- score
- streak
- levels
- achievements
- supporters / super fans

### 5. Financeiro / Apoios

This remains backend-driven and is exposed as OBS docks plus scene overlays:

- Pix/manual support
- multiple receiving accounts
- Mercado Pago and future providers
- goals
- supporter rankings
- paid messages
- TTS
- unified revenue dashboard

### 6. Integrações

Workspace-level connected accounts remain in the Yond Cast backend. OBS consumes the account capabilities through authenticated API calls.

Initial priorities:

- YouTube
- custom RTMP/RTMPS
- Mercado Pago
- future Facebook/TikTok/Twitch/Instagram as supported

### 7. Meteorologia

Expose existing Yond Cast weather APIs in a dock and native/browser overlay source templates.

### 8. Automações

Keep the current automation engine server-side and add OBS actions as targets, for example:

- change scene
- show/hide source
- play media
- trigger lower third
- trigger TTS
- update goal/overlay

## Proposed code structure

Prefer an isolated Yond Cast module/plugin layer to minimize upstream merge pain.

```text
plugins/
  yondcast/
    CMakeLists.txt
    yondcast-plugin.cpp
    api/
    auth/
    docks/
    overlays/
    participants/
    automation/
    integrations/
    telemetry/
```

Where possible, use OBS public frontend/libobs APIs instead of modifying core OBS files.

Only patch OBS core when a required capability is impossible through the plugin/frontend API.

## Migration phases

### Phase 0 — Foundation

- Keep fork synchronized with upstream OBS.
- Create `feat/yondcast-integration` branch.
- Add Yond Cast plugin/module skeleton.
- Add product architecture docs.
- Establish build on Linux first.

### Phase 1 — Yond Cast shell inside OBS

- Branding layer
- Login/workspace/session API client
- Main Yond Cast dock
- Integrations dock
- Session/project selection
- Basic API health/authentication

### Phase 2 — Production workflows

- Guest/participant model
- Layout controller mapped to OBS scene items
- Media library
- overlays and lower thirds
- captions
- program/participant recording workflows

### Phase 3 — Engagement

- chat
- poll
- quiz
- word cloud
- weather
- goals
- rankings
- paid-message overlays

### Phase 4 — Revenue / Community

- financial dashboard
- supporters
- TTS
- community profiles/ranking/history

### Phase 5 — Destinations / Automation

- destination manager
- multistream
- automation actions targeting OBS
- advanced health/telemetry

## Upstream strategy

Configure local clones with:

```bash
git remote add upstream https://github.com/obsproject/obs-studio.git
git fetch upstream
git checkout master
git merge --ff-only upstream/master
git push origin master
```

Feature development stays off `master` on Yond Cast branches.

## Immediate next implementation

1. Add `plugins/yondcast` skeleton to the OBS build.
2. Create a minimal Yond Cast dock using the OBS frontend API.
3. Add authenticated connection to the existing Yond Cast Core API.
4. Load workspace/session information.
5. Then migrate features one vertical at a time rather than copying the browser media pipeline.
