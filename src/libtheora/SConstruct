# see http://www.scons.org if you do not have this tool
from os.path import join
import SCons

# TODO: should use lamda and map to work on python 1.5
def path(prefix, list): return [join(prefix, x) for x in list]

encoder_sources = """
	apiwrapper.c
	fragment.c
	idct.c
	internal.c
	state.c
	quant.c
	analyze.c
	encfrag.c
	encapiwrapper.c
	encinfo.c
	encode.c
	enquant.c
	fdct.c
	huffenc.c
	mathops.c
	mcenc.c
	rate.c
	tokenize.c
"""

decoder_sources = """
        apiwrapper.c
	bitpack.c
        decapiwrapper.c
        decinfo.c
        decode.c
        dequant.c
        fragment.c
        huffdec.c
        idct.c
        info.c
        internal.c
        quant.c
        state.c
"""

env = Environment()
if env['CC'] == 'gcc':
  env.Append(CCFLAGS=["-g", "-O2", "-Wall", "-Wno-parentheses"])

def CheckPKGConfig(context, version): 
  context.Message( 'Checking for pkg-config... ' ) 
  ret = context.TryAction('pkg-config --atleast-pkgconfig-version=%s' % version)[0] 
  context.Result( ret ) 
  return ret 

def CheckPKG(context, name): 
  context.Message( 'Checking for %s... ' % name )
  ret = context.TryAction('pkg-config --exists %s' % name)[0]
  context.Result( ret ) 
  return ret
     
def CheckSDL(context):
  name = "sdl-config"
  context.Message( 'Checking for %s... ' % name )
  ret = SCons.Util.WhereIs('sdl-config')
  context.Result( ret ) 
  return ret

# check for appropriate inline asm support
host_x86_32_test = """
    int main(int argc, char **argv) {
#if !defined(__i386__)
  #error not an x86 host: preprocessor macro __i386__ not defined
#endif
  return 0;
    }
    """
def CheckHost_x86_32(context):
  context.Message('Checking for an x86 host...')
  result = context.TryCompile(host_x86_32_test, '.c')
  context.Result(result)
  return result

host_x86_64_test = """
    int main(int argc, char **argv) {
#if !defined(__x86_64__)
  #error not an x86_64 host: preprocessor macro __x86_64__ not defined
#endif
  return 0;
    }
    """
def CheckHost_x86_64(context):
  context.Message('Checking for an x86_64 host...')
  result = context.TryCompile(host_x86_64_test, '.c')
  context.Result(result)
  return result

conf = Configure(env, custom_tests = {
  'CheckPKGConfig' : CheckPKGConfig,
  'CheckPKG' : CheckPKG,
  'CheckSDL' : CheckSDL,
  'CheckHost_x86_32' : CheckHost_x86_32,
  'CheckHost_x86_64' : CheckHost_x86_64,
  })
  
if not conf.CheckPKGConfig('0.15.0'): 
   print 'pkg-config >= 0.15.0 not found.' 
   Exit(1)

if not conf.CheckPKG('ogg'): 
  print 'libogg not found.' 
  Exit(1) 

if conf.CheckPKG('vorbis vorbisenc'):
  have_vorbis=True
else:
  have_vorbis=False

if conf.CheckPKG('libpng'):
  have_libpng=True
else:
  have_libpng=False
  
build_player_example=True
if not conf.CheckHeader('sys/soundcard.h'):
  build_player_example=False
if build_player_example and not conf.CheckSDL():
  build_player_example=False

if conf.CheckHost_x86_32():
  env.Append(CPPDEFINES='OC_X86_ASM')
  decoder_sources += """
        x86/mmxidct.c
        x86/mmxfrag.c
        x86/mmxstate.c
        x86/x86state.c
  """
  encoder_sources += """
	x86/mmxencfrag.c
	x86/mmxfdct.c
	x86/x86enc.c
	x86/mmxfrag.c
	x86/mmxidct.c
	x86/mmxstate.c
	x86/x86state.c
  """
elif conf.CheckHost_x86_64():
  env.Append(CPPDEFINES=['OC_X86_ASM', 'OC_X86_64_ASM'])
  decoder_sources += """
        x86/mmxidct.c
        x86/mmxfrag.c
        x86/mmxstate.c
        x86/x86state.c
  """
  encoder_sources += """
	x86/mmxencfrag.c
	x86/mmxfdct.c
	x86/x86enc.c
	x86/sse2fdct.c
	x86/mmxfrag.c
	x86/mmxidct.c
	x86/mmxstate.c
	x86/x86state.c
  """

env = conf.Finish()

env.Append(CPPPATH=['include'])
env.ParseConfig('pkg-config --cflags --libs ogg')

libtheoradec_Sources = Split(decoder_sources)
libtheoraenc_Sources = Split(encoder_sources)

libtheoradec_a = env.Library('lib/theoradec',
	path('lib', libtheoradec_Sources))
libtheoradec_so = env.SharedLibrary('lib/theoradec',
	path('lib', libtheoradec_Sources))

libtheoraenc_a = env.Library('lib/theoraenc',
	path('lib', libtheoraenc_Sources))
libtheoraenc_so = env.SharedLibrary('lib/theoraenc',
	path('lib', libtheoraenc_Sources) + [libtheoradec_so])

#installing
prefix='/usr'
lib_dir = prefix + '/lib'
env.Alias('install', prefix)
env.Install(lib_dir, [libtheoradec_a, libtheoradec_so])
env.Install(lib_dir, [libtheoraenc_a, libtheoraenc_so])

# example programs
dump_video = env.Clone()
dump_video_Sources = Split("""dump_video.c ../lib/libtheoradec.a""")
dump_video.Program('examples/dump_video', path('examples', dump_video_Sources))

dump_psnr = env.Clone()
dump_psnr.Append(LIBS='m')
dump_psnr_Sources = Split("""dump_psnr.c ../lib/libtheoradec.a""")
dump_psnr.Program('examples/dump_psnr', path('examples', dump_psnr_Sources))

if have_vorbis:
  encex = dump_video.Clone()
  encex.ParseConfig('pkg-config --cflags --libs vorbisenc vorbis')
  encex_Sources = Split("""
	encoder_example.c
	../lib/libtheoraenc.a 
	../lib/libtheoradec.a
  """)
  encex.Program('examples/encoder_example', path('examples', encex_Sources))

  if build_player_example:
    plyex = encex.Clone()
    plyex_Sources = Split("""
	player_example.c
	../lib/libtheoradec.a
    """)
    plyex.ParseConfig('sdl-config --cflags --libs')
    plyex.Program('examples/player_example', path('examples', plyex_Sources))

png2theora = env.Clone()
png2theora_Sources = Split("""png2theora.c
	../lib/libtheoraenc.a
	../lib/libtheoradec.a
""")
png2theora.ParseConfig('pkg-config --cflags --libs libpng')
png2theora.Program('examples/png2theora', path('examples', png2theora_Sources))
