# Building Scrambler

## VCV Rack

See https://vcvrack.com/manual/Building

```bash
export RACK_DIR=~/Rack-SDK
make install
```

## MetaModule

> **Note:** The paths below assume a `~/projects/` layout (e.g. `~/projects/Scrambler`, `~/projects/metamodule-plugin-sdk`). Adjust to match your machine.

### Prerequisites (one-time setup)

1. **ARM GCC Toolchain v12.2 or v12.3** (must be this version — newer won't work)

   Download `arm-gnu-toolchain-12.3.rel1-darwin-arm64-arm-none-eabi.tar.xz` from the
   [Arm Developer site](https://developer.arm.com/downloads/-/arm-gnu-toolchain-downloads)
   and extract it:

   ```bash
   mkdir -p ~/toolchains
   tar -xf ~/Downloads/arm-gnu-toolchain-12.3.rel1-darwin-arm64-arm-none-eabi.tar.xz -C ~/toolchains
   ```

2. **CMake** (v3.22+) and **Ninja**

   ```bash
   brew install cmake ninja
   ```

3. **MetaModule Plugin SDK**

   ```bash
   git clone https://github.com/4ms/metamodule-plugin-sdk --recursive ~/projects/metamodule-plugin-sdk
   ```

### Configure (first time, or after changing CMakeLists.txt)

This generates the `mm-build/` directory. You only need to re-run this when `CMakeLists.txt` changes.

```bash
cd ~/projects/Scrambler
cmake --fresh -B mm-build -G Ninja \
  -DMETAMODULE_SDK_DIR=$HOME/projects/metamodule-plugin-sdk \
  -DTOOLCHAIN_BASE_DIR=$HOME/toolchains/arm-gnu-toolchain-12.3.rel1-darwin-arm64-arm-none-eabi/bin
```

### Build

```bash
cmake --build mm-build
```

The output file lands in `metamodule-plugins/Scrambler.mmplugin`.

Rename it to include the version before distributing:

```bash
mv metamodule-plugins/Scrambler.mmplugin metamodule-plugins/Scrambler-v2.0.0.mmplugin
```

## Releasing

To release a new version (both VCV Rack and MetaModule):

```bash
./release.sh 2.1.0
```

This will:

1. Update the version in `plugin.json` and `CMakeLists.txt`
2. Commit the change
3. Push two tags: `v2.1.0` (triggers the VCV Rack build) and `mm-v2.1.0` (triggers the MetaModule build)
4. Each workflow creates its own GitHub release with the built artifacts attached

Requires `jq` to be installed (`brew install jq`).

### Panel artwork

The MetaModule cannot render SVGs. The panel PNG in `assets/` was generated from `res/SCRAMBLER.svg` using the SDK's conversion script:

```bash
export INKSCAPE_BIN_PATH=/Applications/Inkscape.app/Contents/MacOS/inkscape
cd ~/projects/metamodule-plugin-sdk
python3 scripts/SvgToPng.py --input ~/projects/Scrambler/res --output ~/projects/Scrambler/assets --height=240
```

Re-run this if you change the panel SVG. The PNG must be 240 pixels tall.
