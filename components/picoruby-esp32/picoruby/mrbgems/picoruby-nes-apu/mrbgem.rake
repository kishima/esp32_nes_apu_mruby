MRuby::Gem::Specification.new('picoruby-nes-apu') do |spec|
  spec.license = 'MIT'
  spec.author  = 'kishima'
  spec.summary = 'NES APU component'
  #spec.add_dependency 'picoruby-gpio'
  
  prj = ENV['IDF_PROJECT_PATH'] || File.expand_path('../../../../..', __dir__)
  spec.cc.include_paths << File.join(prj, 'components', 'apu_emu', 'include')
end
