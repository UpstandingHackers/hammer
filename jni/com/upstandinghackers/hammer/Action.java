package com.upstandinghackers.hammer;

import java.util.List;

public interface Action
{
    public List<ParsedToken> execute(ParseResult p);
}
