# Source SBOM

Source repository inventory only; completeness is UNKNOWN. Syft inventories tracked source/package manifests. Gitlinks and declared vendored/model evidence are inspected at immutable pins. CMake declarations are conditional requirements, not resolved installations. This is NOT proof of operating-system packages, static-link closure, build-time resolution, or the contents of release binaries.

Generation: **success**

## Source identity

wave-owl/echolink-core `0d1fbc490959b88b7ff37595c32e2c2a2596378c`

- sbom:role: source-root
- sbom:git-commit: 0d1fbc490959b88b7ff37595c32e2c2a2596378c
- sbom:git-tree: 7600153a8fc3737b531781f1b3bae44b6ed90903
- sbom:source-repository: https://github.com/wave-owl/echolink-core
- sbom:release-tags: ["v0.1.1"]
- sbom:license-evidence: [{"path": "LICENSE", "sha256": "3972dc9744f6499f0f9b2dbf76696f2ae7ad8af9b23dde66d6af86c9dfb36986"}]
- sbom:license-resolution: unresolved; manual review required

## Generation errors

None.

## Components and unresolved licenses

| Component | Version | Role | License |
| --- | --- | --- | --- |
| wave-owl/echolink-core | 0d1fbc490959b88b7ff37595c32e2c2a2596378c | source-root | unresolved; manual review required |
| wave-owl/libgsm | b77cd90d77c50ba62ad321cf0ebc2ad9deb75a17 | reference-only | unresolved; manual review required |
| wave-owl/svxlink | 595cf9bb3325be753d05b219bdd39feab6a22b33 | reference-only | unresolved; manual review required |
| SIGC | unresolved | conditional-build-requirement | unresolved |
| LADSPA | unresolved | conditional-build-requirement | unresolved |
| Speex | unresolved | conditional-build-requirement | unresolved |
| Opus | unresolved | conditional-build-requirement | unresolved |
| OGG | unresolved | conditional-build-requirement | unresolved |
| GSM | unresolved | conditional-build-requirement | unresolved |
| ALSA | unresolved | conditional-build-requirement | unresolved |
| OpenSSL | unresolved | conditional-build-requirement | unresolved |
| Threads | unresolved | conditional-build-requirement | unresolved |
| DL | unresolved | conditional-build-requirement | unresolved |
| Threads | unresolved | conditional-build-requirement | unresolved |
| RT | unresolved | conditional-build-requirement | unresolved |
| Qt6Core | unresolved | conditional-build-requirement | unresolved |
| Qt6Gui | unresolved | conditional-build-requirement | unresolved |
| Qt6Widgets | unresolved | conditional-build-requirement | unresolved |
| Qt6Network | unresolved | conditional-build-requirement | unresolved |
| Qt5Core | unresolved | conditional-build-requirement | unresolved |
| Qt5Gui | unresolved | conditional-build-requirement | unresolved |
| Qt5Widgets | unresolved | conditional-build-requirement | unresolved |
| Qt5Network | unresolved | conditional-build-requirement | unresolved |
| Qt4 | unresolved | conditional-build-requirement | unresolved |
| Qt6Core | unresolved | conditional-build-requirement | unresolved |
| Qt6Gui | unresolved | conditional-build-requirement | unresolved |
| Qt6Widgets | unresolved | conditional-build-requirement | unresolved |
| Qt6Network | unresolved | conditional-build-requirement | unresolved |
| Qt5Core | unresolved | conditional-build-requirement | unresolved |
| Qt5Gui | unresolved | conditional-build-requirement | unresolved |
| Qt5Widgets | unresolved | conditional-build-requirement | unresolved |
| Qt5Network | unresolved | conditional-build-requirement | unresolved |
| Qt4 | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| ${LIBFIND_PACKAGE_ARGS} | unresolved | conditional-build-requirement | unresolved |
| PkgConfig | unresolved | conditional-build-requirement | unresolved |
| Doxygen | unresolved | conditional-build-requirement | unresolved |
| GSM | unresolved | conditional-build-requirement | unresolved |
| Speex | unresolved | conditional-build-requirement | unresolved |
| Popt | unresolved | conditional-build-requirement | unresolved |
| GSM | unresolved | conditional-build-requirement | unresolved |
| Qt6Core | unresolved | conditional-build-requirement | unresolved |
| Qt6Gui | unresolved | conditional-build-requirement | unresolved |
| Qt6Widgets | unresolved | conditional-build-requirement | unresolved |
| Qt6Network | unresolved | conditional-build-requirement | unresolved |
| Qt6Core5Compat | unresolved | conditional-build-requirement | unresolved |
| Qt5Core | unresolved | conditional-build-requirement | unresolved |
| Qt5Gui | unresolved | conditional-build-requirement | unresolved |
| Qt5Widgets | unresolved | conditional-build-requirement | unresolved |
| Qt5Network | unresolved | conditional-build-requirement | unresolved |
| Qt4 | unresolved | conditional-build-requirement | unresolved |
| Qt6LinguistTools | unresolved | conditional-build-requirement | unresolved |
| Qt5LinguistTools | unresolved | conditional-build-requirement | unresolved |
| Popt | unresolved | conditional-build-requirement | unresolved |
| GCrypt | unresolved | conditional-build-requirement | unresolved |
| Popt | unresolved | conditional-build-requirement | unresolved |
| TCL | unresolved | conditional-build-requirement | unresolved |
| GSM | unresolved | conditional-build-requirement | unresolved |
| GSM | unresolved | conditional-build-requirement | unresolved |
| Popt | unresolved | conditional-build-requirement | unresolved |
| Popt | unresolved | conditional-build-requirement | unresolved |
| GCrypt | unresolved | conditional-build-requirement | unresolved |
| GCrypt | unresolved | conditional-build-requirement | unresolved |
| Popt | unresolved | conditional-build-requirement | unresolved |
| GSM | unresolved | conditional-build-requirement | unresolved |
| TCL | unresolved | conditional-build-requirement | unresolved |
| DL | unresolved | conditional-build-requirement | unresolved |
| CURL | unresolved | conditional-build-requirement | unresolved |
| GCrypt | unresolved | conditional-build-requirement | unresolved |
| RtlSdr | unresolved | conditional-build-requirement | unresolved |
| Threads | unresolved | conditional-build-requirement | unresolved |
| GPIOD | unresolved | conditional-build-requirement | unresolved |
| src/gsm | b77cd90d77c50ba62ad321cf0ebc2ad9deb75a17 | declared-vendored-source | unresolved; manual review required |
| src/elk_packets.c | 595cf9bb3325be753d05b219bdd39feab6a22b33 | declared-vendored-source | unresolved; manual review required |
| src/elk_codec.c | 595cf9bb3325be753d05b219bdd39feab6a22b33 | declared-vendored-source | unresolved; manual review required |
| actions/checkout | v4 | Syft source/package discovery | unresolved |
| tracked-source/.github/workflows/ci.yml | unresolved | Syft source/package discovery | unresolved |

License evidence paths/hashes, source manifests, model hashes and conditional
CMake declarations are recorded in SBOM.cdx.json. Declared upstream pins do not
prove vendored equivalence. No dependency installation or build was performed.
