# SemantiCAD

**Natural-language CAD.** Tell a 3D model what to do in plain Turkish or English — *"resize the object 2x"*, *"rotate it 90 degrees about z"* — and SemantiCAD turns the instruction into a structured command and applies it to the geometry in real time.

A local, offline C++17 desktop application: a quantized LLM (via [llama.cpp](https://github.com/ggerganov/llama.cpp)) parses the instruction into a validated JSON command, and a [VTK](https://vtk.org/) viewer renders the result. No cloud, no API keys — the model runs on your machine.

```
"objeyi yüzde 5 büyült"  ──► LLM ──► {"command":"resize","params":{"x":1.05,"y":1.05,"z":1.05}} ──► VTK
```

---

## Highlights

- **Bilingual (EN + TR) natural language** → structured CAD operations.
- **Fully local & offline** — the LLM (Qwen2.5-3B-Instruct, Q4_K_M) runs through llama.cpp, with optional CUDA acceleration.
- **Grammar-constrained decoding** — the model is forced to emit valid command JSON (GBNF generated from the command schema), so output is always parseable and within range.
- **Data-driven commands** — every command is a JSON file in [`commands/`](commands/). Add a new one and restart; no recompile.
- **Cleanly layered architecture** — the NLP layer and the rendering layer never reference each other; they talk over a header-only event bus.
- **Scriptable & testable** — apply commands directly from JSON (bypassing the model) and run headless for automated verification. Command layer is covered by GoogleTest unit tests.

## Supported commands

| Command     | What it does                                  | Parameters                                                        |
|-------------|-----------------------------------------------|-------------------------------------------------------------------|
| `resize`    | Scale the object (per-axis multipliers)       | `x`, `y`, `z` — multipliers (`1.05` = +5%, `2.0` = double)        |
| `translate` | Move the object                               | `dx`, `dy`, `dz` — signed deltas in millimetres                   |
| `rotate`    | Rotate around an axis                         | `axis` (`x`/`y`/`z`), `angle_deg` (degrees, −360…360)             |
| `mirror`    | Reflect across a plane                        | `plane` (`xy`/`yz`/`xz`)                                          |
| `none`      | Fallback for unclear / unsupported requests   | `reason` — short explanation                                      |

Commands are defined entirely in [`commands/*.json`](commands/) — see [Adding a command](#adding-a-command).

## Architecture

SemantiCAD is split into decoupled layers. **The NLP layer and the VTK layer never include each other** — they communicate only by publishing/subscribing to `CommandEvent`s on a header-only event bus. `main.cpp` is the composition root.

```
                          ┌──────────────────────────────────────────┐
  Natural language  ───►  │  NLP layer  (src/core/NLP/)                │
  (TR / EN)               │   • CommandCatalog  – loads commands/*.json │
                          │   • PromptBuilder   – few-shot prompt       │
                          │   • InferenceEngine – llama.cpp + GBNF       │
                          │   • CommandParser   – validate → envelope   │
                          └───────────────────┬──────────────────────┘
                                              │  CommandEvent
                                              │  { command, params }
                                  ┌───────────▼───────────┐
                                  │   EventBus (pub/sub)   │   src/core/eventbus/
                                  └───────────┬───────────┘
                                              │  CommandEvent
                          ┌───────────────────▼──────────────────────┐
                          │  VTK layer  (src/core/vtk/)               │
                          │   • VtkViewer – subscribes to events       │
                          │   • CommandDispatcher – routes by name     │
                          │       → ResizeCommand / TranslateCommand…  │
                          │   • CadObject – wraps the vtkPolyData      │
                          └────────────────────────────────────────────┘

  Infrastructure (src/infrastructure/MeshLoader) loads .stl/.obj/.ply/.vtp by extension.
```

**Two notions of "command", kept separate:**
- **Command *definitions*** ([`commands/*.json`](commands/)) drive the NLP side — the prompt docs, the generation grammar, and validation. This is where you teach the model *which* operation an instruction maps to.
- **Command *execution*** (`src/core/command/`, the `semanticad_command` library) applies the operation. Each command lives in its own subfolder as a value object (validated params) + a service (the VTK transform) + an `ICommand` adapter. `CommandDispatcher` routes an event by name to the right `ICommand`.

Transforms are **baked directly into the object's `vtkPolyData`** (via the `CadObject` entity), so they are destructive/cumulative rather than a non-destructive actor transform.

## Project layout

```
SemantiCAD/
├── commands/                  # Data-driven command definitions (JSON)
├── src/
│   ├── main.cpp               # Composition root + CLI
│   ├── core/
│   │   ├── NLP/               # Instruction → validated JSON command
│   │   │   ├── scheme/        #   CommandCatalog, DynamicCommandSchema
│   │   │   └── process/       #   SemanticIO, PromptBuilder, CommandParser
│   │   ├── eventbus/          # Header-only pub/sub + CommandEvent
│   │   ├── command/           # Command execution (resize/translate/rotate/mirror)
│   │   └── vtk/               # VtkViewer (subscriber)
│   └── infrastructure/        # MeshLoader (file I/O)
├── tests/                     # GoogleTest unit tests for the command layer
├── external/llama.cpp/        # Submodule
├── models/                    # GGUF model files (gitignored)
└── download_model.sh
```

## Requirements

- **CMake** ≥ 3.14 and a **C++17** compiler
- **VTK 9** (with the `CommonCore`, `CommonDataModel`, `CommonTransforms`, `FiltersSources`, `FiltersGeneral`, `IOGeometry`, `IOPLY`, `IOXML`, `RenderingCore`, `RenderingOpenGL2`, `InteractionStyle` components)
- **GoogleTest** (system install, only needed for the unit tests — on by default)
- **CUDA toolkit** *(optional)* — for GPU-accelerated inference; on by default, see [Build options](#build-options)
- `wget` (used by `download_model.sh`)

llama.cpp and nlohmann/json are vendored via the `external/llama.cpp` submodule — no separate install needed.

## Build

```bash
# 1. Clone with the llama.cpp submodule
git clone --recurse-submodules <repo-url>
cd SemantiCAD
# (if you already cloned without submodules: git submodule update --init --recursive)

# 2. Download the language model (~1.1 GB) into models/
./download_model.sh

# 3. Configure & build
cmake -B build
cmake --build build -j
```

### Build options

| Option                    | Default | Description                                              |
|---------------------------|---------|----------------------------------------------------------|
| `SEMANTICAD_USE_CUDA`     | `ON`    | Enable CUDA acceleration in ggml. Set `OFF` for CPU-only.|
| `SEMANTICAD_USE_VTK`      | `ON`    | Build the VTK visualization + command layer.             |
| `SEMANTICAD_BUILD_TESTS`  | `ON`    | Build the command-layer unit tests.                      |

CPU-only build, for example:

```bash
cmake -B build -DSEMANTICAD_USE_CUDA=OFF
cmake --build build -j
```

## Usage

Run **from the project root** (the default `commands/` path is resolved relative to the working directory).

```bash
# Natural-language instruction (drives the model)
./build/semanticad --object model.stl "objeyi 2 kat büyült"
./build/semanticad --object model.stl "rotate it 90 degrees around the z axis"

# No object? A default cube is loaded.
./build/semanticad "yüksekliği yüzde 10 azalt"
```

### CLI options

| Flag                | Description                                                                       |
|---------------------|-----------------------------------------------------------------------------------|
| `--object <file>`   | 3D model to load (`.stl` / `.obj` / `.ply` / `.vtp`). Defaults to a built-in cube.|
| `--commands <dir>`  | Command-definition directory (default: `commands`).                               |
| `--apply <json>`    | Apply a `{"command":…,"params":…}` envelope directly, **bypassing the model**.    |
| `--no-window`       | Headless: print object bounds and exit without opening a render window.           |
| `<text…>`           | Natural-language instruction for the NLP layer.                                   |

### Scripting / deterministic testing

`--apply` feeds a command straight to the VTK layer with no model involved — handy for scripting or reproducible tests:

```bash
./build/semanticad --object model.stl \
  --apply '{"command":"resize","params":{"x":2.0,"y":2.0,"z":2.0}}'
```

Combine with `--no-window` for headless verification (prints the object's bounds before and after):

```bash
./build/semanticad --object model.stl --no-window \
  --apply '{"command":"translate","params":{"dx":10,"dy":0,"dz":0}}'
```

## Adding a command

Two kinds of "command" exist, so adding a brand-new operation can touch both — but teaching the model a new mapping is **data only**:

**1. Define it for the NLP layer** — drop a new file in [`commands/`](commands/) and restart. No recompile.

```jsonc
{
  "name": "myop",
  "description": "What it does. Bilingual text steers the model (TR + EN).",
  "examples": [
    "do the thing -> {\"command\":\"myop\",\"params\":{\"value\":1.0}}"
  ],
  "params": [
    { "name": "value", "type": "number", "required": true, "min": 0.0, "max": 100.0 }
  ]
}
```

Supported param fields: `type` (`number`/`integer`/`string`/`boolean`), `required`, `default`, `min`/`max` (+ `exclusiveMin`/`exclusiveMax`), `enum`, `note`. Enums are enforced by the grammar; numeric ranges are checked during decode. Malformed definitions are reported per-file and skipped — they don't crash startup.

**2. (If it's a new operation) implement its execution** — add a subfolder under `src/core/command/` with a value object + service + `ICommand`, list its `.cpp` files in [`CMakeLists.txt`](CMakeLists.txt), and register one line in `CommandDispatcher::registerBuiltins()`. The shared `CadObject` entity is reused.

## Testing

The command-execution layer is unit-tested with GoogleTest. Tests run each command against a real (tiny) `vtkPolyData` and assert on the resulting bounds — **no window, GPU, or model required** (they do link VTK).

```bash
cmake --build build -j
ctest --test-dir build --output-on-failure
```

To build just the tests (skips the llama/model parts):

```bash
cmake --build build --target semanticad_tests
```

## License

[MIT](LICENSE) © 2026 Mustafa Bahadır

llama.cpp (bundled as a submodule) is distributed under its own MIT license.
