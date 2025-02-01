[test]

## Color enum
  - symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

## TokenKind enum
  - symbol        , str
  - ROOT          , "root"
  - KEYWORD_BEGIN , "keyword_begin"
  // keyword
  - NIL           , "nil"
  - TRUE          , "true"
  - FALSE         , "false"
  - BOOL          , "bool"
  - INT           , "int"
  - FLOAT         , "float"
  - STRING        , "string"
  - STRUCT        , "struct"
  - ENUM          , "enum"
  - IF            , "if"
  - ELSE          , "or"
  - WHILE         , "while"
  - FOR           , "for"
  - IN            , "in"
  - BREAK         , "break"
  - CONTINUE      , "continue"
  - SWITCH        , "switch"
  - CASE          , "case"
  - DEFAULT       , "default"
  - RETURN        , "return"
  - NOP           , "nop"
  // special
  - CALLER_LINE   , "$caller_line"
  - KEYWORD_END   , "keyword_end"
  // identifier
  - IDENT         , "ident"
  // literal
  - INTLIT        , "int_lit"
  - FLOATLIT      , "float_lit"
  - STRINGLIT     , "string_lit"
  // separator
  - LPAREN        , "("
  - RPAREN        , ")"
  - LBRACK        , "["
  - RBRACK        , "]"
  - LBRACE        , "{"
  - RBRACE        , "}"
  - SEMICOLON     , ";"
  - COLON         , ":"
  - COLON2        , "::"
  - BLOCKBEGIN    , "block_begin"
  - BLOCKEND      , "block_end"
  - MINUS3        , "---"
  - PERIOD        , "."
  - PERIOD2       , ".."
  - COMMA         , ","
  - HASH          , "#"
  - HASH2         , "##"
  - NEWLINE       , "\\n"
  // binary
  - PLUS          , "+"
  - MINUS         , "-"
  - ASTER         , "*"
  - SLASH         , "/"
  - PERCENT       , "%"
  // relational
  - EQUAL2        , "=="
  - EXCLAMEQ      , "!="
  - LT            , "<"
  - LTE           , "<="
  - GT            , ">"
  - GTE           , ">="
  // bitwise
  - LT2           , "<<"
  - GT2           , ">>"
  - CARET         , "^"
  - VBAR          , "|"
  - VBAR2         , "||"
  - AMPERSAND     , "&"
  - AMPERSAND2    , "&&"
  // unary
  - EXCLAM        , "!"
  - TILDE         , "~"
  // assign
  - EQUAL         , "="
  - PLUSEQ        , "+="
  - MINUSEQ       , "-="
  - ASTEREQ       , "*="
  - SLASHEQ       , "/="
  - PERCENTEQ     , "%="
  - LT2EQ         , "<<="
  - GT2EQ         , ">>="
  - CARETEQ       , "^="
  - VBAREQ        , "|="
  - AMPERSANDEQ   , "&="
  // eof
  - EOF           , "EOF"

# main(args []string) int

  ---
    - c = Color.A
    - s = c.name
    test.AssertS("alpha", s)
    test.AssertI(42, c.val)
    test.AssertI(42, Color.R.val)
    test.AssertI(4095, Color.B.val)

  ---
    test.AssertS("NIL", TokenKind.NIL.symbol)
    test.AssertS("block_begin", TokenKind.BLOCKBEGIN.str)

  ---
    - i = 3
    - c = Color.G
    switch c
    case Color.B
      i = 33
    case Color.R
      i = 44
    case Color.G
      i = 55
    default
      nop
    test.AssertI(55, i)

  print(test._test_count_, "tests done.")

  return 0
