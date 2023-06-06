incldir = include_directories('include')
subdir('libdash')
executable('chklibdash', 'chklibdash.c',
  include_directories: incldir,
  link_with: [libdash_so])
