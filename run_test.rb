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

MRuby::CrossBuild.new('linuxfb') do |conf|
  toolchain :clang
  conf.gembox 'full-core'

  conf.gem github: 'furunkel/waah-canvas' do |g|
    g.configure conf, :linuxfb, false
  end

  conf.gem File.expand_path(File.dirname(__FILE__)) 

end

__END__

MRuby::CrossBuild.new('androideabi') do |conf|
  toolchain :androideabi
  conf.gembox 'full-core'

  conf.gem File.expand_path(File.dirname(__FILE__)) do |g|
    g.configure conf, :android, true
  end
end

MRuby::CrossBuild.new('mingw32-i686') do |conf|
  toolchain :gcc
  conf.gembox 'full-core'

  cmd = lambda do |c|
    "#{File.join ENV['MINGW_TOOLCHAIN'], 'bin', ENV['MINGW_TOOLCHAIN_PREFIX']}-#{c}"
  end

  conf.cc.defines  << "WAAH_PLATFORM_WINDOWS"
  conf.cc.command = cmd["gcc"]
  conf.archiver.command = cmd["ar"]
  conf.linker.command = cmd["gcc"]

  conf.gem File.expand_path(File.dirname(__FILE__)) do |g|
    g.configure conf, :windows, true
  end
end

