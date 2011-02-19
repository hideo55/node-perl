import Utils

srcdir = '.'
blddir = 'build'
VERSION = '0.0.1'

def set_options(opt): 
    opt.tool_options('compiler_cxx')

def configure(conf):
    conf.check_tool('compiler_cxx')
    conf.check_tool('node_addon')
    conf.env.append_unique('CXXFLAGS',Utils.cmd_output('perl -MExtUtils::Embed -e ccopts').split())
    conf.env.append_unique('CXXFLAGS',['-Dusethreads'])
    conf.env.append_unique('LINKFLAGS',Utils.cmd_output('perl -MExtUtils::Embed -e ldopts').split())
    Utils.exec_command('perl -MExtUtils::Embed -e xsinit -- -o src/perlxsi.c')

def build(bld):
    obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
    obj.target = 'perl'
    obj.source = './src/perlxsi.c ./src/perl_bindings.cc'

