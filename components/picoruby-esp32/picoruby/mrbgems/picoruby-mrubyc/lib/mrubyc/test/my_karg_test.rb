class MyKarg

  def with_initial_value(k: 1)
    k
  end

  def without_initial_value(k:)
    k
  end

  def combined_with_m_opt_rest(a, b, o1 = 1, o2 = 2, *c, k1:, k2: 3, k3:)
    k1 + k2 + k3
  end

  def combined_with_dict(k1: 0, k2:, **dict)
    k1 + k2 + dict[:k3] + dict[:k4]
  end

  def yield_case(opt: {})
    yield(opt)
  end

end

class MyKargTest < Picotest::Test
  def setup
    @obj = MyKarg.new
  end

  description 'karg with initial value'
  def test_karg_with_initial_value
    assert_equal 1, @obj.with_initial_value
    assert_equal 2, @obj.with_initial_value(k: 2)
  end

  description 'karg without initial value'
  def test_karg_without_initial_value
    assert_equal true, @obj.without_initial_value(k: true)
    assert_raise(ArgumentError, "missing keywords: k") do
      @obj.without_initial_value
    end
  end

  description 'karg combined with m, opt, and rest'
  def test_karg_combined_with_m_opt_rest
    assert_equal 13, @obj.combined_with_m_opt_rest("dummy", "dummy", k3: 1, k2: 2, k1: 10)
  end

  description 'karg combined with dict'
  def test_karg_combined_with_dict
    assert_equal 10, @obj.combined_with_dict(k2: 2, k1: 1, k3: 3, k4: 4)
    assert_equal 10, @obj.combined_with_dict(**{k2: 2, k1: 1, k3: 3, k4: 4})
    assert_equal 10, @obj.combined_with_dict(k2: 2, k1: 1, **{k3: 3, k4: 4})
    assert_equal 10, @obj.combined_with_dict(k2: 2, k1: 1, **{k3: 3}, **{k4: 4})
  end

  description 'karg in block argument'
  def test_karg_in_block_argument
    p = Proc.new do |a:, b: 11|
      a + b
    end
    assert_equal 25, p.call(b: 12, a: 13)
  end

  description 'yield case'
  def test_yield
    assert_equal({:a => 0}, @obj.yield_case(opt: {a: 0}) { |opt| opt })
  end

end

