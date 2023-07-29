project('libdash', 'c',
        version: '0.0.1',
	default_options: ['buildtype=debug'])

# Detect the compiler in order to check features
cc = meson.get_compiler('c')

# Provide an instance for configuration
conf = configuration_data()

# Check if the compiler supports the __alias__ attribute
conf.set10('HAVE_ALIAS_ATTRIBUTE',
  cc.has_function_attribute('alias'),
  description: 'Define if compiler has the alias attribute')

# Check if the compiler supports the __unused__ attribute
conf.set10('HAVE_UNUSED_ATTRIBUTE',
  cc.has_function_attribute('unused'),
  description: 'Define if compiler has the unused attribute')

# Check if the compiler supports the __visibility__ attribute
conf.set10('HAVE_VISIBLE_ATTRIBUTE',
  cc.has_function_attribute('visibility'),
  description: 'Define if compile has the visibility attribute')

# Check for the standard C headers
std_headers = []
foreach hdr: std_headers
  conf.set10('HAVE_' + hdr.underscorify().to_upper(),
    cc.check_header(hdr),
    description: f'Define if you have the @hdr@ header')
endforeach

# Check for library functions
req_funcs = []
foreach func: req_funcs
  conf.set10('HAVE_' + func.underscorify().to_upper(),
    cc.check_function(func),
    description: f'Define if you have the @func@ function')
endforeach

# Provide defaults for obstack
conf.set('obstack_chunk_alloc',
  'malloc',
  description: 'Define the allocator to use for obstack')
conf.set('obstack_chunk_free',
  'free',
  description: 'Define the function to free obstack memory')

inline_queue = get_option('inline_queue_funcs')
conf.set10('INLINE_QUEUE_FUNCS', inline_queue,
  description: 'Define if the queue functions should be compiled inline')

# Enable debug support if required
bld_debug = get_option('build_debug')
if bld_debug > 0
  conf.set('SHPARSE_DEBUG', get_option('build_debug'),
    description: 'Enable built-in debug support')
endif

# Write out the config.h file to be used
config_h = configure_file(configuration: conf,
  output: 'config.h')
add_project_arguments(['-imacros', 'config.h'], language: 'c')

subdir('src')
