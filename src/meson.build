libbismuth_header_subdir = package_subdir / package_api_name
libbismuth_header_dir = get_option('includedir') / libbismuth_header_subdir

libbismuth_resources = gnome.compile_resources(
  'bis-resources',
  'bismuth.gresources.xml',
  c_name: 'bis',
)

bis_public_enum_headers = [
  'bis-animation.h',
  'bis-lapel.h',
  'bis-fold-threshold-policy.h',
  'bis-easing.h',
  'bis-album.h',
  'bis-navigation-direction.h',
  'bis-hugger.h',
]

bis_private_enum_headers = [
  'bis-settings-private.h',
]

version_data = configuration_data()
version_data.set('BIS_MAJOR_VERSION', bismuth_version_major)
version_data.set('BIS_MINOR_VERSION', bismuth_version_minor)
version_data.set('BIS_MICRO_VERSION', bismuth_version_micro)
version_data.set('BIS_VERSION', meson.project_version())

bis_version_h = configure_file(
           input: 'bis-version.h.in',
          output: 'bis-version.h',
     install_dir: libbismuth_header_dir,
   configuration: version_data)

libbismuth_generated_headers = [
  bis_version_h,
]

install_headers(['bismuth.h'],
  subdir: libbismuth_header_subdir)

# Filled out in the subdirs
libbismuth_public_headers = []
libbismuth_public_sources = []
libbismuth_private_sources = []

bis_public_enums = gnome.mkenums_simple('bis-enums',
         sources: bis_public_enum_headers,
     body_prefix: '#include "config.h"',
   header_prefix: '#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)\n' +
                  '#error "Only <bismuth.h> can be included directly."\n' +
                  '#endif\n' +
                  '\n' +
                  '#include "bis-version.h"\n',
       decorator: 'BIS_AVAILABLE_IN_ALL',
  install_header: true,
     install_dir: libbismuth_header_dir,
)

bis_private_enums = gnome.mkenums_simple('bis-enums-private',
         sources: bis_private_enum_headers,
     body_prefix: '#include "config.h"',
   header_prefix: '#if !defined(_BISMUTH_INSIDE) && !defined(BISMUTH_COMPILATION)\n' +
                  '#error "Only <bismuth.h> can be included directly."\n' +
                  '#endif\n' +
                  '\n' +
                  '#include "bis-enums.h"\n',
  install_header: false,
)

libbismuth_public_sources += [bis_public_enums[0]]
libbismuth_private_sources += bis_private_enums
libbismuth_generated_headers += [bis_public_enums[1]]

src_headers = [
  'bis-animation.h',
  'bis-animation-target.h',
  'bis-animation-util.h',
  'bis-bin.h',
  'bis-carousel.h',
  'bis-carousel-indicator-dots.h',
  'bis-carousel-indicator-lines.h',
  'bis-clamp.h',
  'bis-clamp-layout.h',
  'bis-clamp-scrollable.h',
  'bis-deprecation-macros.h',
  'bis-easing.h',
  'bis-enum-list-model.h',
  'bis-lapel.h',
  'bis-fold-threshold-policy.h',
  'bis-album.h',
  'bis-main.h',
  'bis-navigation-direction.h',
  'bis-spring-animation.h',
  'bis-spring-params.h',
  'bis-hugger.h',
  'bis-swipe-tracker.h',
  'bis-swipeable.h',
  'bis-timed-animation.h',
]

gen_public_types = find_program('gen-public-types.py', required: true)

libbismuth_init_public_types = custom_target('bis-public-types.c',
   output: 'bis-public-types.c',
    input: [src_headers, libbismuth_generated_headers],
  command: [gen_public_types, '@INPUT@'],
  capture: true,
)

src_sources = [
  'bis-animation.c',
  'bis-animation-target.c',
  'bis-animation-util.c',
  'bis-bin.c',
  'bis-carousel.c',
  'bis-carousel-indicator-dots.c',
  'bis-carousel-indicator-lines.c',
  'bis-clamp.c',
  'bis-clamp-layout.c',
  'bis-clamp-scrollable.c',
  'bis-easing.c',
  'bis-enum-list-model.c',
  'bis-lapel.c',
  'bis-fold-threshold-policy.c',
  'bis-album.c',
  'bis-main.c',
  'bis-navigation-direction.c',
  'bis-spring-animation.c',
  'bis-spring-params.c',
  'bis-hugger.c',
  'bis-swipe-tracker.c',
  'bis-swipeable.c',
  'bis-timed-animation.c',
  'bis-version.c',
]

# Files that should not be introspected
libbismuth_private_sources += files([
  'bis-bidi.c',
  'bis-tool.c',
  'bis-gtkbuilder-utils.c',
  'bis-settings.c',
  'bis-shadow-helper.c',
  'bis-widget-utils.c',
])

libbismuth_public_headers += files(src_headers)
libbismuth_public_sources += files(src_sources)

install_headers(src_headers, subdir: libbismuth_header_subdir)


libbismuth_sources = [
  libbismuth_generated_headers,
  libbismuth_public_sources,
  libbismuth_private_sources,
  libbismuth_resources,
  libbismuth_init_public_types,
]

glib_min_version = '>= 2.66'
gtk_min_version = '>= 4.5.0'

gio_dep = dependency('gio-2.0', version: glib_min_version)
gtk_dep = dependency('gtk4', version: gtk_min_version)

libbismuth_deps = [
  dependency('glib-2.0', version: glib_min_version),
  dependency('fribidi'),
  gio_dep,
  gtk_dep,
  cc.find_library('m', required: false),
]

libbismuth_public_deps = [
  gio_dep,
  gtk_dep,
]

libbismuth_c_args = [
  '-DG_LOG_DOMAIN="Bismuth"',
]

config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', 'libbismuth')
config_h.set_quoted('LOCALEDIR', get_option('prefix') / get_option('localedir'))

# Symbol visibility
configure_file(
         output: 'config.h',
  configuration: config_h,
)

libbismuth_link_args = []
libbismuth_symbols_file = 'libbismuth.syms'

# Check linker flags
ld_version_script_arg = '-Wl,--version-script,@0@/@1@'.format(meson.project_source_root(),
                                                              libbismuth_symbols_file)
if cc.links('int main() { return 0; }', args : ld_version_script_arg, name : 'ld_supports_version_script')
  libbismuth_link_args += [ld_version_script_arg]
endif

bismuth_libdir = libdir

libbismuth = library(
  'bismuth-' + apiversion,
  libbismuth_sources,

              soversion: soversion,
                 c_args: libbismuth_c_args,
           dependencies: libbismuth_deps,
    include_directories: [ root_inc, src_inc ],
                install: true,
              link_args: libbismuth_link_args,
            install_dir: bismuth_libdir,
  gnu_symbol_visibility: 'hidden',
)

libbismuth_dep = declare_dependency(
              sources: libbismuth_generated_headers,
         dependencies: libbismuth_public_deps,
            link_with: libbismuth,
  include_directories: include_directories('.'),
)

meson.override_dependency(package_api_name, libbismuth_dep)

if introspection
   libbismuth_gir_extra_args = [
     '--c-include=bismuth.h',
     '-DBISMUTH_COMPILATION',
   ]

   libbismuth_gir = gnome.generate_gir(libbismuth,
                 sources: libbismuth_generated_headers + libbismuth_public_headers + libbismuth_public_sources,
               nsversion: apiversion,
               namespace: 'Bis',
         export_packages: package_api_name,
           symbol_prefix: 'bis',
       identifier_prefix: 'Bis',
               link_with: libbismuth,
                includes: ['Gio-2.0', 'Gtk-4.0'],
                 install: true,
         install_dir_gir: girdir,
     install_dir_typelib: typelibdir,
              extra_args: libbismuth_gir_extra_args,
   )

   if get_option('vapi')
     libbismuth_vapi = gnome.generate_vapi(package_api_name,
             sources: libbismuth_gir[0],
            packages: [ 'gio-2.0', 'gtk4' ],
             install: true,
         install_dir: vapidir,
       metadata_dirs: [ meson.current_source_dir() ],
     )
   endif
endif

pkgg = import('pkgconfig')

pkgg.generate(
    libraries: [libbismuth],
      subdirs: libbismuth_header_subdir,
      version: meson.project_version(),
         name: 'Bismuth',
     filebase: package_api_name,
  description: 'The best responsive widgets ever',
     requires: 'gtk4',
  install_dir: libdir / 'pkgconfig',
)
