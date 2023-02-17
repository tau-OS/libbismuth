gidocgen = find_program('gi-docgen')

toml_data = configuration_data()
toml_data.set('VERSION', meson.project_version())

libbismuth_toml = configure_file(
  input: 'libbismuth.toml.in',
  output: 'libbismuth.toml',
  configuration: toml_data
)

docs_dir = datadir / 'doc'

custom_target('libbismuth-doc',
  input: [ libbismuth_toml, libbismuth_gir[0], meson.project_source_root() / 'assets' / 'Bismuth.svg'],
  output: [ 'libbismuth-@0@'.format(apiversion) ],
  command: [
    gidocgen,
    'generate',
    '--quiet',
    '--add-include-path=@0@'.format(meson.current_build_dir() / '../../src'),
    '--config=@INPUT0@',
    '--output-dir=@OUTPUT@',
    '--no-namespace-dir',
    '--content-dir=@0@'.format(meson.current_source_dir()),
    '@INPUT1@',
    '&&',
    'cp',
    '@INPUT2@',
    '@OUTPUT@',
  ],
  build_by_default: true,
  install: true,
  install_dir: docs_dir,
)
