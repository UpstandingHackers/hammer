import com.upstandinghackers.hammer.*;
import java.util.Arrays;
/**
* Example JHammer usage
*/

public class Example
{

static {
    System.loadLibrary("jhammer");
}

private static void handle(ParseResult result)
{
    if(result == null)
    {
        System.out.println("FAIL");
    }
    else
    {
        System.out.println("PASS");
        handleToken(result.getAst());
    }
}

private static void handleToken(ParsedToken p)
{
    if(p==null)
    {
        System.out.println("Empty AST");
        return;
    }
    switch(p.getTokenType())
    {
        case NONE: out("NONE token type"); break;
        case BYTES: out("BYTES token type, value: " + Arrays.toString(p.getBytesValue())); break;
        case SINT: out("SINT token type, value: " + p.getSIntValue()); break;
        case UINT: out("UINT token type, value: " + p.getUIntValue()); break;
        case SEQUENCE: out("SEQUENCE token type"); for(ParsedToken tok : p.getSeqValue()) {handleToken(tok);} break;
        case ERR: out("ERR token type"); break;
        case USER: out("USER token type"); break;
    }
}

private static void out(String msg)
{
    System.out.println(">> " + msg);
}

public static void main(String args[])
{
    out("chRange");
    handle(Hammer.parse(Hammer.chRange((byte)0x30, (byte)0x39), "1".getBytes(), 1));
    handle(Hammer.parse(Hammer.chRange((byte)0x30, (byte)0x39), "a".getBytes(), 1));
    
    out("ch");
    handle(Hammer.parse(Hammer.ch((byte)0x31), "1".getBytes(), 1));
    handle(Hammer.parse(Hammer.ch((byte)0x31), "0".getBytes(), 1));
    
    out("token");
    handle(Hammer.parse(Hammer.token("herp".getBytes(), 4), "herp".getBytes(), 4));
    handle(Hammer.parse(Hammer.token("herp".getBytes(), 4), "derp".getBytes(), 4));
    
    out("intRange");
    byte inbytes[] = {0x31, 0x31, 0x31, 0x31};
    handle(Hammer.parse(Hammer.intRange(Hammer.uInt8(), 0L, 0x32), inbytes, inbytes.length));
    handle(Hammer.parse(Hammer.intRange(Hammer.uInt8(), 0L, 0x30), inbytes, inbytes.length));

    out("bits");
    handle(Hammer.parse(Hammer.bits(7, false), inbytes, inbytes.length));
    
    out("int64");
    byte ints[] = {(byte)0x8F, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF, (byte) 0xFF};
    handle(Hammer.parse(Hammer.int64(), ints, ints.length));
    handle(Hammer.parse(Hammer.int64(), inbytes, inbytes.length));

    out("choice");
    Parser two32s[] = {Hammer.intRange(Hammer.uInt32(), 0x00, 0x01), Hammer.int32()};
    handle(Hammer.parse(Hammer.choice(Hammer.intRange(Hammer.uInt32(), 0x00, 0x01), Hammer.int32()), ints, ints.length));

    out("sequence");
    byte i3[] = {(byte)'i', (byte)3, (byte)0xFF};
    Parser i3parsers[] = {Hammer.ch((byte)'i'), Hammer.uInt8(), Hammer.int8()};
    handle(Hammer.parse(Hammer.sequence(Hammer.ch((byte)'i'), Hammer.uInt8(), Hammer.int8()), i3, i3.length));

    
}



}
