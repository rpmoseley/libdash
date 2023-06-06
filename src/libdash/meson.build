libdash_sources = files(['libdash.c'])

subdir('shparse')

libdash_so = library('dash', libdash_sources, include_directories: [incldir, inclshparse], gnu_symbol_visibility: 'hidden')
