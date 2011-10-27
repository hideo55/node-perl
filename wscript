import Options, Utils, re
from os import unlink, symlink, popen
from os.path import exists, lexists

srcdir = '.'
blddir = 'build'

def set_options(opt): 
    opt.tool_options('compiler_cxx')

def configure(conf):
    conf.check_tool('compiler_cxx')
    conf.check_tool('node_addon')

def build(bld):
    obj = bld.new_task_gen('cxx', 'shlib', 'node_addon')
    obj.env.Release = 'Debug'
    obj.cxxflags = Utils.cmd_output('perl -MExtUtils::Embed -e ccopts').split()
    obj.cxxflags.append('-Duseithreads');
    obj.linkflags = Utils.cmd_output('perl -MExtUtils::Embed -e ldopts').split()
    obj.target = 'perl'
    obj.source = './src/perlxsi.c ./src/perl_bindings.cc'

def shutdown():
  t = 'perl.node'
  node_version = Utils.cmd_output('node --version')
  if Options.commands['clean']:
    if lexists(t): unlink(t)
  else:
    v = ( Utils.cmd_output('node --version').split('.') )[1]
    print v
    if v >= 5 :
      if exists('build/default/' + t) and not lexists(t):
        symlink('build/default/' + t, t)
    else:
      if exists('build/Release/' + t) and not lexists(t):
        symlink('build/Release/' + t, t)
