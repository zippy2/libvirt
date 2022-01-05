executable(
  'virt-dump-config',
  [
    'virt-dump-config.c'
  ],
  dependencies: [
    tools_dep,
  ],
  link_args: [
    coverage_flags,
  ],
  link_with: [
    libvirt_lib,
  ],
  install: true,
  install_dir: libexecdir,
  install_rpath: libvirt_rpath,
)
