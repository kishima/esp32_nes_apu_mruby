
class MySuperTest < Picotest::Test

  class MySuper0
    attr_reader :a1, :a2

    def initialize( a1, a2 )
      @a1 = a1
      @a2 = a2
    end

    def method1( a1, a2 )
      @a1 = a1
      @a2 = a2
    end
  end

  class MySuper1 < MySuper0
    def initialize( a1 = 1, a2 = 2 )
      super
    end

    def method1( a1, a2 )
      a1 *= 2
      super( a1, a2*2 )
    end
  end

  class MySuper2 < MySuper1
    def method1( a1, a2 )
      super
      @a2 = 2222
    end
  end

  description "MySuper1"
  def test_super_1
    obj = MySuper1.new()
    assert_equal 1, obj.a1
    assert_equal 2, obj.a2

    obj.method1( 11, 22 )
    assert_equal 22, obj.a1
    assert_equal 44, obj.a2
  end

  description "MySuper2"
  def test_super_2
    obj = MySuper2.new()
    obj.method1( 11, 22 )
    assert_equal 22, obj.a1
    assert_equal 2222, obj.a2
  end

end
