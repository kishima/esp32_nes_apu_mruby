class GlobalTest < Picotest::Test

  description "RUBY_ENGINE"
  def test_ruby_engine
    assert_equal "mruby/c", RUBY_ENGINE
  end

end
