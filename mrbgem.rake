MRuby::Gem::Specification.new('waah-app') do |spec|
  spec.license = 'MPL 2'
  spec.author  = 'furunkel'
  spec.bins = %w(waah)

  class << self
    def configure(conf, platform, build_deps)
      spec = self
      conf.gem github: 'furunkel/waah-canvas' do |g|
        g.configure conf, platform, build_deps, true
        spec.cc.include_paths.concat g.cc.include_paths
        spec.cc.defines.concat g.cc.defines
        conf.cc.defines.concat g.cc.defines
      end
    end
  end
end
