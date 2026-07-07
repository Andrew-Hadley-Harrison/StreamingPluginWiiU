FROM ghcr.io/wiiu-env/devkitppc:20260225
# Match the toolchain the current official Aroma plugins (e.g. ftpiiu) are built
# with, so the WUPS runtime/ABI matches the user's Aroma PluginBackend and the
# plugin is not flagged "outdated"/"possibly_broken_reent". turbojpeg is bundled
# in the repo (libs/), and libutils has been dropped (vendored shims under src/).
COPY --from=ghcr.io/wiiu-env/wiiupluginsystem:20260418 /artifacts $DEVKITPRO
WORKDIR project
