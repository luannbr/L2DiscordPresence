# L2DiscordPresence

Discord Rich Presence DLL for **Lineage 2** clients, with support for custom text, art assets, timestamps, and **profile buttons**. Single self-contained DLL — talks to the Discord desktop client directly via named-pipe IPC, no Game SDK required.

[Português](#portugues) · [English](#english)

This is a fork of [zikdoz/L2DiscordPresence](https://github.com/zikdoz/L2DiscordPresence) reworked to use Discord IPC instead of the deprecated Game SDK. The buttons feature is exclusive to IPC.

---

<a name="portugues"></a>

## Português

### O que é

DLL injetada no cliente do Lineage 2 (via `ogg.dll` stub) que mostra Rich Presence no Discord. Personaliza:

- `details` e `state` (duas linhas de texto)
- imagem grande e pequena (assets)
- tooltip ao passar o mouse nos assets
- timer "tempo decorrido"
- **até 2 botões** com link (ex: site do servidor, Discord)

Tudo configurado num único arquivo `presence.ini` — `build.bat` aplica nas constantes do `main.cpp` e compila.

### Pré-requisitos

1. **Visual Studio 2022** com o workload **"Desenvolvimento para desktop com C++"** (inclui MSBuild, MSVC v143, Windows 10/11 SDK).
2. App registrado no [Discord Developer Portal](https://discord.com/developers/applications).
3. Discord desktop **aberto** ao iniciar o L2 (a DLL aguarda até 10 min, mas mais simples se estiver aberto).
4. Em `Discord → Configurações → Privacidade da atividade → Compartilhar atividade do meu jogo` precisa estar ligado.

### Passo 1 — criar o app no Discord

1. Acesse [https://discord.com/developers/applications](https://discord.com/developers/applications).
2. Clique em **New Application**, dê um nome (ex: `MeuServerL2`). Esse nome aparece como "Jogando *Nome*" no Discord.
3. Anote o **Application ID** (numérico, ~19 dígitos) — é o seu `APP_ID`.
4. Aba **Rich Presence → Art Assets**: faça upload das imagens (mínimo 512×512). Anote a chave de cada uma (ex: `logo_big`, `bow_small`). *Pode levar ~5 min pra propagar.*
5. Aba **OAuth2**: opcional para Rich Presence, não precisa configurar nada aqui.

### Passo 2 — clonar e configurar

```bash
git clone https://github.com/luannbr/L2DiscordPresence.git
cd L2DiscordPresence
```

Edite `presence.ini`:

```ini
APP_ID=SEU_ID_AQUI

details=Caçando em Cruma
state=meuserver.com

large_image=logo_big
large_text=Meu Server L2
small_image=bow_small
small_text=Archer

button1_label=Site
button1_url=https://meuserver.com
button2_label=Discord
button2_url=https://discord.gg/xxxxxx
```

Regras:
- `APP_ID` precisa ser numérico.
- Cada campo é **1 linha** (limite ~127 caracteres).
- Para esconder um botão, deixe `label` ou `url` em branco.
- Botões precisam de URL `https://` válida.
- **Botões só aparecem para outras pessoas vendo seu perfil**, nunca para você mesmo.

### Passo 3 — compilar

Duplo-clique em `build.bat`. Ele:
1. Aplica os valores do `presence.ini` no `main.cpp`.
2. Compila Release|x86 com o MSBuild do VS 2022.

A DLL sai em:
```
bin\VS_L2DiscordPresence_Release-Win32\L2DiscordPresence.dll
```

### Passo 4 — instalar no L2

Copie estes 2 arquivos para a pasta `system\` do seu cliente Lineage 2 (faça backup do `ogg.dll` original antes!):

| Origem | Destino |
|---|---|
| `bin\VS_L2DiscordPresence_Release-Win32\L2DiscordPresence.dll` | `L2\system\L2DiscordPresence.dll` |
| `ogg.dll` (raiz do repo) | `L2\system\ogg.dll` (substitui) |

> O `ogg.dll` daqui é um stub que importa a função `Anchor` da `L2DiscordPresence.dll`. Quando o L2 carrega o ogg.dll para áudio, o Windows automaticamente carrega a `L2DiscordPresence.dll` junto e dispara o Rich Presence.

Inicie o Discord, depois o L2. Em segundos sua atividade aparece no perfil.

### Solução de problemas

| Sintoma | Causa provável |
|---|---|
| Nada aparece no Discord | Discord não estava aberto / Activity Status desligado / APP_ID errado |
| Aparece só "Jogando *Nome*" sem texto custom | A DLL conectou mas a config está vazia — confira `presence.ini` |
| Imagens não aparecem | Assets recém-criados levam até 5 min para propagar / nome no `.ini` não bate com a chave no Dev Portal |
| Botões não aparecem **pra mim** | Comportamento normal do Discord — peça pra outra conta ver seu perfil |
| Botões não aparecem **pra ninguém** | URL inválida / Discord bloqueia certos domínios |
| Build falha com `MSBuild não encontrado` | Falta o workload C++ no VS 2022 |

### Como funciona (técnico)

1. `ogg.dll` (stub) é carregado pelo L2 → importa `Anchor` da `L2DiscordPresence.dll` → DllMain executa.
2. DllMain cria uma thread que abre `\\.\pipe\discord-ipc-{0..9}` (named pipe local do Discord).
3. Handshake `{v:1, client_id:APP_ID}` → Discord responde com `READY`.
4. Envia `SET_ACTIVITY` com JSON contendo details, state, assets, timestamps, **buttons**.
5. Mantém o pipe aberto e responde pings — se fechar, Discord limpa a atividade.

Sem dependência da `discord_game_sdk.dll` — uma única DLL standalone (~26 KB).

### Créditos

- Fork de [zikdoz/L2DiscordPresence](https://github.com/zikdoz/L2DiscordPresence) (Herald Gersdorff) — repositório original.
- Licença: GPLv3 (mantida do projeto original). Veja [`COPYING.txt`](COPYING.txt).

---

<a name="english"></a>

## English

### What it is

A DLL injected into the Lineage 2 client (via an `ogg.dll` stub) that shows Discord Rich Presence. Customizes:

- `details` and `state` (two lines of text)
- large and small image (assets)
- tooltip text when hovering assets
- "time elapsed" timer
- **up to 2 link buttons** (e.g. server site, Discord invite)

Everything is configured in a single `presence.ini` — `build.bat` patches the `main.cpp` constants and compiles.

### Requirements

1. **Visual Studio 2022** with the **"Desktop development with C++"** workload (provides MSBuild, MSVC v143, Windows 10/11 SDK).
2. A registered app at [Discord Developer Portal](https://discord.com/developers/applications).
3. Discord desktop **running** when you start L2 (the DLL retries for ~10 min, but having it open is simpler).
4. In `Discord → Settings → Activity Privacy → Share your activity with others` must be ON.

### Step 1 — create the Discord app

1. Go to [https://discord.com/developers/applications](https://discord.com/developers/applications).
2. Click **New Application**, name it (e.g. `MyL2Server`). This shows as "Playing *Name*" on Discord.
3. Copy the **Application ID** (numeric, ~19 digits) — this is your `APP_ID`.
4. **Rich Presence → Art Assets** tab: upload images (minimum 512×512). Note each one's key (e.g. `logo_big`, `bow_small`). *Can take up to ~5 min to propagate.*
5. **OAuth2** tab: optional for Rich Presence, no setup required.

### Step 2 — clone and configure

```bash
git clone https://github.com/luannbr/L2DiscordPresence.git
cd L2DiscordPresence
```

Edit `presence.ini`:

```ini
APP_ID=YOUR_ID_HERE

details=Hunting in Cruma
state=myserver.com

large_image=logo_big
large_text=My L2 Server
small_image=bow_small
small_text=Archer

button1_label=Website
button1_url=https://myserver.com
button2_label=Discord
button2_url=https://discord.gg/xxxxxx
```

Rules:
- `APP_ID` must be numeric.
- Each field is **a single line** (limit ~127 characters).
- To hide a button, leave its `label` or `url` blank.
- Buttons need a valid `https://` URL.
- **Buttons are only visible to other people viewing your profile**, never to yourself.

### Step 3 — build

Double-click `build.bat`. It will:
1. Apply values from `presence.ini` into `main.cpp`.
2. Compile Release|x86 with MSBuild from VS 2022.

DLL ends up at:
```
bin\VS_L2DiscordPresence_Release-Win32\L2DiscordPresence.dll
```

### Step 4 — install into L2

Copy these 2 files into your Lineage 2 client's `system\` folder (back up the original `ogg.dll` first):

| Source | Destination |
|---|---|
| `bin\VS_L2DiscordPresence_Release-Win32\L2DiscordPresence.dll` | `L2\system\L2DiscordPresence.dll` |
| `ogg.dll` (repo root) | `L2\system\ogg.dll` (overwrite) |

> The included `ogg.dll` is a stub that imports the `Anchor` function from `L2DiscordPresence.dll`. When L2 loads `ogg.dll` for audio, Windows automatically loads `L2DiscordPresence.dll` alongside it and fires the Rich Presence.

Start Discord, then start L2. Your activity appears on your profile within a few seconds.

### Troubleshooting

| Symptom | Likely cause |
|---|---|
| Nothing shows in Discord | Discord wasn't open / Activity Status off / wrong APP_ID |
| Only "Playing *Name*" with no custom text | DLL connected but config is empty — check `presence.ini` |
| Images don't show | New assets take up to 5 min to propagate / asset key in `.ini` doesn't match Dev Portal |
| Buttons don't show **for me** | Expected Discord behavior — ask another account to view your profile |
| Buttons don't show **for anyone** | Invalid URL / blocked domain |
| Build fails with `MSBuild not found` | Missing C++ workload in VS 2022 |

### How it works (technical)

1. `ogg.dll` (stub) is loaded by L2 → it imports `Anchor` from `L2DiscordPresence.dll` → DllMain runs.
2. DllMain spawns a thread that opens `\\.\pipe\discord-ipc-{0..9}` (Discord's local named pipe).
3. Handshake `{v:1, client_id:APP_ID}` → Discord responds with `READY`.
4. Sends `SET_ACTIVITY` with JSON containing details, state, assets, timestamps, **buttons**.
5. Keeps the pipe open and replies to pings — closing it would clear the activity.

No dependency on `discord_game_sdk.dll` — single standalone DLL (~26 KB).

### Credits

- Fork of [zikdoz/L2DiscordPresence](https://github.com/zikdoz/L2DiscordPresence) (Herald Gersdorff) — original repo.
- License: GPLv3 (inherited from upstream). See [`COPYING.txt`](COPYING.txt).
