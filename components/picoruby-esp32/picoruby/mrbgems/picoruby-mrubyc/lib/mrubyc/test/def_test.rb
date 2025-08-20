
class DefTest < Picotest::Test
  description "OP_DEF should return the symbol of the method name"
  METHOD_NAME_SYM = def test_op_def_return
    assert_equal(:test_op_def_return, METHOD_NAME_SYM)
  end
end
