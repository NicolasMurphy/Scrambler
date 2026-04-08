#!/bin/bash
set -euo pipefail

# Release script for Scrambler
# Usage: ./release.sh <version>
# Example: ./release.sh 2.1.0

VERSION="$1"

if [[ -z "$VERSION" ]]; then
    echo "Usage: ./release.sh <version>"
    echo "Example: ./release.sh 2.1.0"
    exit 1
fi

# Validate semver format
if [[ ! "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "Error: Version must be in semver format (e.g. 2.1.0)"
    exit 1
fi

# Extract major.minor for CMakeLists.txt (it uses VERSION X.Y, no patch)
CMAKE_VERSION="${VERSION%.*}"

# Check for uncommitted changes
if ! git diff --quiet || ! git diff --cached --quiet; then
    echo "Error: You have uncommitted changes. Commit or stash them first."
    exit 1
fi

# Show what we're about to do
CURRENT_VERSION=$(jq -r '.version' plugin.json)
echo ""
echo "  Scrambler Release"
echo "  ───────────────────"
echo "  Current version:  $CURRENT_VERSION"
echo "  New version:      $VERSION"
echo ""
echo "  This will:"
echo "    1. Update plugin.json       ($CURRENT_VERSION → $VERSION)"
echo "    2. Update CMakeLists.txt    (VERSION $CMAKE_VERSION)"
echo "    3. Commit: \"Bump version to $VERSION\""
echo "    4. Tag:    v$VERSION  (VCV Rack release)"
echo "    5. Tag:    mm-v$VERSION  (MetaModule release)"
echo "    6. Push commit + both tags"
echo ""
read -p "  Proceed? [y/N] " confirm
if [[ "$confirm" != "y" && "$confirm" != "Y" ]]; then
    echo "Aborted."
    exit 0
fi

# 1. Update plugin.json
jq --arg v "$VERSION" '.version = $v' plugin.json > plugin.json.tmp && mv plugin.json.tmp plugin.json
echo "Updated plugin.json → $VERSION"

# 2. Update CMakeLists.txt
sed -i.bak -E "s/(^    VERSION )[0-9]+\.[0-9]+/\1$CMAKE_VERSION/" CMakeLists.txt && rm CMakeLists.txt.bak
echo "Updated CMakeLists.txt → VERSION $CMAKE_VERSION"

# 3. Commit
git add plugin.json CMakeLists.txt
git commit -m "Bump version to $VERSION"
echo "Committed."

# 4-5. Tag
git tag "v$VERSION"
git tag "mm-v$VERSION"
echo "Tagged v$VERSION and mm-v$VERSION"

# 6. Push
git push && git push --tags
echo ""
echo "Done! Both workflows should now be running:"
echo "  https://github.com/NicolasMurphy/Scrambler/actions/workflows/build-plugin.yml"
echo "  https://github.com/NicolasMurphy/Scrambler/actions/workflows/build-metamodule.yml"
