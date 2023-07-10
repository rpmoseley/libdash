libdash_sources = files(['libdash.c'])
incldir = include_directories('include')

subdir('shparse')

libdash_so = library('dash', libdash_sources, include_directories: [incldir, inclshparse], gnu_symbol_visibility: 'hidden')
