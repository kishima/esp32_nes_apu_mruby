class MethodMissing
  def method_missing(name, *args)
    "method_missing: #{name}, #{args.inspect}"
  end
end

class MethodMissingTest < Picotest::Test
  description 'method_missing'
  def test_method_missing
    assert_equal("method_missing: method_does_not_exist, [:foo]", MethodMissing.new.method_does_not_exist(:foo))
  end
end

