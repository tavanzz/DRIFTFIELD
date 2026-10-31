# Driftfield

Plugin di effetti (VST3 / AU / Standalone) ispirato a *Echo Valley*: prende qualsiasi cosa suoni, anche una sinusoide o note a caso, e la trasforma in texture ambient e lo-fi.
È scritto in C++ con [JUCE](https://juce.com) 8.

## I 5 moduli

| Modulo | Cosa fa | Controlli |
|---|---|---|
| **Harmonic** | 8 risonatori "simpatici" (come le corde di un sitar) accordati su un accordo della tonalità scelta: qualunque nota suoni fa risuonare le note *in key*. Più un filtro a formanti che fa "cantare" il suono sulle vocali A‑E‑I‑O‑U. | Key, Scale (Major, Minor, Dorian, Lydian, Pentatonic), Amount, Ring (durata delle risonanze), Vowel, Voice |
| **Mood** | Dà il carattere complessivo: chorus/ensemble, tilt EQ, calore e **shimmer** (una trasposizione reiniettata nel riverbero). | Mood (Dreamy, Dark, Bright, Haunted, Warm), Depth |
| **Diffusion** | Riverbero FDN a 8 linee con modulazione, fino a 20 s di coda. Il tasto **FREEZE** congela la coda all'infinito. | Amount, Size, Decay, Damping |
| **Texture** | Pioggia generata in tempo reale (segue il livello del suono), lo‑fi da nastro (wow/flutter, sample-rate e bit ridotti, passa-basso) e grit (saturazione). | Rain, Lo-Fi, Grit |
| **Delay** | Delay stereo sincronizzato al tempo del progetto, con ping-pong e feedback filtrato. | Time (da 1/16 a 1/2), Ping-Pong, Amount, Feedback, Tone |

Catena del segnale: `Harmonic → Mood → Delay → Diffusion → Texture → Mix/Output`.
Ogni modulo ha il suo pulsante di accensione. Ci sono 12 preset di fabbrica (Rain Window, Glass Choir, Infinite Field, Tape Memories, …).

Trucco: fai doppio clic su una manopola per riportarla al valore di default.

## Come ottenere il plugin compilato

### Opzione A: GitHub Actions (non serve installare niente)
1. Crea un repository su GitHub e carica questa cartella.
2. Alla prima push parte la build in automatico (scheda **Actions**, workflow *Build Driftfield*).
3. Quando la build è finita, scarica gli artifact: **Driftfield-Windows** e/o **Driftfield-macOS**.

### Opzione B: compilare sul tuo computer
Ti servono CMake ≥ 3.22 e un compilatore:
- **Windows**: Visual Studio 2022 (workload "Sviluppo di applicazioni desktop con C++")
- **Mac**: Xcode (o i Command Line Tools)

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```
JUCE viene scaricato in automatico alla prima configurazione. I file compilati finiscono in `build/Driftfield_artefacts/Release/`.

## Installazione in Ableton Live (Mac)

1. Scompatta `Driftfield-AU-mac.zip` e `Driftfield-VST3-mac.zip`.
2. Nel Finder: **Vai → Vai alla cartella…** (`Cmd+Shift+G`) e copia:
   - `Driftfield.component` → `~/Library/Audio/Plug-Ins/Components/`
   - `Driftfield.vst3` → `~/Library/Audio/Plug-Ins/VST3/`
3. Il plugin non è firmato con un certificato Apple: apri il Terminale e lancia
   ```bash
   xattr -cr ~/Library/Audio/Plug-Ins/Components/Driftfield.component ~/Library/Audio/Plug-Ins/VST3/Driftfield.vst3
   ```
4. In Ableton: **Settings → Plug-Ins** → attiva *Use Audio Units v2* e/o *Use VST3 Plug-In System Folders* → **Rescan**.
5. Lo trovi nel Browser sotto **Plug-Ins → Driftfield Audio → Driftfield**: trascinalo su una traccia.

## Installazione in FL Studio

**Windows**: copia `Driftfield.vst3` in `C:\Program Files\Common Files\VST3\`.

**Mac**:
- VST3 → `~/Library/Audio/Plug-Ins/VST3/`
- AU → `~/Library/Audio/Plug-Ins/Components/`
- Il plugin non è firmato con un certificato Apple, quindi la prima volta potresti dover eseguire:
  `xattr -cr ~/Library/Audio/Plug-Ins/VST3/Driftfield.vst3`

Poi in FL Studio vai su **Options → Manage plugins → Find plugins**, cerca "Driftfield" e aggiungilo a un canale del Mixer come effetto.
Per sentire subito l'effetto: un 3x Osc con una sinusoide pura, il preset *Lo-fi Sine* o *Rain Window*, e suona qualche nota.

## Struttura del codice

```
Source/DSP.h              tutto il DSP (C++ puro, senza JUCE): facile da studiare e modificare
Source/PluginProcessor.*  parametri, preset, collegamento con la DAW
Source/PluginEditor.*     interfaccia grafica (le 5 card colorate)
tests/dsp_render.cpp      render offline in WAV per testare il suono senza DAW
tests/plugin_smoke.cpp    test: carica tutti i preset, elabora audio, salva uno screenshot della UI
```

Test offline (genera dei WAV da ascoltare):
```bash
cmake -B build -DDRIFTFIELD_BUILD_TESTS=ON && cmake --build build --target dsp_render
./build/dsp_render .
```

## Idee per le prossime versioni
- **Capture/Looper**: registrare un suono e farlo girare in loop dentro gli effetti (c'è nel plugin originale).
- **Harmonizer con pitch detection**: voci trasposte che seguono la nota suonata, invece dei risonatori.
- Salvataggio dei preset dell'utente e più Mood.
- Visualizzazione animata (forma d'onda / particelle) al centro della UI.

## Nome e marchio
"Driftfield" e "Driftfield Audio" sono dei segnaposto: puoi cambiarli in `CMakeLists.txt` (`PRODUCT_NAME`, `COMPANY_NAME`, `BUNDLE_ID`, `PLUGIN_MANUFACTURER_CODE`) e nel titolo disegnato in `PluginEditor.cpp`.

## Licenza di JUCE
JUCE è gratuito per uso personale e per chi ha ricavi sotto una certa soglia annua. Se un giorno vuoi venderlo, controlla la licenza JUCE attuale su juce.com.
