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

# main() int
  - month_menu = vec{"Select Month"}

  for m in Month
    vecpush(month_menu, m.name)
    print(format("%2d: %s", m.num, m.name))
    nop

  print(month_menu)

  return 0
