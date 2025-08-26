class MyAlias
  # simple alias
  def method1
    return "MyClass#method1"
  end
  alias :method1_alias :method1

  # overriding original method
  def method2
    return "MyClass#method2"
  end
  alias :method2_alias :method2
  def method2
    return "MyClass#method2_alternate"
  end

  # overriding alias method
  def method3
    return "MyClass#method3"
  end
  alias :method3_alias :method3
  def method3
    return "MyClass#method3"
  end
  alias :method3_alias :method3
  def method3
    return "MyClass#method3_alternate"
  end
  alias :method3_alias :method3
end

class MyAliasTest < Picotest::Test
  def setup
    @obj = MyAlias.new
  end

  description 'alias'
  def test_method1
    assert_equal "MyClass#method1", @obj.method1
    assert_equal "MyClass#method1", @obj.method1_alias
  end

  description 'override original method'
  def test_method2
    assert_equal "MyClass#method2_alternate", @obj.method2
    assert_equal "MyClass#method2", @obj.method2_alias
  end

  description 'override alias method'
  def test_method3
    assert_equal "MyClass#method3_alternate", @obj.method3
    assert_equal "MyClass#method3_alternate", @obj.method3_alias
  end
end
