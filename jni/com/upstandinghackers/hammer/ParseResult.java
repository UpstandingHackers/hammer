package com.upstandinghackers.hammer;

import java.util.List;

public class ParseResult
{
    public native ParsedToken getAst();
    public native long getBitLength();
    
    public native void free();
    public long getInner() {return this.inner;}

    private long inner;
    ParseResult(long inner) {this.inner=inner;}
}
