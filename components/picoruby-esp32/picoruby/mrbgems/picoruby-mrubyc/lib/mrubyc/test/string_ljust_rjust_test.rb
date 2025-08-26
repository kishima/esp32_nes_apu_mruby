
class StringLjustRjustTest < Picotest::Test
  description "ljust"
  def test_ljust
    assert_equal "foo    ", "foo".ljust(7)
    assert_equal "foo", "foo".ljust(3)
    assert_equal "foo****", "foo".ljust(7, "*")
    assert_equal "foo", "foo".ljust(1, "*")
    assert_equal "foo", "foo".ljust(-1, "*")
  end

  description "ljust object_id"
  def test_ljust_id
    obj = "foo"
    assert_not_equal obj.object_id, obj.ljust(0).object_id
  end

  description "ljust with multiple_characters of padding"
  def test_ljust_multiple_chars_padding
    assert_equal "fooabca", "foo".ljust(7, "abc")
    assert_equal "foo", "foo".ljust(3, "abc")
    assert_equal "abcabca", "".ljust(7, "abc")
  end

  description "ljust no Integer"
  def test_ljust_no_Integer
    assert_raise(TypeError, "no implicit conversion into Integer") do
      "foo".ljust("7")
    end
  end

  description "ljust zero width padding"
  def test_ljust_zero_width_padding
    assert_raise(ArgumentError, "zero width padding") do
      "foo".ljust(7, "")
    end
  end

  description "rjust"
  def test_rjust
    assert_equal "    bar", "bar".rjust(7)
    assert_equal "bar", "bar".rjust(3)
    assert_equal "****bar", "bar".rjust(7, "*")
    assert_equal "bar", "bar".rjust(1, "*")
    assert_equal "bar", "bar".ljust(-1, "*")
  end

  description "rjust object_id"
  def test_rjust_id
    obj = "bar"
    assert_not_equal obj.object_id, obj.rjust(0).object_id
  end

  description "rjust with multiple_characters of padding"
  def test_rjust_multiple_chars_padding
    assert_equal "abcabar", "bar".rjust(7, "abc")
    assert_equal "bar", "bar".rjust(3, "abc")
    assert_equal "abcabca", "".rjust(7, "abc")
  end

  description "rjust no Integer"
  def test_rjust_no_Integer
    assert_raise(TypeError, "no implicit conversion into Integer") do
      "bar".rjust("7")
    end
  end

  description "rjust zero width padding"
  def test_rjust_zero_width_padding
    assert_raise(ArgumentError, "zero width padding") do
      "bar".rjust(7, "")
    end
  end
end

