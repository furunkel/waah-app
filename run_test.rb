#!/usr/bin/env ruby
#
# mrbgems test runner
#
#

if __FILE__ == $0
  repository, dir = 'https://github.com/mruby/mruby.git', 'tmp/mruby'
  build_args = ARGV

  Dir.mkdir 'tmp'  unless File.exist?('tmp')
  unless File.exist?(dir)
    system "git clone #{repository} #{dir}"
  end

  exit system(%Q[cd #{dir}; MRUBY_CONFIG=#{File.expand_path __FILE__} ruby minirake -v #{build_args.join(' ')}])
end

MRuby::Build.new do |conf|
  toolchain :clang
  conf.gembox 'full-core'

  conf.gem github: 'furunkel/waah-canvas' do |g|
    g.configure conf, :x11, false
  end

  conf.gem File.expand_path(File.dirname(__FILE__)) 
end


MRuby::CrossBuild.new('androideabi') do |conf|
  toolchain :androideabi
  conf.gembox 'full-core'

  conf.gem github: 'furunkel/waah-canvas' do |g|
    g.configure conf, :android, true
  end

  conf.gem File.expand_path(File.dirname(__FILE__)) 
end

MRuby::CrossBuild.new('win32') do |conf|
  toolchain :gcc
  conf.gembox 'full-core'

  conf.cc.command = '/opt/mingw32/bin/i686-w64-mingw32-gcc'
  conf.archiver.command = '/opt/mingw32/bin/i686-w64-mingw32-ar'
  conf.linker.command = '/opt/mingw32/bin/i686-w64-mingw32-gcc'

  ENV['RANLIB'] = '/opt/mingw32/bin/i686-w64-mingw32-ranlib'

  conf.gem github: 'furunkel/waah-canvas' do |g|
    g.configure conf, :windows, true
  end

  conf.gem File.expand_path(File.dirname(__FILE__)) 
end


