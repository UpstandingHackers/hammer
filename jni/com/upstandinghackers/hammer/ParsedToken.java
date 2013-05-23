package com.upstandinghackers.hammer;

public class ParsedToken
{
    public Hammer.TokenType getTokenType()
    {
        int tt = this.getTokenTypeInternal();
        if(0==tt)
            return null;
        return Hammer.tokenTypeMap.get(new Integer(tt));
    }

    private native int getTokenTypeInternal();
    public native int getIndex();
    public native byte getBitOffset();
    public native byte[] getBytesValue();
    public native long getSIntValue();
    public native long getUIntValue();
    public native double getDoubleValue();
    public native float getFloatValue();
    public native ParsedToken[] getSeqValue();
//    public native Object getUserValue();

    native void setTokenType(Hammer.TokenType type);
    native void setIndex(int index);
    native void setBitOffset(byte offset);
    native void setBytesValue(byte[] value);
    native void setSIntValue(long value);
    native void setUIntValue(long value);
    native void setDoubleValue(double value);
    native void setFloatValue(float value);
    native void setSeqValue(ParsedToken value[]);
//    native void setUserValue(Object value);
    
//    public native void free();
    public long getInner() {return this.inner;}

    private long inner;
    ParsedToken(long inner) {this.inner=inner;}
}
