> test

## Color enum
  : symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

## TokenKind enum
  : symbol        , str
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
  - COLON2        , "."
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

## Difficulty enum
  : sym        , damage_coeff, time_coeff
  - EASY       , 0.5         , 1.5
  - NORMAL     , 1.0         , 1.0
  - HARD       , 1.5         , 0.8
  - NIGHTMARE  , 2.5         , 0.5

## Month enum
  : symbol , name         , num
  - Jan    , "January"    , 1
  - Feb    , "February"   , 2
  - Mar    , "March"      , 3
  - Apr    , "April"      , 4
  - May    , "May"        , 5
  - Jun    , "June"       , 6
  - Jul    , "July"       , 7
  - Aug    , "August"     , 8
  - Sep    , "September"  , 9
  - Oct    , "October"    , 10
  - Nov    , "November"   , 11
  - Dec    , "December"   , 12

# main(args vec{string}) int

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

  // enum with floating point values
  ---
    - d Difficulty

    d = Difficulty.EASY
    test.AssertS("EASY", d.sym)
    test.AssertF(0.5, d.damage_coeff)
    test.AssertF(1.5, d.time_coeff)

    d = Difficulty.NORMAL
    test.AssertF(1.0, d.damage_coeff)
    test.AssertF(1.0, d.time_coeff)

    d = Difficulty.HARD
    test.AssertF(1.5, d.damage_coeff)
    test.AssertF(0.8, d.time_coeff)

    d = Difficulty.NIGHTMARE
    test.AssertF(2.5, d.damage_coeff)
    test.AssertF(0.5, d.time_coeff)

  ---
    - month_menu = vec{"Select Month"}
    for m in Month
      vecpush(month_menu, m.name)
    test.AssertI(13, veclen(month_menu))
    test.AssertS("Select Month" , month_menu[0])
    test.AssertS("January"      , month_menu[1])
    test.AssertS("February"     , month_menu[2])
    test.AssertS("March"        , month_menu[3])
    test.AssertS("April"        , month_menu[4])
    test.AssertS("May"          , month_menu[5])
    test.AssertS("June"         , month_menu[6])
    test.AssertS("July"         , month_menu[7])
    test.AssertS("August"       , month_menu[8])
    test.AssertS("September"    , month_menu[9])
    test.AssertS("October"      , month_menu[10])
    test.AssertS("November"     , month_menu[11])
    test.AssertS("December"     , month_menu[12])

  print(test._test_count_, "tests done.")

  return 0
