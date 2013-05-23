package com.upstandinghackers.hammer;

public class Parser
{
    public native void bindIndirect(Parser inner);
    public native void free();
    public long getInner() {return this.inner;}

    private long inner;
    Parser(long inner) {this.inner=inner;}
}
