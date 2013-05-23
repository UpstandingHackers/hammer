package com.upstandinghackers.hammer;
import java.util.HashMap;

public class Hammer
{
    public final static byte BYTE_BIG_ENDIAN = 0x1;
    public final static byte BIT_BIG_ENDIAN = 0x2;
    public final static byte BYTE_LITTLE_ENDIAN = 0x0;
    public final static byte BIT_LITTLE_ENDIAN = 0x0;
   
    static final HashMap<Integer, TokenType> tokenTypeMap = new HashMap<Integer, TokenType>();

    public enum TokenType
    {
        NONE(1),
        BYTES(2),
        SINT(4),
        UINT(8),
        SEQUENCE(16),
        ERR(32),
        USER(64);

        private int value;
        public int getValue() { return this.value; }
        private TokenType(int value) { this.value = value; }
    }

    static
    {
        for(TokenType tt : TokenType.values())
        {
            Hammer.tokenTypeMap.put(new Integer(tt.getValue()), tt);
        }
    }

    public static native ParseResult parse(Parser parser, byte[] input, int length);
    public static native Parser token(byte[] str, int length);
    public static native Parser ch(byte c);
    public static native Parser chRange(byte from, byte to);
    public static native Parser intRange(Parser p, long lower, long upper);
    public static native Parser bits(int len, boolean sign);
    public static native Parser int64();
    public static native Parser int32();
    public static native Parser int16();
    public static native Parser int8();
    public static native Parser uInt64();
    public static native Parser uInt32();
    public static native Parser uInt16();
    public static native Parser uInt8();
    public static native Parser whitespace(Parser p);
    public static native Parser left(Parser p, Parser q);
    public static native Parser right(Parser p, Parser q);
    public static native Parser middle(Parser p, Parser x, Parser q);
//    public static native Parser action(Parser p, Action a);
    public static native Parser in(byte[] charset, int length);
    public static native Parser endP();
    public static native Parser nothingP();
    public static native Parser sequence(Parser... parsers);
    public static native Parser choice(Parser... parsers);
    public static native Parser butNot(Parser p1, Parser p2);
    public static native Parser difference(Parser p1, Parser p2);
    public static native Parser xor(Parser p1, Parser p2);
    public static native Parser many(Parser p);
    public static native Parser many1(Parser p);
    public static native Parser repeatN(Parser p, int n);
    public static native Parser optional(Parser p);
    public static native Parser ignore(Parser p);
    public static native Parser sepBy(Parser p, Parser sep);
    public static native Parser sepBy1(Parser p, Parser sep);
    public static native Parser epsilonP();
    public static native Parser lengthValue(Parser length, Parser value);
//    public static native Parser attrBool(Parser p, Predicate pred);
    public static native Parser and(Parser p);
    public static native Parser not(Parser p);
    public static native Parser indirect();
}
