class MyEqual3
end

class MyEqual3Test < Picotest::Test

  description "Integer Integer"
  def test_fixnum_fixnum
    assert_equal true,  1 === 1
    assert_equal false, 1 === 2
  end

  description "Integer Float"
  def test_fixnum_float
    assert_equal true,  1 === 1.0
    assert_equal false, 1 === 1.1
  end

  description "Float Integer"
  def test_float_fixnum
    assert_equal true,  1.0 === 1
    assert_equal false, 1.0 === 2
  end

  description "nil"
  def test_nil
    assert_equal true,  nil === nil
    assert_equal false, nil === 1
    assert_equal false, 1 === nil
  end

  description "False, True"
  def test_boolean
    assert_equal true,  false === false
    assert_equal false, false === true
    assert_equal false, true === false
    assert_equal true,  true === true
    assert_equal false, false === 1
    assert_equal false, true === 1
  end

  description "Symbol"
  def test_symbol
    assert_equal true,  :Symbol === :Symbol
    assert_equal false, :Symbol === :Symbol2
  end

  description "String"
  def test_string
    assert_equal true, "abcde" === "abcde"
    assert_equal false, "abcde" === "abcd"
    assert_equal false, "abcd" === "abcde"
    assert_equal false, "abcde" === "abCde"
    assert_equal false, "abcde" === "abcdE"
  end

  description "Array"
  def test_array
    assert_equal true,  [1,2,3] === [1,2,3]

    assert_equal true,  Array === [1,2,3]
    assert_equal false, Array === {}
  end

  description "Hash"
  def test_hash
    assert_equal true,  {:k=>"v"} === {:k=>"v"}
    assert_equal true,  Hash === {:k=>"v"}
    assert_equal false, Hash === [1,2,3]
  end

  description "Range"
  def test_range
    assert_equal true,  Range === (1..3)
    assert_equal true,  (1..3) === 1
    assert_equal true,  (1..3) === 3
    assert_equal false, (1..3) === 4

    assert_equal true,  (1...3) === 1
    assert_equal false, (1...3) === 3
    assert_equal false, (1...3) === 4
  end

  description "Object"
  def test_object
    assert_equal true,  Object === nil
    assert_equal true,  Object === false
    assert_equal true,  Object === true
    assert_equal true,  Object === 1
    assert_equal true,  Object === 1.1
    assert_equal true,  Object === :symbol
    assert_equal true,  Object === Object
    assert_equal true,  Object === []
    assert_equal true,  Object === "ABC"
    assert_equal true,  Object === {}
    assert_equal true,  Object === (1..3)

    assert_equal true,  Object === MyEqual3
    assert_equal true,  Object === MyEqual3.new
  end
end
