import unittest
import hammer as h

class TestTokenParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.token("95\xa2")
    def test_success(self):
        self.assertEqual(self.parser.parse("95\xa2"), "95\xa2")
    def test_partial_fails(self):
        self.assertEqual(self.parser.parse("95"), None)

class TestChParser(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser_int = h.ch(0xa2)
        cls.parser_chr = h.ch("\xa2")
    def test_success(self):
        self.assertEqual(self.parser_int.parse("\xa2"), 0xa2)
        self.assertEqual(self.parser_chr.parse("\xa2"), "\xa2")
    def test_failure(self):
        self.assertEqual(self.parser_int.parse("\xa3"), None)
        self.assertEqual(self.parser_chr.parse("\xa3"), None)

class TestChRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.ch_range("a", "c")
    def test_success(self):
        self.assertEqual(self.parser.parse("b"), "b")
    def test_failure(self):
        self.assertEqual(self.parser.parse("d"), None)

class TestInt64(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int64()
    def test_success(self):
        self.assertEqual(self.parser.parse("\xff\xff\xff\xfe\x00\x00\x00\x00"), -0x200000000)
    def test_failure(self):
        self.assertEqual(self.parser.parse("\xff\xff\xff\xfe\x00\x00\x00"), None)

class TestInt32(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int32()
    def test_success(self):
        self.assertEqual(self.parser.parse("\xff\xfe\x00\x00"), -0x20000)
        self.assertEqual(self.parser.parse("\x00\x02\x00\x00"), 0x20000)
    def test_failure(self):
        self.assertEqual(self.parser.parse("\xff\xfe\x00"), None)
        self.assertEqual(self.parser.parse("\x00\x02\x00"), None)

class TestInt16(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int16()
    def test_success(self):
        self.assertEqual(self.parser.parse("\xfe\x00"), -0x200)
        self.assertEqual(self.parser.parse("\x02\x00"), 0x200)
    def test_failure(self):
        self.assertEqual(self.parser.parse("\xfe"), None)
        self.assertEqual(self.parser.parse("\x02"), None)

class TestInt8(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int8()
    def test_success(self):
        self.assertEqual(self.parser.parse("\x88"), -0x78)
    def test_failure(self):
        self.assertEqual(self.parser.parse(""), None)

class TestUint64(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.uint64()
    def test_success(self):
        self.assertEqual(self.parser.parse("\x00\x00\x00\x02\x00\x00\x00\x00"), 0x200000000)
    def test_failure(self):
        self.assertEqual(self.parser.parse("\x00\x00\x00\x02\x00\x00\x00"), None)

class TestUint32(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.uint32()
    def test_success(self):
        self.assertEqual(self.parser.parse("\x00\x02\x00\x00"), 0x20000)
    def test_failure(self):
        self.assertEqual(self.parser.parse("\x00\x02\x00"), None)

class TestUint16(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.uint16()
    def test_success(self):
        self.assertEqual(self.parser.parse("\x02\x00"), 0x200)
    def test_failure(self):
        self.assertEqual(self.parser.parse("\x02"), None)

class TestUint8(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.uint8()
    def test_success(self):
        self.assertEqual(self.parser.parse("\x78"), 0x78)
    def test_failure(self):
        self.assertEqual(self.parser.parse(""), None)
        
class TestIntRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.int_range(h.uint8(), 3, 10)
    def test_success(self):
        self.assertEqual(self.parser.parse("\x05"), 5)
    def test_failure(self):
        self.assertEqual(self.parser.parse("\x0b"), None)

class TestWhitespace(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.whitespace(h.ch("a"))
    def test_success(self):
        self.assertEqual(self.parser.parse("a"), "a")
        self.assertEqual(self.parser.parse(" a"), "a")
        self.assertEqual(self.parser.parse("  a"), "a")
        self.assertEqual(self.parser.parse("\ta"), "a")
    def test_failure(self):
        self.assertEqual(self.parser.parse("_a"), None)

class TestWhitespaceEnd(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.whitespace(h.end_p())
    def test_success(self):
        self.assertEqual(self.parser.parse(""), None) # empty string
        self.assertEqual(self.parser.parse("  "), None) # empty string
    def test_failure(self):
        self.assertEqual(self.parser.parse("  x"), None)

class TestLeft(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.left(h.ch("a"), h.ch(" "))
    def test_success(self):
        self.assertEqual(self.parser.parse("a "), "a")
    def test_failure(self):
        self.assertEqual(self.parser.parse("a"), None)
        self.assertEqual(self.parser.parse(" "), None)
        self.assertEqual(self.parser.parse("ab"), None)

class TestRight(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.right(h.ch(" "), h.ch("a"))
    def test_success(self):
        self.assertEqual(self.parser.parse(" a"), "a")
    def test_failure(self):
        self.assertEqual(self.parser.parse("a"), None)
        self.assertEqual(self.parser.parse(" "), None)
        self.assertEqual(self.parser.parse("ba"), None)

class TestMiddle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.middle(h.ch(" "), h.ch("a"), h.ch(" "))
    def test_success(self):
        self.assertEqual(self.parser.parse(" a "), "a")
    def test_failure(self):
        self.assertEqual(self.parser.parse("a"), None)
        self.assertEqual(self.parser.parse(" "), None)
        self.assertEqual(self.parser.parse(" a"), None)
        self.assertEqual(self.parser.parse("a "), None)
        self.assertEqual(self.parser.parse(" b "), None)
        self.assertEqual(self.parser.parse("ba "), None)
        self.assertEqual(self.parser.parse(" ab"), None)

class TestAction(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.action(h.sequence(h.choice(h.ch("a"), h.ch("A")),
                                         h.choice(h.ch("b"), h.ch("B"))),
                                lambda x: [y.upper() for y in x])
    def test_success(self):
        self.assertEqual(self.parser.parse("ab"), ["A", "B"])
        self.assertEqual(self.parser.parse("AB"), ["A", "B"])
    def test_failure(self):
        self.assertEqual(self.parser.parse("XX"), None)

class TestIn(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.in_("abc")
    def test_success(self):
        self.assertEqual(self.parser.parse("b"), "b") 
    def test_failure(self):
        self.assertEqual(self.parser.parse("d"), None)

class TestNotIn(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.not_in("abc")
    def test_success(self):
        self.assertEqual(self.parser.parse("d"), "d")
    def test_failure(self):
        self.assertEqual(self.parser.parse("a"), None)

class TestEndP(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"), h.end_p())
    def test_success(self):
        self.assertEqual(self.parser.parse("a"), ("a",))
    def test_failure(self):
        self.assertEqual(self.parser.parse("aa"), None)

class TestNothingP(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.nothing_p()
    def test_success(self):
        pass
    def test_failure(self):
        self.assertEqual(self.parser.parse("a"), None)

class TestSequence(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"), h.ch("b"))
    def test_success(self):
        self.assertEqual(self.parser.parse("ab"), ('a','b'))
    def test_failure(self):
        self.assertEqual(self.parser.parse("a"), None)
        self.assertEqual(self.parser.parse("b"), None)

class TestSequenceWhitespace(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"), h.whitespace(h.ch("b")))
    def test_success(self):
        self.assertEqual(self.parser.parse("ab"), ('a','b'))
        self.assertEqual(self.parser.parse("a b"), ('a','b'))
        self.assertEqual(self.parser.parse("a  b"), ('a','b'))
    def test_failure(self):
        self.assertEqual(self.parser.parse("a  c"), None)

class TestChoice(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.choice(h.ch("a"), h.ch("b"))
    def test_success(self):
        self.assertEqual(self.parser.parse("a"), "a")
        self.assertEqual(self.parser.parse("b"), "b")
    def test_failure(self):
        self.assertEqual(self.parser.parse("c"), None)

class TestButNot(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.butnot(h.ch("a"), h.token("ab"))
    def test_success(self):
        self.assertEqual(self.parser.parse("a"), "a")
        self.assertEqual(self.parser.parse("aa"), "a")
    def test_failure(self):
        self.assertEqual(self.parser.parse("ab"), None)

class TestButNotRange(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.butnot(h.ch_range("0", "9"), h.ch("6"))
    def test_success(self):
        self.assertEqual(self.parser.parse("4"), "4")
    def test_failure(self):
        self.assertEqual(self.parser.parse("6"), None)

class TestDifference(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.difference(h.token("ab"), h.ch("a"))
    def test_success(self):
        self.assertEqual(self.parser.parse("ab"), "ab")
    def test_failure(self):
        self.assertEqual(self.parser.parse("a"), None)

class TestXor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.xor(h.ch_range("0", "6"), h.ch_range("5", "9"))
    def test_success(self):
        self.assertEqual(self.parser.parse("0"), "0")
        self.assertEqual(self.parser.parse("9"), "9")
    def test_failure(self):
        self.assertEqual(self.parser.parse("5"), None)
        self.assertEqual(self.parser.parse("a"), None)

class TestMany(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.many(h.choice(h.ch("a"), h.ch("b")))
    def test_success(self):
        self.assertEqual(self.parser.parse(""), ())
        self.assertEqual(self.parser.parse("a"), ('a',))
        self.assertEqual(self.parser.parse("b"), ('b',))
        self.assertEqual(self.parser.parse("aabbaba"), ('a','a','b','b','a','b','a'))
    def test_failure(self):
        pass

class TestMany1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.many1(h.choice(h.ch("a"), h.ch("b")))
    def test_success(self):
        self.assertEqual(self.parser.parse("a"), ("a",))
        self.assertEqual(self.parser.parse("b"), ("b",))
        self.assertEqual(self.parser.parse("aabbaba"), ("a", "a", "b", "b", "a", "b", "a"))
    def test_failure(self):
        self.assertEqual(self.parser.parse(""), None)
        self.assertEqual(self.parser.parse("daabbabadef"), None)

class TestRepeatN(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.repeat_n(h.choice(h.ch("a"), h.ch("b")), 2)
    def test_success(self):
        self.assertEqual(self.parser.parse("abdef"), ('a', 'b'))
    def test_failure(self):
        self.assertEqual(self.parser.parse("adef"), None)
        self.assertEqual(self.parser.parse("dabdef"), None)

class TestOptional(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"), h.optional(h.choice(h.ch("b"), h.ch("c"))), h.ch("d"))
    def test_success(self):
        self.assertEqual(self.parser.parse("abd"), ('a','b','d'))
        self.assertEqual(self.parser.parse("acd"), ('a','c','d'))
        self.assertEqual(self.parser.parse("ad"), ('a',h.Placeholder(), 'd'))
    def test_failure(self):
        self.assertEqual(self.parser.parse("aed"), None)
        self.assertEqual(self.parser.parse("ab"), None)
        self.assertEqual(self.parser.parse("ac"), None)

class TestIgnore(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"), h.ignore(h.ch("b")), h.ch("c"))
    def test_success(self):
        self.assertEqual(self.parser.parse("abc"), ("a","c"))
    def test_failure(self):
        self.assertEqual(self.parser.parse("ac"), None)

class TestSepBy(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sepBy(h.choice(h.ch("1"), h.ch("2"), h.ch("3")), h.ch(","))
    def test_success(self):
        self.assertEqual(self.parser.parse("1,2,3"), ('1','2','3'))
        self.assertEqual(self.parser.parse("1,3,2"), ('1','3','2'))
        self.assertEqual(self.parser.parse("1,3"), ('1','3'))
        self.assertEqual(self.parser.parse("3"), ('3',))
        self.assertEqual(self.parser.parse(""), ())
    def test_failure(self):
        pass

class TestSepBy1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sepBy1(h.choice(h.ch("1"), h.ch("2"), h.ch("3")), h.ch(","))
    def test_success(self):
        self.assertEqual(self.parser.parse("1,2,3"), ('1','2','3'))
        self.assertEqual(self.parser.parse("1,3,2"), ('1','3','2'))
        self.assertEqual(self.parser.parse("1,3"), ('1','3'))
        self.assertEqual(self.parser.parse("3"), ('3',))
    def test_failure(self):
        self.assertEqual(self.parser.parse(""), None)

### segfaults
class TestEpsilonP1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"), h.epsilon_p(), h.ch("b"))
    def test_success(self):
        self.assertEqual(self.parser.parse("ab"), ("a", "b"))
    def test_failure(self):
        pass

class TestEpsilonP2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.epsilon_p(), h.ch("a"))
    def test_success(self):
        self.assertEqual(self.parser.parse("a"), ("a",))
    def test_failure(self):
        pass

class TestEpsilonP3(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"), h.epsilon_p())
    def test_success(self):
        self.assertEqual(self.parser.parse("a"), ("a",))
    def test_failure(self):
        pass

class TestAttrBool(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.attr_bool(h.many1(h.choice(h.ch("a"), h.ch("b"))),
                                 lambda x: x[0] == x[1])
    def test_success(self):
        self.assertEqual(self.parser.parse("aa"), ("a", "a"))
        self.assertEqual(self.parser.parse("bb"), ("b", "b"))
    def test_failure(self):
        self.assertEqual(self.parser.parse("ab"), None)

class TestAnd1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.and_(h.ch("0")), h.ch("0"))
    def test_success(self):
        self.assertEqual(self.parser.parse("0"), ("0",))
    def test_failure(self):
        pass

class TestAnd2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.and_(h.ch("0")), h.ch("1"))
    def test_success(self):
        pass
    def test_failure(self):
        self.assertEqual(self.parser.parse("0"), None)

class TestAnd3(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("1"), h.and_(h.ch("2")))
    def test_success(self):
        self.assertEqual(self.parser.parse("12"), ('1',))
    def test_failure(self):
        pass

class TestNot1(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"),
                                h.choice(h.ch("+"), h.token("++")),
                                h.ch("b"))
    def test_success(self):
        self.assertEqual(self.parser.parse("a+b"), ("a", "+", "b"))
    def test_failure(self):
        self.assertEqual(self.parser.parse("a++b"), None)

class TestNot2(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.parser = h.sequence(h.ch("a"), h.choice(h.sequence(h.ch("+"), h.not_(h.ch("+"))),
                                                    h.token("++")),
                                h.ch("b"))
    def test_success(self):
        self.assertEqual(self.parser.parse("a+b"), ('a', ('+',), 'b'))
        self.assertEqual(self.parser.parse("a++b"), ('a', "++", 'b'))
    def test_failure(self):
        pass

# ### this is commented out for packrat in C ...
# #class TestLeftrec(unittest.TestCase):
# #    @classmethod
# #    def setUpClass(cls):
# #        cls.parser = h.indirect()
# #        a = h.ch("a")
# #        h.bind_indirect(cls.parser, h.choice(h.sequence(cls.parser, a), a))
# #    def test_success(self):
# #        self.assertEqual(self.parser.parse("a"), "a")
# #        self.assertEqual(self.parser.parse("aa"), ["a", "a"])
# #        self.assertEqual(self.parser.parse("aaa"), ["a", "a", "a"])
# #    def test_failure(self):
# #        pass


class TestRightrec(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        #raise unittest.SkipTest("Bind doesn't work right now")
        cls.parser = h.indirect()
        a = h.ch("a")
        cls.parser.bind(h.choice(h.sequence(a, cls.parser),
                                 h.epsilon_p()))
    def test_success(self):
        self.assertEqual(self.parser.parse("a"), ('a',))
        self.assertEqual(self.parser.parse("aa"), ('a', ('a',)))
        self.assertEqual(self.parser.parse("aaa"), ('a', ('a', ('a',))))
    def test_failure(self):
        pass

# ### this is just for GLR
# #class TestAmbiguous(unittest.TestCase):
# #    @classmethod
# #    def setUpClass(cls):
# #        cls.parser = h.indirect()
# #        d = h.ch("d")
# #        p = h.ch("+")
# #        h.bind_indirect(cls.parser, h.choice(h.sequence(cls.parser, p, cls.parser), d))
# #        # this is supposed to be flattened
# #    def test_success(self):
# #        self.assertEqual(self.parser.parse("d"), ["d"])
# #        self.assertEqual(self.parser.parse("d+d"), ["d", "+", "d"])
# #        self.assertEqual(self.parser.parse("d+d+d"), ["d", "+", "d", "+", "d"])
# #    def test_failure(self):
# #        self.assertEqual(self.parser.parse("d+"), None)

