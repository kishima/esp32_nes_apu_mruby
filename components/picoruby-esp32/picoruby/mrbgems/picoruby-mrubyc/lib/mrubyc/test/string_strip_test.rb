
class StringStripTest < Picotest::Test

  description "strip, lstrip, rstrip"
  def test_strip
    s = " a b c "
    assert_equal "a b c", s.strip
    assert_equal "a b c ", s.lstrip
    assert_equal " a b c", s.rstrip

    s = "abc"
    assert_equal "abc", s.strip
    assert_equal "abc", s.lstrip
    assert_equal "abc", s.rstrip

    s = " abc"
    assert_equal "abc", s.strip
    assert_equal "abc", s.lstrip
    assert_equal " abc", s.rstrip

    s = "abc "
    assert_equal "abc", s.strip
    assert_equal "abc ", s.lstrip
    assert_equal "abc", s.rstrip

    s = " "
    assert_equal "", s.strip
    assert_equal "", s.lstrip
    assert_equal "", s.rstrip

    s = ""
    assert_equal "", s.strip
    assert_equal "", s.lstrip
    assert_equal "", s.rstrip

    s = " \t\r\n\f\vABC \t\r\n\f\v\0"
    assert_equal "ABC", s.strip
    assert_equal "ABC \t\r\n\f\v\0", s.lstrip
    assert_equal " \t\r\n\f\vABC", s.rstrip

    s = " \0ABC\0 "
    assert_equal "\0ABC", s.strip
    assert_equal "\0ABC\0 ", s.lstrip
    assert_equal " \0ABC", s.rstrip

    s = "\0"
    assert_equal "", s.strip
    assert_equal "\0", s.lstrip
    assert_equal "", s.rstrip
  end

  description "strip!"
  def test_strip_bang
    s = " a b c "
    assert_equal "a b c", s.strip!
    assert_equal "a b c", s

    s = "abc"
    assert_equal nil, s.strip!
    assert_equal "abc", s

    s = " abc"
    assert_equal "abc", s.strip!
    assert_equal "abc", s

    s = "abc "
    assert_equal "abc", s.strip!
    assert_equal "abc", s

    s = " "
    assert_equal "", s.strip!
    assert_equal "", s

    s = ""
    assert_equal nil, s.strip!
    assert_equal "", s

    s = " \t\r\n\f\vABC \t\r\n\f\v\0"
    assert_equal "ABC", s.strip!
    assert_equal "ABC", s

    s = " \0ABC\0 "
    assert_equal "\0ABC", s.strip!
    assert_equal "\0ABC", s

    s = "\0"
    assert_equal "", s.strip!
    assert_equal "", s
  end

  description "lstrip!"
  def test_lstrip_bang
    s = " a b c "
    assert_equal "a b c ", s.lstrip!
    assert_equal "a b c ", s

    s = "abc"
    assert_equal nil, s.lstrip!
    assert_equal "abc", s

    s = " abc"
    assert_equal "abc", s.lstrip!
    assert_equal "abc", s

    s = "abc "
    assert_equal nil, s.lstrip!
    assert_equal "abc ", s

    s = " "
    assert_equal "", s.lstrip!
    assert_equal "", s

    s = ""
    assert_equal nil, s.lstrip!
    assert_equal "", s

    s = " \t\r\n\f\vABC \t\r\n\f\v\0"
    assert_equal "ABC \t\r\n\f\v\0", s.lstrip!
    assert_equal "ABC \t\r\n\f\v\0", s

    s = " \0ABC\0 "
    assert_equal "\0ABC\0 ", s.lstrip!
    assert_equal "\0ABC\0 ", s

    s = "\0"
    assert_equal nil, s.lstrip!
    assert_equal "\0", s
  end

  description "rstrip!"
  def test_rstrip_bang
    s = " a b c "
    assert_equal " a b c", s.rstrip!
    assert_equal " a b c", s

    s = "abc"
    assert_equal nil, s.rstrip!
    assert_equal "abc", s

    s = " abc"
    assert_equal nil, s.rstrip!
    assert_equal " abc", s

    s = "abc "
    assert_equal "abc", s.rstrip!
    assert_equal "abc", s

    s = " "
    assert_equal "", s.rstrip!
    assert_equal "", s

    s = ""
    assert_equal nil, s.rstrip!
    assert_equal "", s

    s = " \t\r\n\f\vABC \t\r\n\f\v\0"
    assert_equal " \t\r\n\f\vABC", s.rstrip!
    assert_equal " \t\r\n\f\vABC", s

    s = " \0ABC\0 "
    assert_equal " \0ABC", s.rstrip!
    assert_equal " \0ABC", s

    s = "\0"
    assert_equal "", s.rstrip!
    assert_equal "", s
  end

  description "chomp"
  def test_chomp
    s1 = "foo\r\n"
    s2 = s1.chomp
    assert_equal "foo\r\n", s1
    assert_equal "foo", s2

    s1 = "foo\r"
    s2 = s1.chomp
    assert_equal "foo\r", s1
    assert_equal "foo", s2

    s1 = "foo\n"
    s2 = s1.chomp
    assert_equal "foo\n", s1
    assert_equal "foo", s2

    s1 = "foo"
    s2 = s1.chomp
    assert_equal "foo", s1
    assert_equal "foo", s2

    s1 = ""
    s2 = s1.chomp
    assert_equal "", s1
    assert_equal "", s2

    s1 = "\r\n"
    s2 = s1.chomp
    assert_equal "\r\n", s1
    assert_equal "", s2

    s1 = "foo\r\n\r\n"
    s2 = s1.chomp
    assert_equal "foo\r\n\r\n", s1
    assert_equal "foo\r\n", s2
  end

  description "chomp!"
  def test_chomp_bang
    s = "foo\r\n"
    assert_equal "foo", s.chomp!
    assert_equal "foo", s

    s = "foo"
    assert_equal nil, s.chomp!
    assert_equal "foo", s

    s = ""
    assert_equal nil, s.chomp!
    assert_equal "", s

    s = "\r\n"
    assert_equal "", s.chomp!
    assert_equal "", s
  end

end
