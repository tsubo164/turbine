## Difficulty enum
  - sym        , damage_coeff, time_coeff
  - EASY       , 0.5         , 1.5
  - NORMAL     , 1.0         , 1.0
  - HARD       , 1.5         , 0.8
  - NIGHTMARE  , 2.5         , 0.5


# main(args vec{string}) int
  - d = Difficulty.EASY
  print(d.sym)
  print(d.damage_coeff)

  return 42
