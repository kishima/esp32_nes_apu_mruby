MRuby::Gem::Specification.new 'mruby-bin-mrbc2' do |spec|
  spec.license = 'MIT'
  spec.author  = 'mruby & PicoRuby developers'
  spec.summary = 'mruby compiler executable'

  spec.add_dependency "mruby-compiler2"
  spec.add_conflict 'mruby-compiler'
  spec.add_conflict 'mruby-bin-picorbc'

  cc.include_paths << "#{build.gems['mruby-compiler2'].dir}/lib/prism/include"

  mrbc2_objs = Dir.glob("#{dir}/tools/mrbc/*.c").map { |f| objfile(f.pathmap("#{build_dir}/tools/mrbc/%n")) }
  mrbc2_objs += build.gems['mruby-compiler2'].objs
  mrbc2_objs.delete_if{|o| o.include?("gem_init")}

  exe_name = 'picorbc'

  file exefile("#{build.build_dir}/bin/#{exe_name}") => mrbc2_objs do |t|
    build.linker.run t.name, t.prerequisites
  end

  build.bins << exe_name

end
