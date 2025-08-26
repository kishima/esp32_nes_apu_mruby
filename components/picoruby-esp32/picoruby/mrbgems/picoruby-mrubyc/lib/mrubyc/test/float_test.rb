
class FloatTest < Picotest::Test

  description "clamp"
  def test_clamp
    assert_equal 2, 10.0.clamp(0, 2)
    assert_equal 2, 10.0.clamp(-1, 2)
    assert_equal -2, -10.0.clamp(-2, 2)
    assert_equal 2.0, 10.0.clamp(0, 2.0)
    assert_equal 2, 10.0.clamp(-1.0, 2.0)
    assert_equal -2, -10.0.clamp(-2.0, 2)
    assert_raise(ArgumentError, "min argument must be smaller than max argument") do
      0.1.clamp(1, -1)
    end
    assert_raise(ArgumentError, "wrong number of arguments") do
      1.0.clamp(1)
    end
  end

end
